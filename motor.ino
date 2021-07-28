void motor_control()
/* Controlling the stepper motor.

   v2.0: A complete rewrite of the motion algorithm. Instead of trying to be a "real time", with some attempts to fix backwards (which didn't
         really work), now this is an accurate prediction - correction algorithm.

     I have two time lines - the real time (as returned by micros() ), and the model time: micros() - g.dt_lost
     The real and model times are only synced when we overshoot a special event - making a step. Sync is achieved by increasing g.dt_lost accordingly,
       so steps happen exactly as planned (in the model timeline), on integer values of the position.
     If a model change comes from a real time event (hit a limiter; pressed/depressed a key while moving), the action is taken only when we overshoot the next step
       (when we the real and model times are synced via g.dt_lost).
     Every time we discover that a step should have just happened:
     - First we adjust (sync) the model time (g.dt_lost)
     - We carry out the action (make a step plus optionally change direction)
     - The model is updated/changed as needed (e.g. there was a key press/depress, limiter hit, between the previous step and the current one)
     - And then we predict the model time when the next step should happen.

  This way, all motions happen exactly according to the model equations (in terms of position), with speeds a bit slower than in the model.
  This should result in zero missed steps even if some Arduino loops are significantly longer than a time between adjacent steps (because of displaying data, or some internal
  microprocessor delays - happens to ESP8266 regularly).

  Overall strategy:
  1) An event (external, internal) submits a request to initialize a new model of motion, of the following types:
   - GOTO: always starting from rest, go to a specific coordinate, stop there.
   - FF / REWIND: accelerate until hitting the speed limit (optionally), then hit a soft limit, decelerate and stop. Can be initiated while moving. This is the only model
      allowing for a direction change.
   - STOP / BREAK: break using the prescribed accceleration until it stops. Can only be called if moving. (STOP uses an intermediate acceleration, BREAK - maximum one)
  2) Inside motor_control, we use the model to predict the next step.
   - If currently at rest, start processing the model during the first call of the function
   - If currently moving, start processing the new model at the next step (when model and physical times are synced)
  3) Inside motor_control, execute the next event (step with an optional dir change) after the predicted time, while syncing the model and physical times.
  4) Make sure stop event is handled reliably.
   - Overshoot slightly all moves (by 0.5-1 microsteps) to account for floating point errors, and stop abruptly when reaching the target coordinate in the last model leg.
  5) Backlash is handled by overshooting the final leg of the model by a backlash amount - only if the leg is in the bad (negative) direction.
*/

{
  TIME_TYPE dt, dt_a;
  float dV;
  char new_accel;
  byte instant_stop, i_case;
  byte make_prediction = 0;

  // Current time in microseconds (might not be synced yet in the current Arduino loop):
  // Model time (can be behind or ahead of the physical time):
  g.t = micros() - g.dt_lost;

  // no motion and no change of model, so simply returning:
  if (g.moving == 0 && g.model_init == 0)
    return;

  // Checking just in case (should never happen):
  if (g.model_init == 1 && g.model_type == MODEL_NONE)
  {
    g.model_init == 0;
    return;
  }

  // If we are moving, but not at a sync (make a step) point yet - return:
  if (g.moving == 1 && g.t < g.t_next_step)
    return;

  // Reached a sync point while moving. Time to make a step, and sync the times, optionally update the model, and either stop or predict the next step.
  if (g.moving == 1 && g.t >= g.t_next_step)
  {
    // Executing one step:
    make_step();

    g.ipos = g.ipos_next_step; // Updating the current coordinate

    // Measuring backlash (it gets larger as we move in the bad - negative - direction,
    // and gets smaller as we move in the good - positive - direction):
    g.BL_counter -= g.direction;
    // Backlash cannot be negative:
    if (g.BL_counter < 0)
      g.BL_counter = 0;
    // and cannot be larger than g.backlash:
    if (g.BL_counter > g.backlash)
      g.BL_counter = g.backlash;

    // Syncing times
    g.dt_lost += g.t - g.t_next_step; // Adding the current step's time mismatch to the total mismatch
    g.t = g.t_next_step; // This syncs the model time. We are now at exactly the next_event point, from the model's point of view
  }

  // At this point, model and real times are guaranteed to be in sync (via g.dt_lost)

  if (g.model_init == 1 && g.uninterrupted == 0)
    // Optionally updating the model
  {
    // Generating the new model:
    generate_model();

    /// If the above command set model_type to NONE, exit:
    if (g.model_type == MODEL_NONE)
      return;
  }

  // Finding the model leg for the current point:
  char i0 = -1;
  TIME_TYPE dt = g.t - g.model_t0; // Current (already synced) model time, relative to the initial model time
  for (byte i = 0; i < g.Npoints - 1; i++)
  {
    if (dt >= g.model_time[i] && dt < g.model_time[i + 1])
    {
      i0 = i;
      break;
    }
  }
  if (i0 == -1)
    // Couldn't find a leg; a problem, should never happen. Emergency stop
  {
    stop_now();
    return;
  }

  // Immediate stop condition - if we are the the last leg of the model, and at the target coordinate:
  // This is where all normal motions should end
  if (i0 == g.Npoints - 1 && g.ipos == g.model_ipos1)
  {
    stop_now();
    return;
  }

  float d_pos = g.ipos - g.model_ipos0 - g.model_pos[i0]; // Current relative position within the current model leg
  float d_pos_next = d_pos + g.direction;  // Next step coordinate within the current leg, using current direction


  // Optional direction change
  // Direction change always happenes right after making a step (above)
  char i_dir = -1;
  for (byte i = i0 + 1; i++; i < g.Npoints - 1)
  {
    float delta = g.direction * (g.model_pos[i] - g.ipos); // How far is the leg's starting point, in the direction of motion
    // If a direction change happenes before the next step, we change direction now
    if (g.model_ptype[i] == DIR_CHANGE_POINT && delta < 1.0 && delta >= 0.0)
    {
      i_dir = i;
      g.direction = g.model_dir[i]; // Should be the more reliable way, compared to simply flipping the direction here
      motor_direction();
      motion_status();

      // Re-syncing the model to the dir change moment
      g.dt_lost += g.t - g.model_time[i]; // dt_lost becomes smaller, as we jump into future
      g.t = g.model_time[i]; // This syncs the model time. We are now at exactly the dir change point, from the model's point of view

      i0 = i;  // Updating current model leg
      // Coordinate (within the current leg) of the next step: moving 1 step in the direction opposite to the original direction (before dir change)
      d_pos_next = g.ipos + g.direction - g.model_pos[i];
      d_pos = 0.0;   // Updating the current position in the updated leg

      break;
    }
  }

  // Predicting the next step timing
  // At this point (as we took care of a dir change above), motions are guaranteed to be in one direction only.

  TIME_TYPE dt0 = g.t - g.model_time[i0];  // Current time relative to the leg's initial time
  TIME_TYPE t_step = 0; // Absolute time for the next step

  // The next step coordinate relative to the initial (first leg) model coordinate
  // It is a float, but it is effectively an integer
  float d_pos0_next = d_pos_next + g.model_pos[i0];
  char i_next = -1;

  for (byte i = i0; i < g.Npoints - 1; i++)
  {
    // Finding the model leg where the next step should happen:
    if (g.model_dir[i] == 1 && (d_pos0_next >= g.model_pos[i] && d_pos0_next < g.model_pos[i + 1]) ||
        g.model_dir[i] == -1 && (d_pos0_next <= g.model_pos[i] && d_pos0_next > g.model_pos[i + 1]))
    {
      i_next = i;

      if (i > i0)
        // Things to do if the next step takes us beyond the current leg
      {
        d_pos_next = d_pos0_next - g.model_pos[i]; // Distance to travel to next step within the leg it belongs to (with a sign)
        dt0 = 0.0;
      }

      if (g.model_ptype[i] == ZERO_ACCEL_POINT)
        // Zero current acceleration = linear leg.
      {
        t_step = d_pos_next / g.model_speed[i] + g.model_time[i] + g.model_t0;  // Absolute time
      }
      else
        // Constant acceleration leg
      {
        // Solving a quadratic equation
        // Discriminant square:
        float D2 = g.model_speed[i] * g.model_speed[i] + 2.0 * g.model_accel[i] * d_pos_next;
        // This should always be true. The only times it would be untrue if there is a direction change before the next step;
        // This was handled above, so should never happen.
        if (D2 .ge. 0.0)
        {
          float D = sqrt(D2);
          // Two possible prediction times, relative to the current time:
          TIME_TYPE dt1 = roundMy((-g.model_speed[i] - D) / g.model_accel[i]) - dt0;
          TIME_TYPE dt2 = roundMy((-g.model_speed[i] + D) / g.model_accel[i]) - dt0;
          // Picking a dt solution which is both physical (>=0 - in the future) and smaller than the other one (if they are both physical)
          if (dt1 >= 0 && (dt2 < 0 || dt1 <= dt2))
          {
            t_step = dt1 + dt0 + g.model_time[i] + g.model_t0;
          }
          else if (dt2 >= 0 && (dt1 < 0 || dt2 < dt1))
          {
            t_step = dt2 + dt0 + g.model_time[i] + g.model_t0;
          }
        }
        else
          // This should never happen. Emergency stop
        {
          stop_now();
          return;
        }

      }
    }
  }

  // Failed to predict the next step. This should never happen. Emergency stop
  if (t_step == 0 || i_next == -1 || t_step - g.t < 0)
  {
    stop_now();
    return;
  }


  g.t_next_step = t_step;  // Absolute time for the next step
  g.ipos_next_step = roundMy(d_pos0_next) + g.model_ipos0;  // Absolute coordinate for the next step

  return;

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void generate_model()
/* Generate a new model of motion, based on the current position and speed, and based on the requested new model
    (g.model_type). For g.model_type=MODEL_GOTO we also use the values of the target speed (g.model_speed_max), and
    target coordinate (g.model_ipos1).

    This function can only be called inside motor_control, and only after the model and real times are synced.
    (This happenes if at rest, or right after making a step.)

    The following models are supported (values of g.model_type):
    MODEL_NONE 0 : No model, no motion
    MODEL_GOTO 1 : GoTo model, can only start from rest, cannot be interrupted by FF, REWIND, STOP. Needs model_speed and model_ipos1
    MODEL_FF 2 : Fast-Forward model, ignored if current model is GOTO or BREAK. Uses intermediate acceleration, and maximum speed limit
    MODEL_REWIND 3 : Rewind model, ignored if current model is GOTO or BREAK. Uses intermediate acceleration, and maximum speed limit
    MODEL_STOP 4 : Decelerate until stopped, using intermediate acceleration. Can be interrupted by FF and REWIND
    MODEL_BREAK 5 : Emergency breaking (hit a limiter etc). Decelerate until stopped, using maximum acceleration. Cannot be interrupted by anything

    Soft limits have to be enforced here!

    We apply here backlash compensation for STOP, BREAK, and GOTO models moving in the bad (negative) direction at the final model leg.

*/
{
  float speed0, dx_break, accel, Vmax0, Vmax;
  float model_time[N_POINTS_MAX]; // Time intervals for the previous leg (ending at the current point)
  byte speed_changes, dir_change;

  if (g.model_type == MODEL_NONE || g.model_init == 0)
  {
    stop_now();  // This also will be a signal to immediately exit from motor_control
    return;
  }

  // We are already at the destination, so just returning (only for GOTO model):
  if (g.model_type == MODEL_GOTO && g.ipos == g.model_ipos1 && g.moving == 0)
  {
    stop_now();  // This also will be a signal to immediately exit from motor_control
    return;
  }

  // Limits enforced
  if (g.model_type == MODEL_FF && g.ipos >= g.limit2 || g.model_type == MODEL_REWIND && g.ipos <= g.limit1)
  {
    stop_now();  // This also will be a signal to immediately exit from motor_control
    return;
  }

  speed0 = current_speed();  // Current speed (signed), based on the current model; will be the initial speed in the new model

  // First model point (direction and acceleration will be defined later):
  g.Npoints = 0;  // Point counter
  model_time[g.Npoints] = 0.0;  // time spent in the previous leg of the model
  g.model_speed[g.Npoints] = speed0;  // Speed at this point
  g.model_pos[g.Npoints] = 0.0; // Model position (float) relative to the initial point (g.model_ipos0)
  g.model_ptype[g.Npoints] = INIT_POINT;  // Point type

  // Global model parameters:
  g.model_ipos0 = g.ipos;  // Initial absolute coordinate
  g.model_t0 = g.t;  // Initial absolute time


  //++++++++++++++++++++  STOP and BREAK models +++++++++++++++++++++++
  if (g.model_type == MODEL_STOP || g.model_type == MODEL_BREAK)
  {
    // Acceleration vector (we are deccelerating):
    if (g.model_type == MODEL_STOP)
      // Interemediate deceleration value:
      accel = -g.direction * g.accel_v[3];
    else
      // Maximum deceleration:
      accel = -g.direction * g.accel_v[4];
    // Breaking travel vector with the current speed and accel:
    dx_break = -0.5 * speed0 * speed0 / accel;
    COORD_TYPE dx_break_int = myRound(dx_break);  // Rounding to the nearest integer
    g.model_ipos1 = g.ipos + dx_break_int;  // The target coordinate is integer, nearest to the true number
    // Backlash compensation:
    if (g.direction == -1)
      g.model_ipos1 = g.model_ipos1 - g.backlash;
    // Revised value for dx_break, after making it integer and adding a small overshoot (0.5-1 steps):
    dx_break = (float)dx_break_int + g.direction * OVERSHOOT;
    // Revised acceleration, for the revised dx_break:
    accel = -0.5 * speed0 * speed0 / dx_break;

    // Missing data for the first point:
    g.model_dir[g.Npoints] = g.direction; // Maintaining the current direction
    g.model_accel[g.Npoints] = accel;  // Acceleration corrected for overshoot

    // The second (and final) point:
    g.Npoints++;
    g.model_dir[g.Npoints] = g.direction;
    g.model_accel[g.Npoints] = 0;
    model_time[g.Npoints] = -speed0 / accel; // Time corrected for overshoot
    g.model_speed[g.Npoints] = 0;
    g.model_pos[g.Npoints] = dx_break; // Coordinate corrected for overshoot
    g.model_ptype[g.Npoints] = STOP_POINT;
  }

  else

    //++++++++++++++++++++  GOTO, REWIND and FF models +++++++++++++++++++++++
  {
    // Backlash compensation for GOTO model:
    if (g.model_type == MODEL_GOTO && g.model_ipos1 < g.ipos)
      g.model_ipos1 = g.model_ipos1 - g.backlash;

    // Models FF and REWIND do not have explicit destination. We set it here to the corresponding soft limit
    if (g.model_type == MODEL_FF)
      g.model_ipos1 = g.limit2;
    else if (g.model_type == MODEL_REWIND)
      g.model_ipos1 = g.limit1;
    else
    {
      // Enforcing soft limits for GOTO model:
      if (g.model_ipos1 > g.limit2)
        g.model_ipos1 = g.limit2;
      if (g.model_ipos1 < g.limit1)
        g.model_ipos1 = g.limit1;
    }

    // Accurate travel vector (if no accel limit existed):
    COORD_TYPE dx = g.model_ipos1 - g.ipos;
    // Travel vector with a small overshoot applied:
    float dx_prime;
    if (dx >= 0)
      dx_prime = (float)dx + OVERSHOOT;
    else
      dx_prime = (float)dx - OVERSHOOT;

    // Determining the direction of the motion for the first leg of the new model:
    if (g.moving == 0)
    {
      if (dx_prime > 0.0)
        g.direction = 1;
      else
        g.direction = -1;

      motion_status();
      motor_direction(); // Explicitly sending the proper direction command to the motor
    }

    // Initial acceleration
    if (g.model_type == MODEL_FF)
      accel = g.accel_v[3];
    else if (g.model_type == MODEL_REWIND)
      accel = g.accel_v[1];
    else
      accel = g.direction * g.accel_v[4];  // Maximum acceleration used for all GOTO moves

    // Describing the motion model.
    // First point (the other parameters were assigned above):
    g.model_dir[g.Npoints] = g.direction;
    g.model_accel[g.Npoints] = accel;

    // Maximum allowed speed for the move (positive):
    if (g.model_type == MODEL_GOTO)
      Vmax0 = g.model_speed_max;
    else
      Vmax0 = SPEED_LIMIT;

    char direction = g.direction;

    float dx1;
    if (g.moving == 0)
      dx1 = 0.0;
    else
      // Travel vector to get to the zero speed
      // (if speed0 and accel have the same sign - this point is in the past,
      // if opposite signs - this is a dir change point, in the future)
      dx1 = -0.5 * speed0 * speed0 / accel;

    // Direction change point (never happens when starting from rest)
    if (g.moving == 1 && (accel > 0.0 && speed0 < 0.0 || accel < 0.0 && speed0 > 0.0))
    {
      g.Npoints++;
      direction = -direction;  // Flipping the direction
      g.model_dir[g.Npoints] = direction;
      g.model_accel[g.Npoints] = accel;  // Maintaining the same acceleration
      model_time[g.Npoints] = -speed0 / accel;
      g.model_speed[g.Npoints] = 0.0;
      g.model_pos[g.Npoints] = dx1;
      g.model_ptype[g.Npoints] = DIR_CHANGE_POINT;
    }

    // Maximum attained speed (if there were no speed limits), squared
    float Vmax2 = accel * (dx_prime - dx1);
    if (Vmax2 < 0.0)
      // Should never happen
    {
      stop_now();
      return;
    }
    Vmax = sqrt(Vmax2);

    if (Vmax > Vmax0)
      // We'll hit the speed limit, so need two more model points
    {
      // Hitting the speed limit point
      g.Npoints++;
      g.model_dir[g.Npoints] = direction;
      g.model_accel[g.Npoints] = 0;
      model_time[g.Npoints] = (direction * Vmax0 - g.model_speed[g.Npoints - 1]) / accel;
      g.model_speed[g.Npoints] = direction * Vmax0;
      g.model_pos[g.Npoints] = 0.5 * Vmax0 * Vmax0 / accel + dx1;
      g.model_ptype[g.Npoints] = ZERO_ACCEL_POINT;

      // The travel vector for the final leg:
      float dx2 = 0.5 * Vmax0 * Vmax0 / accel;

      // Leaving the speed limit point
      g.Npoints++;
      g.model_dir[g.Npoints] = direction;
      g.model_accel[g.Npoints] = -accel;
      model_time[g.Npoints] = direction * (dx_prime - dx2 - g.model_pos[g.Npoints - 1]) / Vmax0;
      g.model_speed[g.Npoints] = direction * Vmax0;
      g.model_pos[g.Npoints] = dx_prime - dx2;
      g.model_ptype[g.Npoints] = ACCEL_POINT;
    }
    else
      // Not hitting the speed limit, only one more model point
    {
      // Vmax point
      g.Npoints++;
      g.model_dir[g.Npoints] = direction;
      g.model_accel[g.Npoints] = -accel;  // Switching from acceleration to decelaration
      model_time[g.Npoints] = (direction * Vmax - g.model_speed[g.Npoints - 1]) / accel;
      g.model_speed[g.Npoints] = direction * Vmax;
      g.model_pos[g.Npoints] = 0.5 * Vmax2 / accel + dx1;
      g.model_ptype[g.Npoints] = ACCEL_POINT;
    }

    // The final point:
    g.Npoints++;
    g.model_dir[g.Npoints] = g.direction;
    g.model_accel[g.Npoints] = 0;
    model_time[g.Npoints] = -g.model_speed[g.Npoints - 1] / g.model_accel[g.Npoints - 1];
    g.model_speed[g.Npoints] = 0;
    g.model_pos[g.Npoints] = dx_prime;
    g.model_ptype[g.Npoints] = STOP_POINT;

  }  // else (non-breaking models)

  float time1 = 0.0;
  for (byte i = 0, i < g.Npoints)
  {
    // model_time is float, per leg timing for the previous leg:
    time1 += model_time[i];

    if (model_time[i] < 0.0)
      // This should never happen, something got messed up
    {
      stop_now();
      return;
    }

    // g.model_time is integer, cumulative time for each point (from the first model point):
    g.model_time[i] = roundMy(time1);
  }


  g.model_init = 0;
  g.moving = 1;

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


float current_speed()
// Calculating the current speed, for the current model. Only call when model/real times are synced.
{

  if (g.model_type == MODEL_NONE || g.moving == 0)
    return 0.0;

  // Finding the model leg for the current point:
  char i0 = -1;
  TIME_TYPE dt = g.t - g.model_t0; // Current (already synced) model time, relative to the initial model time
  for (byte i = 0; i < g.Npoints - 1; i++)
  {
    if (dt >= g.model_time[i] && dt < g.model_time[i + 1])
    {
      i0 = i;
      break;
    }
  }

  return  g.model_speed[i0] + g.model_accel[i0] * (dt - g.model_time[i0]);
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void stop_now()
/*
  Things to do when we decide to stop inside motor_control().
*/
{
#ifdef TIMING
  // Update timing stats for the very last loop in motion (before setting g.moving=0):
  //  timing();
  //  g.total_dt_timing =+ micros() - g.t0_timing;
#endif

  g.moving = 0;
  g.model_init = 0;
  g.model_type = MODEL_NONE;
  g.dt_lost = 0; // Now at rest, so model time is identical to the real time

  /*
      if (g.error == 1)
      {
        unsigned char limit_on = digitalRead(PIN_LIMITERS);
        // If we fixed the error 1 (limiter on initially) by rewinding to a safe area, set error code to 0:
        if (limit_on == LOW)
          g.error = 0;
      }
  */

  if (g.reg.save_energy)
  {
#ifndef DISABLE_MOTOR
    iochip.digitalWrite(EPIN_ENABLE, HIGH);
#endif
    delay(ENABLE_DELAY_MS);
  }

  // Saving the current position to EEPROM:
  EEPROM.put( ADDR_POS, g.ipos );

  if (g.stacker_mode >= 2 && g.backlashing == 0 && g.continuous_mode == 1)
  {
    // Ending focus stacking
    g.stacker_mode = 0;
  }

  // We can lower the breaking flag now, as we already stopped:
  g.uninterrupted = 0;
  g.backlashing = 0;
  // Refresh the whole display:
  display_all();
  if (g.noncont_flag > 0)
  {
    letter_status("S");
  }
  g.t_display = g.t;

  // Used in continuous_mode=0; we are here right after the travel to the next frame position
  if (g.noncont_flag == 4)
    g.noncont_flag = 1;

#ifdef TEST_SWITCH
  if (g.test_flag == 1 || g.test_flag == 5)
    g.test_flag = 2;
  if (g.test_flag == 3)
    g.test_flag = 4;
#endif

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void motor_direction()
// Sending a direction command to the motor, based on the current g.direction value
// Ideally should be done when not moving yet
{

  // Only sending command to motor if  there is a mismatch between the desired (g.direction) and current actual (g.dir) motor direction:
  if (g.dir != g.direction)
  {
    delayMicroseconds(STEP_LOW_DT); // Putting the delay here, as we might have just executed a step, in motor_control

    if (g.direction == 1)
    {
      g.dir = 1; // This variable reflect the actual state of the motor
#ifndef DISABLE_MOTOR
      digitalWrite(PIN_DIR, 1 - g.reg.straight);
#endif
    }
    else
    {
      g.dir = -1;
#ifndef DISABLE_MOTOR
      digitalWrite(PIN_DIR, g.reg.straight);
#endif
    }
  }

  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void make_step()
// Make one step
{
#ifndef DISABLE_MOTOR
  digitalWrite(PIN_STEP, LOW);
#endif
  // Minimum delay required by the driver:
  delayMicroseconds(STEP_LOW_DT);
#ifndef DISABLE_MOTOR
  digitalWrite(PIN_STEP, HIGH);
#endif

  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void go_to(COORDS_TYPE ipos1, float speed_max)
/*
  This command can be issued anywhere. It submits a request to generate a new motion model, at the next real/model time sync inside motor_control().

  - ipos1: integer destination coordinate for the move
  - speed_limit: maximum allowed speed during the move (float)

*/
{

  g.model_init = 1;
  g.model_type = MODEL_GOTO;
  g.model_speed_max = speed_max;
  g.model_ipos1 = ipos1;

  return;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void start_breaking()
// Initiating breaking at the highest deceleration allowed
{
  g.model_type = MODEL_BREAK;
  g.model_init = 1;
  g.uninterrupted = 1;
  letter_status("B");
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



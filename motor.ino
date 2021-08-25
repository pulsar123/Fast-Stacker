void motor_control()
/* Controlling the stepper motor.

   v2.0: A complete rewrite of the motion algorithm. Instead of trying to be "real time", with some attempts to fix backwards (which didn't
         really work), now this is an accurate prediction - correction algorithm.

     I have two timelines - the real time (as returned by micros() ), and the model time = micros() - g.dt_lost
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
   - STOP / BREAK: break using the prescribed accceleration until it stops. Can only be called if moving.
  2) Inside motor_control, we use the model to predict the next step.
   - If currently at rest, start processing the model during the first call of the function
   - If currently moving, start processing the new model at the next step (when model and physical times are synced)
  3) Inside motor_control, execute the next event (step with an optional dir change) after the predicted time, while syncing the model and physical times.
  4) Making sure stop event is handled reliably.
   - Overshoot slightly all moves (by ~0.5 microsteps) to account for floating point errors, and stop abruptly when reaching the target coordinate in the last model leg.
  5) Backlash is handled by overshooting the final leg of the model by a backlash amount - only if the leg is in the bad direction.
*/

{
  // Current time in microseconds (might not be synced yet in the current Arduino loop):
  // Model time (can be behind or ahead of the physical time):
  g.t = micros() - g.dt_lost;

  // no motion and no change of model, so simply returning:
  if (g.moving == 0 && g.model_init == 0)
    return;

  // Checking just in case (should never happen):
  if (g.model_init == 1 && g.model_type == MODEL_NONE)
  {
#ifdef SER_DEBUG
    Serial.println("g.model_init == 1 && g.model_type == MODEL_NONE!");
#endif
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
    if (g.reg.backlash_on * g.BL_counter < 0)
      g.BL_counter = 0;
    // and cannot be larger than g.backlash:
    if (g.reg.backlash_on == 1 && (g.BL_counter > g.backlash) || g.reg.backlash_on == -1 && (g.BL_counter < g.backlash))
      g.BL_counter = g.backlash;

    // Syncing times
    g.dt_lost += g.t - g.t_next_step; // Adding the current step's time mismatch to the total mismatch
    g.t = g.t_next_step; // This syncs the model time. We are now at exactly the next_step point, from the model's point of view
  }

  // At this point, model and real times are guaranteed to be in sync (via g.dt_lost)

  if (g.model_init == 1)
    // Optionally updating the model
  {
    // Generating the new model:
    generate_model();

    /// If the above command set model_type to NONE, exit:
    if (g.model_type == MODEL_NONE)
      return;

    if (g.reg.save_energy && g.enable_flag == HIGH)
    {
#ifndef DISABLE_MOTOR
      g.enable_flag = LOW;
      iochip.digitalWrite(EPIN_ENABLE, g.enable_flag);
      delay(ENABLE_DELAY_MS);
#endif
    }
  }

  // Finding the model leg for the current point:
  signed char i0 = -1;
  TIME_STYPE dt = g.t - g.model_t0; // Current (already synced) model time, relative to the initial model time
  for (byte i = 0; i < g.Npoints - 1; i++)
  {
    if (dt >= g.model_time[i] && dt < g.model_time[i + 1])
    {
      i0 = i;
      /*
        #ifdef SER_DEBUG
        if (g.model_type==MODEL_GOTO)
        {
        Serial.print("i = ");
        Serial.println(i);
        }
        #endif
      */
      break;
    }
  }
  if (i0 == -1)
    // Couldn't find a leg; a problem, should never happen. Emergency stop
  {
#ifdef SER_DEBUG
    Serial.println("***********Couldn't find a leg!");
#endif
    stop_now();
    return;
  }

  // Immediate stop condition - if we are the the last leg of the model, and at the target coordinate:
  // This is where all normal motions should end
  if (i0 == g.Npoints - 2 && g.ipos == g.model_ipos1)
  {
#ifdef SER_DEBUG
    Serial.println("Proper stop");
#endif
    stop_now();
    return;
  }

  float d_pos = g.ipos - g.model_ipos0 - g.model_pos[i0]; // Current relative position within the current model leg
  float d_pos_next = d_pos + g.direction;  // Next step coordinate within the current leg, using current direction


  // Optional direction change
  // Direction change always happenes right after making a step (above)
  signed char i_dir = -1;
  for (byte i = i0 + 1; i < g.Npoints - 1; i++)
  {
    float delta = g.direction * (g.model_ipos0 + g.model_pos[i] - g.ipos); // How far is the leg's starting point, in the direction of motion
    // If a direction change happenes before the next step, we change direction now
    if (g.model_ptype[i] == DIR_CHANGE_POINT && delta < 1.0 && delta >= 0.0)
    {
      i_dir = i;
      g.direction = g.model_dir[i]; // Should be the more reliable way, compared to simply flipping the direction here
      motor_direction();

      // Re-syncing the model to the dir change moment
      g.dt_lost += g.t - g.model_time[i] - g.model_t0; // dt_lost becomes smaller, as we jump into future
      g.t = g.model_t0 + g.model_time[i]; // This syncs the model time. We are now at exactly the dir change point, from the model's point of view

      i0 = i;  // Updating current model leg
      // Coordinate (within the current leg) of the next step: moving 1 step in the direction opposite to the original direction (before dir change)
      d_pos_next = g.ipos + g.direction - g.model_pos[i] - g.model_ipos0;
      d_pos = 0.0;   // Updating the current position in the updated leg

#ifdef SER_DEBUG
      Serial.print("Dir change; ipos=");
      Serial.print(g.ipos);
      Serial.print(", delta=");
      Serial.println(delta);
#endif

      break;
    }
  }

  // Predicting the next step timing
  // At this point (as we took care of a dir change above), motions are guaranteed to be in one direction only.

  TIME_STYPE dt0 = g.t - g.model_t0 - g.model_time[i0];  // Current time relative to the leg's initial time
  TIME_UTYPE t_step = 0; // Absolute time for the next step

  // The next step coordinate relative to the initial (first leg) model coordinate
  // It is a float, but it is effectively an integer
  float d_pos0_next = d_pos_next + g.model_pos[i0];
  signed char i_next = -1;

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
        // Solving a quadratic equation, to find the time for the next step
        // Discriminant square:
        float D2 = g.model_speed[i] * g.model_speed[i] + 2.0 * g.model_accel[i] * d_pos_next;
        // This should always be true. The only times it would be untrue if there is a direction change before the next step;
        // This was handled above, so should never happen.
        if (D2 >= 0.0)
        {
          float D = sqrt(D2);
          // Two possible prediction times, relative to the current time:
          TIME_STYPE dt1 = roundMy((-g.model_speed[i] - D) / g.model_accel[i]) - dt0;
          TIME_STYPE dt2 = roundMy((-g.model_speed[i] + D) / g.model_accel[i]) - dt0;
          // Picking a dt solution which is both physical (>=0 - in the future) and smaller than the other one (if they are both physical)
          if (dt1 >= 0 && (dt2 < 0 || dt1 <= dt2))
          {
            t_step = dt1 + dt0 + g.model_time[i] + g.model_t0;  // Absolute time
          }
          else if (dt2 >= 0 && (dt1 < 0 || dt2 < dt1))
          {
            t_step = dt2 + dt0 + g.model_time[i] + g.model_t0;  // Absolute time
          }
        }
        else
          // This should never happen. Couldn't find a leg for the next step. Emergency stop
        {
#ifdef SER_DEBUG
          Serial.println("***********Couldn't find a leg for the next step!");
#endif
          stop_now();
          return;
        }

      }
      break;
    }
  }

  // Failed to predict the next step. This should never happen. Instant stop
  if (t_step == 0 || i_next == -1 || t_step < g.t)
  {
#ifdef SER_DEBUG
    Serial.println("***********Failed to predict the next step!");
    Serial.print(t_step); Serial.print("  "); Serial.print(i_next); Serial.print("  "); Serial.println(g.t);
    Serial.print(g.ipos); Serial.print("  ");  Serial.println(d_pos0_next);
    Serial.print(dt); Serial.print("  ");  Serial.println(i0);
#endif
    stop_now();
    return;
  }

  // The final leg (before hitting the limits) of FF and REWIND models is uninterrupted, to prevent issues:
  if (i_next == g.Npoints - 1 && (g.model_type == MODEL_FF || g.model_type == MODEL_REWIND))
    g.uninterrupted = 1;

  g.t_next_step = t_step;  // Absolute time for the next step
  g.ipos_next_step = roundMy(d_pos0_next) + g.model_ipos0;  // Absolute coordinate for the next step

  return;

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void generate_model()
/*  Generate a new model of motion, based on the current position and speed, and based on the requested new model
    (g.model_type). For g.model_type=MODEL_GOTO we also use the values of the target speed (g.model_speed_max), and
    target coordinate (g.model_ipos1).

    This function can only be called inside motor_control, and only after the model and real times are synced.
    (This happenes if at rest, or right after making a step.)

    The following models are supported (values of g.model_type):
    MODEL_NONE 0 : No model, no motion
    MODEL_GOTO 1 : GoTo model, can only start from rest, cannot be interrupted by FF, REWIND, STOP. Needs model_speed and model_ipos1
    MODEL_FF 2 : Fast-Forward model, ignored if current model is GOTO or BREAK. Uses intermediate acceleration (except for the final leg, where it's at max), and maximum speed limit
    MODEL_REWIND 3 : Rewind model, ignored if current model is GOTO or BREAK. Uses intermediate acceleration (except for the final leg, where it's at max), and maximum speed limit
    MODEL_STOP 4 : Decelerate until stopped, using maximum acceleration. Can be interrupted by FF and REWIND
    MODEL_BREAK 5 : Emergency breaking (hit a limiter etc). Decelerate until stopped, using maximum acceleration. Cannot be interrupted by anything

    Soft limits have to be enforced here!

    We apply here backlash compensation for STOP, BREAK, and GOTO models moving in the bad direction at the final model leg.

    New: the original STOP model is way too slow when backlashing is enabled and the move is in the bad direction. The new strategy: in such situations,
    STOP model should essentially behave as REWIND (or FF, depending on the direction). It starts from a non-zero speed, and then goes to the
    calculated target position, which is the position it would stop without a backlash, minus (or plus) BACKLASH. As it is following a FF/REWIND
    model, it can first accelerate before slowing down and stopping at the destination. (This is unlike STOP model, which can only decelerated until stopped.)
    We move at the highest acceleration/deceleration, and maximum speed, so the move will be as quick as it can be.
*/
{
  float speed0, dx_break, accel, Vmax0, Vmax;
  float model_time[N_POINTS_MAX]; // Time intervals for the previous leg (ending at the current point)
  byte speed_changes, dir_change;

  if (g.model_type == MODEL_NONE || g.model_init == 0)
  {
#ifdef SER_DEBUG
    Serial.println("***********g.model_type == MODEL_NONE || g.model_init == 0");
#endif
    stop_now();  // This also will be a signal to immediately exit from motor_control
    return;
  }

  // We are already at the destination, so just returning (only for GOTO model):
  if (g.model_type == MODEL_GOTO && g.ipos == g.model_ipos1 && g.moving == 0)
  {
#ifdef SER_DEBUG
    Serial.println("We are already at the destination!");
#endif
    stop_now();  // This also will be a signal to immediately exit from motor_control
    return;
  }

  // Limits enforced
  if (g.model_type == MODEL_FF && g.ipos >= g.limit2 || g.model_type == MODEL_REWIND && g.ipos <= g.limit1)
  {
#ifdef SER_DEBUG
    Serial.println("Limits enforced");
    Serial.print(g.model_type);  Serial.print("  "); Serial.println(g.ipos);
#endif
    stop_now();  // This also will be a signal to immediately exit from motor_control
    return;
  }

  byte backlashed_stop = 0;

  speed0 = current_speed();  // Current speed (signed), based on the current model; will be the initial speed in the new model

  // First model point (direction and acceleration will be defined later):
  byte i_point = 0;  // Point counter
  model_time[i_point] = 0.0;  // time spent in the previous leg of the model
  g.model_speed[i_point] = speed0;  // Speed at this point
  g.model_pos[i_point] = 0.0; // Model position (float) relative to the initial point (g.model_ipos0)
  g.model_ptype[i_point] = INIT_POINT;  // Point type

  // Global model parameters:
  g.model_ipos0 = g.ipos;  // Initial absolute coordinate
  g.model_t0 = g.t;  // Initial absolute time

  //++++++++++++++++++++  STOP and BREAK models +++++++++++++++++++++++
  if (g.model_type == MODEL_STOP || g.model_type == MODEL_BREAK)
  {
    // Acceleration vector (we are deccelerating):
    // Maximum deceleration:
    accel = -g.direction * g.accel_v[4];
    // Breaking travel vector with the current speed and accel:
    dx_break = -0.5 * speed0 * speed0 / accel;
    COORD_TYPE dx_break_int = roundMy(dx_break);  // Rounding to the nearest integer
    g.model_ipos1 = g.ipos + dx_break_int;  // The target coordinate is integer, nearest to the true number
    // Revised value for dx_break, after making it integer and adding a small overshoot (0.5-1 steps):
    dx_break = (float)dx_break_int + g.direction * OVERSHOOT;
    // Backlash compensation when moving in the bad direction:
    if (g.direction == -g.reg.backlash_on)
    {
      g.model_ipos1 = g.model_ipos1 - g.backlash;
      dx_break = dx_break - g.backlash;
    }

    // Detecting if this is a special case of a STOP model moving in the bad direction:
    if (g.model_type == MODEL_STOP && g.reg.backlash_on != 0 && g.direction == -g.reg.backlash_on)
    {
      float V0 = fabs(speed0);  // Module of the initial speed
      // Breaking distance (absolute value) with the backlash overshooting:
      float dx1 = fabs(dx_break);
      // Old stype (constant deceleration) breaking time:
      float t_old = 2 * dx1 / V0;
      // New style (accel, then decel, using maximum accelerations) breaking time:
      float t_new = (V0 + 2 * sqrt(0.5 * V0 * V0 + ACCEL_LIMIT * dx1)) / ACCEL_LIMIT;
      // To use new style breaking, the new time should be shorter than the old one.
      // The 0.95 factors are to avoid floating point issues later on
      if (t_new < 0.95 * t_old && V0 < 0.95 * SPEED_LIMIT)
        backlashed_stop = 1;
    }

    if (backlashed_stop == 0)
      // Old style (constant deceleration) model
    {
      // Revised acceleration, for the revised dx_break:
      accel = -0.5 * speed0 * speed0 / dx_break;

      // Missing data for the first point:
      g.model_dir[i_point] = g.direction; // Maintaining the current direction
      g.model_accel[i_point] = accel;  // Acceleration corrected for overshoot

      // The second (and final) point:
      i_point++;
      g.model_dir[i_point] = g.direction;
      g.model_accel[i_point] = 0;
      model_time[i_point] = -speed0 / accel; // Time corrected for overshoot
      g.model_speed[i_point] = 0;
      g.model_pos[i_point] = dx_break; // Coordinate corrected for overshoot
      g.model_ptype[i_point] = STOP_POINT;
    }
  }

  if (g.model_type == MODEL_GOTO || g.model_type == MODEL_REWIND || g.model_type == MODEL_FF || (g.model_type == MODEL_STOP && backlashed_stop))
    //++++++++++++++++++++  GOTO, REWIND and FF models +++++++++++++++++++++++
  {
    // Backlash compensation for GOTO model:
    if (g.model_type == MODEL_GOTO && (g.reg.backlash_on == 1 && (g.model_ipos1 < g.ipos) || g.reg.backlash_on == -1 && (g.model_ipos1 > g.ipos)))
      g.model_ipos1 = g.model_ipos1 - g.backlash;

    if (backlashed_stop == 0)
    {
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

      motor_direction(); // Explicitly sending the proper direction command to the motor
    }

    // Initial and final accelerations
    float accel_last;
    if (g.model_type == MODEL_FF)
    {
      accel = g.accel_v[3];
      accel_last = g.accel_v[0];
    }
    else if (g.model_type == MODEL_REWIND)
    {
      accel = g.accel_v[1];
      accel_last = g.accel_v[4];
    }
    else
    {
      accel = g.direction * g.accel_v[4];  // Maximum acceleration is used for all GOTO moves, and for backlashed_stop=1 situation
      accel_last = -accel;
    }

    // Describing the motion model.
    // First point (the other parameters were assigned above):
    g.model_dir[i_point] = g.direction;
    g.model_accel[i_point] = accel;

    // Maximum allowed speed for the move (positive):
    if (g.model_type == MODEL_GOTO)
      Vmax0 = g.model_speed_max;
    else
      Vmax0 = SPEED_LIMIT;  // also works for backlashed_stop=1 situation (MODEL_STOP)

    signed char direction = g.direction;

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
      i_point++;
      direction = -direction;  // Flipping the direction
      g.model_dir[i_point] = direction;
      g.model_accel[i_point] = accel;  // Maintaining the same acceleration
      model_time[i_point] = -speed0 / accel;
      g.model_speed[i_point] = 0.0;
      g.model_pos[i_point] = dx1;
      g.model_ptype[i_point] = DIR_CHANGE_POINT;
    }

    // As first and last legs are now allowed to use different accelerations, we first compute
    // the coordinate of the point where the maximum speed (in the absence of speed limits) would be reached:
    float x1 = (dx1 * accel - dx_prime * accel_last) / (accel - accel_last);

    // Maximum attained speed (if there were no speed limits), squared
    float Vmax2 = 2 * accel * (x1 - dx1);
    if (Vmax2 < 0.0)
      // Should never happen
    {
#ifdef SER_DEBUG
      Serial.println("***********Vmax2 < 0.0!");
#endif
      stop_now();
      return;
    }
    Vmax = sqrt(Vmax2);

    if (Vmax > Vmax0)
      // We'll hit the speed limit, so need two more model points
    {
      // Hitting the speed limit point
      i_point++;
      g.model_dir[i_point] = direction;
      g.model_accel[i_point] = 0;
      model_time[i_point] = (direction * Vmax0 - g.model_speed[i_point - 1]) / accel;
      g.model_speed[i_point] = direction * Vmax0;
      g.model_pos[i_point] = 0.5 * Vmax0 * Vmax0 / accel + dx1;
      g.model_ptype[i_point] = ZERO_ACCEL_POINT;

      // The travel vector for the final leg:
      float dx2 = -0.5 * Vmax0 * Vmax0 / accel_last;

      // Leaving the speed limit point
      i_point++;
      g.model_dir[i_point] = direction;
      g.model_accel[i_point] = accel_last;
      model_time[i_point] = direction * (dx_prime - dx2 - g.model_pos[i_point - 1]) / Vmax0;
      g.model_speed[i_point] = direction * Vmax0;
      g.model_pos[i_point] = dx_prime - dx2;
      g.model_ptype[i_point] = ACCEL_POINT;
    }
    else
      // Not hitting the speed limit, only one more model point
    {
      // Vmax point
      i_point++;
      g.model_dir[i_point] = direction;
      g.model_accel[i_point] = accel_last;  // Switching from acceleration to decelaration
      model_time[i_point] = (direction * Vmax - g.model_speed[i_point - 1]) / accel;
      g.model_speed[i_point] = direction * Vmax;
      g.model_pos[i_point] = x1;
      g.model_ptype[i_point] = ACCEL_POINT;
    }

    // The final point:
    i_point++;
    g.model_dir[i_point] = direction;
    g.model_accel[i_point] = 0;
    model_time[i_point] = -g.model_speed[i_point - 1] / g.model_accel[i_point - 1];
    g.model_speed[i_point] = 0;
    g.model_pos[i_point] = dx_prime;
    g.model_ptype[i_point] = STOP_POINT;

  }  // if (non-breaking models or backlashed_stop=1)

  g.Npoints = i_point + 1;  // Number of model points

#ifdef SER_DEBUG
  Serial.print("Generated model #");
  Serial.println(g.model_type);
#endif

  float time1 = 0.0;
  for (byte i = 0; i < g.Npoints; i++)
  {
    if (model_time[i] < 0.0)
      // This should never happen, something got messed up
    {
#ifdef SER_DEBUG
      Serial.print("***********model_time[i] < 0.0");
      Serial.println(i);
      Serial.println(model_time[i]);
#endif
      stop_now();
      return;
    }

    // model_time is float, per leg timing for the previous leg:
    time1 += model_time[i];

    // g.model_time is integer, cumulative time for each point (from the first model point):
    g.model_time[i] = roundMy(time1);
  }

  // Threse should be at the very end. Now officially we are moving (even though we haven't made a single step yet).
  g.model_init = 0;
  g.moving = 1;

#ifdef SER_DEBUG
  Serial.print(g.limit1);  Serial.print("  ");  Serial.println(g.limit2);
  Serial.println(g.Npoints);
  Serial.print(g.model_ipos0);  Serial.print("  ");    Serial.println(g.model_ipos1);
  Serial.println(g.model_t0);
  Serial.println(g.direction);
  Serial.println(g.dt_lost);
  for (byte i = 0; i < g.Npoints; i++)
  {
    Serial.print(g.model_dir[i]);  Serial.print("  ");
    Serial.print(1e9 * g.model_accel[i]);  Serial.print("  ");
    Serial.print(g.model_time[i]);  Serial.print("  ");
    Serial.print(1000 * g.model_speed[i]);  Serial.print("  ");
    Serial.print(g.model_ipos0 + g.model_pos[i]);  Serial.print("  ");
    Serial.println(g.model_ptype[i]);
  }
#endif

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


float current_speed()
// Calculating the current speed, for the current model. Only call when model/real times are synced.
{

  if (g.model_type == MODEL_NONE || g.moving == 0)
    return 0.0;

  // Finding the model leg for the current point:
  signed char i0 = -1;
  TIME_STYPE dt = g.t - g.model_t0; // Current (already synced) model time, relative to the initial model time
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
#ifdef SER_DEBUG
  Serial.print("stop_now; ipos=");
  Serial.println(g.ipos);
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
    g.enable_flag = HIGH;
    iochip.digitalWrite(EPIN_ENABLE, g.enable_flag);
#endif
    delay(ENABLE_DELAY_MS);
  }

  // Saving the current position to EEPROM:
  EEPROM.put( ADDR_POS, g.ipos );

  if (g.stacker_mode >= 2 && g.Backlashing == 0 && g.continuous_mode == 1)
  {
    // Ending focus stacking
    g.stacker_mode = 0;
  }

  // We can lower the breaking flag now, as we already stopped:
  g.uninterrupted = 0;
  g.Backlashing = 0;

  // Refresh the whole display:
  if (g.alt_flag || g.error)
    display_all();
  else
  {
    display_current_position();
    display_status_line();
  }

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

#ifdef BUZZER
  if (g.accident_buzzer)
  {
    g.accident_buzzer = 0;
    // Starting a beep
    g.beep_length = ACCIDENT_BEEP_US; // Beep length in us
    g.beep_on = 1;
    g.t_beep = micros(); // We nee actual time, not g.t, for buzzer manipulation
    g.t_buzz = g.t_beep;
  }
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
      g.dir_raw = 1 - g.reg.straight; // Raw motor direction (to use in parking)
    }
    else
    {
      g.dir = -1;
      g.dir_raw = g.reg.straight;
    }
#ifndef DISABLE_MOTOR
    digitalWrite(PIN_DIR, g.dir_raw);
#endif
  }

  // Updating the display - only if the motion status changed:
  motion_status();

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

  // Raw coordinate (used in parking):
  if (g.dir_raw == 1)
    g.ipos_raw++;
  else
    g.ipos_raw--;

  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void go_to(COORD_TYPE ipos1, float speed_max)
/*
  This command can be issued anywhere. It submits a request to generate a new motion model, at the next real/model time sync inside motor_control().

  - ipos1: integer destination coordinate for the move
  - speed_limit: maximum allowed speed during the move (float)

*/
{
  // We can only initiate GoTo from rest:
  if (g.model_type != MODEL_NONE)
    return;

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

void parking()
// Parking
{

  byte raw_direction;
  if (g.dir_raw == 1)
    raw_direction = 1;
  else
    raw_direction = 0;

  // Figuring out the raw parking coordinate (multiples of 4 full steps):
  COORD_TYPE delta_ipos_park = g.ipos_raw % (4 * N_MICROSTEPS);

  // The nearest parking point:
  if (delta_ipos_park > 2 * N_MICROSTEPS)
    delta_ipos_park = delta_ipos_park - 4 * N_MICROSTEPS;
  else if (delta_ipos_park < -2 * N_MICROSTEPS)
    delta_ipos_park = delta_ipos_park + 4 * N_MICROSTEPS;

  // If program direction = raw direction,  my move in the programs coordinates will be  -delta_ipos_park
  // If program direction = -raw direction, my move in the programs coordinates will be   delta_ipos_park
  COORD_TYPE ipos_park;  // Parking coordinate in the model coordinates
  if (g.direction == raw_direction)
    ipos_park = g.ipos - delta_ipos_park;
  else
    ipos_park = g.ipos + delta_ipos_park;

  // Inforcing the limits:
  if (ipos_park > g.limit2 - BACKLASH)
  {
    // How many 4N steps to move back to become smaller than limit2:
    COORD_TYPE d2 = (ipos_park - (g.limit2 - BACKLASH)) / (4 * N_MICROSTEPS) + 1;
    ipos_park = ipos_park - d2 * 4 * N_MICROSTEPS;
  }
  else if (ipos_park < g.limit1 + BACKLASH)
  {
    // How many 4N steps to move forward to become larger than limit1+BACKLASH:
    COORD_TYPE d2 = (g.limit1 + BACKLASH - ipos_park) / (4 * N_MICROSTEPS) + 1;
    ipos_park = ipos_park + d2 * 4 * N_MICROSTEPS;
  }

  go_to(ipos_park, SPEED_LIMIT);  // Parking move

  display_comment_line("       Parked       ");
  EEPROM.commit();

  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


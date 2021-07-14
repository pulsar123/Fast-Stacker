void motor_control()
/* Controlling the stepper motor, based on current time, target speed (speed1), acceleration (accel),
   values at the last accel change (t0, pos0, speed0), and old integer position pos_int_old.

   Important: g.moving can be set to zero only here (by calling stop_now())! Also, it should be set to 1 only outside of this function.

   v2.0: I plan to completely re-write this algorithm. It should become one of a "prediction-correction" type. The main points:

     There is only one model special event requiring syncing the model and real times:
     - The moment when a step should be taken
     I have two time lines - the real time (as returned by micros() ), and the model time: micros() - g.dt_lost
     The real and model times are only synced when we overshoot a special event (making a step). Sync is achieved by increasing g.dt_lost accordingly,
       so all special events happen exactly as planned (in the model timeline).
     If a model change comes from a real time event (hit a limiter; pressed/depressed a key while moving), the action is taken only when we overshoot the next special event
       (when we the real and model times are synced via g.dt_lost).
     Every time we discover that a special event has just happened:
     - First we adjust (sync) the model time (g.dt_lost)
     - We carry out the action (make a step plus optionally change direction)
     - The model is updated/changed as needed (e.g. there was a key press/depress, limiter hit, between the previous step and the current one)
     - And then we predict the model time when the next special event
       (step) should happen.

  This way, all motions happen exactly according to the model equations (in terms of position), with speeds a bit slower than in the model. If properly implemented,
  this should result in zero missed steps even if some Arduino loops are significantly longer than a time between adjacent steps (because of displaying data, or some internal
  microprocessor delays - happens to ESP8266 regularly).

  Overall strategy:
  1) An event (external, internal) triggers a call to one of the three motion model generators. This assigns values to all model parameters and vectors, fully describing
  the motion until it stops. If there was a prior model, the new one replaces it.
   - go_to: always starting from rest, go to a specific coordinate, stop there.
   - accelerate: accelerate until hitting the speed limit (optionally), then hit a soft limit, decelerate and stop. Can be initiated while moving. This is the only model
      allowing for a direction change.
   - stop: break using the prescribed accceleration until it stops. Can only be called if moving.
  2) Inside motor_control, use the model to predict the next motor controlling event: a step.
   - If currently at rest, start processing the model during the first call of the function
   - If currently moving, start processing the new model at the next step (when model and physical times are synced)
  3) Inside motor_control, execute the next event (step with an optional dir change) after the predicted time, while syncing the model and physical times.
  4) Make sure stop event is handled reliably.
   - Overshoot slightly all moves (by 1-3 microsteps) to account for floating point errors, and stop abruptly when reaching the target coordinate in the last model leg.
*/

{
  TIME_TYPE dt, dt_a;
  float dV;
  char new_accel;
  byte instant_stop, i_case;
  byte make_prediction = 0;

  // Current time in microseconds (might not be synced yet in the current Arduino loop):
  // Model time (can be behind the physical time):
  g.t = micros() - g.dt_lost;

  // no motion, so simply returning:
  if (g.moving == 0 && g.started_moving == 0)
    return;

  if (g.started_moving == 1)
    // We are here if we are starting to move - initiated from a prior either go_to() or accelerate() commands (only when at rest)
  {
    g.started_moving = 0;
    g.moving = 1;
    g.speed = 0.0;
    g.model_t0 = g.t;
    // We make prediction either when we start from rest (here), or at the next model/real time sync (below)
    make_prediction = 1;
    // No need to sync model/real times here, as we start from rest, so they are identical for now (g.dt_lost=0)
  }


  // If it's time to execute an event (step), do it now, and sync model time with real time
  if (g.t >= g.t_next_step)
  {
    make_prediction = 1;

    make_step();
    g.ipos = g.ipos0 + g.d_ipos_next; // Current coordinate
    // Measuring backlash (it gets larger as we move in the bad - negative - direction,
    // and gets smaller as we move in the good - positive - direction):
    g.BL_counter = - g.direction;
    // Backlash cannot be negative:
    if (g.BL_counter < 0)
      g.BL_counter = 0;
    // and cannot be larger than g.backlash:
    if (g.BL_counter > g.backlash)
      g.BL_counter = g.backlash;


    // Syncing time
    g.dt_lost += g.t - g.t_next_event; // Adding the current step's time mismatch to the total mismatch
    g.t = g.t_next_event; // This syncs the model time. We are now at exactly the next_event point, from the model's point of view

  }

  if (make_prediction == 0)
    // If there is no need to make a prediction, we are done here
    return;

  // We are here only if we need to make a prediction
  // At this point, g.t has already beeing synced

  // Finding the model leg for the current time:
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
    // A problem, should never happen. Emergency stop
  {
    stop_now();
    return;
  }

  // Immediate stop condition - if we are the the last leg of the model, and at the target coordinate:
  if (i0 == g.Npoints - 1 && g.ipos == g.ipos_goto)
  {
    stop_now();
    return;
  }

  float d_pos = g.d_pos_next - g.model_pos[i0];   // Current relative position within the current model leg
  float d_pos_next = d_pos + g.direction;  // Next step coordinate within the current leg


  // Optional direction change
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
      // Integer coordinate (within the current leg) of the next step: moving 1 step in the direction opposite to the original direction (before dir change)
      d_pos_next = g.ipos + g.direction - g.model_pos[i];
      d_pos = 0.0;   // Updating the current position in the updated leg

      break;
    }
  }

  // Predicting the next step timing
  // At this point (as we took care of a dir change above), motions are guaranteed to be in one direction only.

  TIME_TYPE dt0 = g.t - g.model_time[i0];  // Current time relative to the leg's initial time
  TIME_TYPE t_step = 0; // Absolute time for the next step

  float d_pos0_next = d_pos_next + g.model_pos[i0]; // The next step coordinate relative to the initial (first leg) model coordinate
  char i_next = -1;

  for (byte i = i0; i < g.Npoints - 1; i++)
  {
    // Finding the model leg where the next step should happen:
    if (d_pos0_next >= g.model_pos[i] && d_pos0_next < g.model_pos[i + 1])
    {
      i_next = i;
      
      if (i > i0)
      // Things to do if the next step takes us beyond the current leg
      {
        d_pos_next = d_pos0_next - g.model_pos[i]; // Distance to travel to next step within the leg it belongs to (with a sign)
        dt0 = 0.0;
      }

      if (g.model_accel[i] == 0)
        // Zero current acceleration = linear leg.
      {
        t_step = d_pos_next / g.model_speed[i] + g.model_time[i];
      }
      else
        // Constant acceleration leg
      {
        // Solving a quadratic equation
        // Discriminant square:
        float D2 = g.model_speed[i] * g.model_speed[i] + 2.0 * g.accel_v[2 + g.model_accel[i]] * d_pos_next;
        // This should always be true. the only times it would be untrue if there is a direction change before the next step;
        // This was handled above, so should never happen.
        if (D2 .ge. 0.0)
        {
          float D = sqrt(D2);
          // Two possible prediction times, relative to the current time:
          TIME_TYPE dt1 = roundMy((-g.model_speed[i] - D) / g.accel_v[2 + g.model_accel[i]]) - dt0;
          TIME_TYPE dt2 = roundMy((-g.model_speed[i] + D) / g.accel_v[2 + g.model_accel[i]]) - dt0;
          // Picking a dt solution which is both physical (>=0) and smaller than the other one (if they are both physical)
          if (dt1 >= 0 && (dt2 < 0 || dt1 <= dt2))
          {
            t_step = dt1 + dt0 + g.model_time[i];
          }
          else if (dt2 >= 0 && (dt1 < 0 || dt2 < dt1))
          {
            t_step = dt2 + dt0 + g.model_time[i];
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
  if (t_step == 0 || i_next == -1)
  {
    stop_now();
    return;
  }


  g.t_next_step = t_step;
  g.d_ipos_next = roundMy(d_pos0_next);

  return;

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void change_speed(float speed1_loc, byte moving_mode1, char accel)
/* Run the function every time you want to change speed. It will figure out required acceleration based on current speed and speed1,
   and will update t0, speed0, pos0, if accel changed here. The parameter "accel" is the suggested acceleration (0, 1, or 2).
   Inputs:
    - speed1_loc: new target speed.
    When moving_mode1=1, global moving_mode=1 is  enabled (to be used in go_to).
*/
{
  char new_accel;

  // Ignore any speed change requests during emergency breaking  (after hitting a limiter)
  //!!!
  //    if (g.uninterrupted || g.calibrate_flag == 2)
  if (g.uninterrupted)
    return;

  g.moving_mode = moving_mode1;

  if (speed1_loc >= g.speed)
    // We have to accelerate
    new_accel = accel;
  else
    // Have to decelerate:
    new_accel = -accel;

  if (new_accel != g.accel)
    // Acceleration changed
  {
    g.accel = new_accel;
    // Memorizing the current values for t, speed and pos:
    g.t0 = g.t;
    g.speed0 = g.speed;
    g.pos0 = g.pos;
  }

  if (g.accel != 0 && g.moving == 0 && g.started_moving == 0)
  {
    // Starting moving
    g.started_moving = 1;
    //!!!!
    if (g.accel < 0 )
      g.direction = -1;
    else
      g.direction = 1;
    motion_status();
#ifndef DISABLE_MOTOR
    if (g.reg.save_energy)
    {
      iochip.digitalWrite(EPIN_ENABLE, LOW);
      delay(ENABLE_DELAY_MS);
    }
#endif
  }

  // Updating the target speed:
  g.speed1 = speed1_loc;
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void go_to(COORD_TYPE ipos1, float speed)
/* Initiating a travel to pos1 at given target speed (positive number) and maximum acceleration.
   With non-zero g.backlash constant, all go_to moves result in fully backlash-compensated moves. The
   backlash model used is the simplest possible: when starting from a good initial position
   (g.BL_counter=0), meaning the physical coordinate = program coordinate, and
   moving in the bad (negative pos) direction, the rail will not start moving until g.backlash
   steps are done (and g.BL_counter gets the largest possible value, g.backlash), and then it starts
   moving instantly. If switching direction, again the rail doesn't move until g.backlash steps
   are carried out, and g.BL_counter becomes 0 (smallest possible value), and then it starts moving
   instantly. The current physical coordinate of the rail is always connected to the program
   coordinate via equation:

   pos_phys = pos_prog + g.BL_counter

   v2.0
   Complete rework, based on model time, and models of motion. For now, only the simplest case -
   starting from rest - is considered. All coordinates are integers now (as we only sync model and real times
   when making a step, or changing the direction).

   Soft limit has to be enforced here!

*/
{
  float speed1_loc;
  byte speed_changes;

  // New (in v2.0): will do nothing unless we are at rest:
  if (g.moving || g.started_moving)
    return;

  // We are already there, and no need for backlash compensation, so just returning:
  if (ipos1 == g.ipos && g.BL_counter == 0)
    return;

  if (ipos1 >= g.ipos)
    g.direction = 1;
  else
    g.direction = -1;

  motion_status();
  motor_direction(); // Explicitly sending the proper direction command to the motor

  if (g.direction > 0)
    // Will be moving in the good (positive) direction (no need for backlash compensation):
  {
    speed1_loc = speed;
  }
  else
    // Will be moving in the bad (negative) direction (have to overshoot, for future backlash compensation):
  {
    // Overshooting by g.backlash microsteps (this will be compensated in backlash() function after we stop):
    ipos1 = ipos1 - g.backlash;
    speed1_loc = -speed;
  }

  // The coordinate to stop at, to be used in motor_control():
  g.ipos_goto = ipos1;

  // Distance to travel (always positive):
  float dx = (float)(fabs(ipos1 - g.ipos));

  // Describing the motion model.
  // First point:
  g.model_accel[0] = 2;
  g.model_time[0] = 0;
  g.model_speed[0] = 0.0;
  g.model_pos[0] = 0.0; // Model position (float) relative to the initial point (g.ipos0)
  g.model_ptype[0] = INIT_POINT;

  // First - how many model points? 3 or 4, depending on how far to travel, and the maximum travel speed.
  // Maximum speed (if there are no speed limits):
  float Vmax = sqrt(g.accel_limit * dx);
  if (Vmax > speed1_loc)
  {
    g.Npoints = 4; // More points, as we hit the speed limit

    g.model_accel[1] = 0;  // Hit the speed limit
    g.model_time[1] = speed1_loc / g.accel_limit; // Time to hit the speed limit
    g.model_speed[1] = speed;  // Speed at this moment (with a sign)
    g.model_pos[1] = g.direction * 0.5 * speed1_loc * speed1_loc / g.accel_limit;
    g.model_ptype[1] = ACCEL_CHANGE_POINT;

    float d2 = dx - g.accel_limit * g.model_time[1] * g.model_time[1];
    g.model_accel[2] = -2;  // Starting to decelerate
    g.model_time[2] = d2 / speed1_loc + g.model_time[1];
    g.model_speed[2] = speed;
    g.model_pos[2] = g.model_pos[1] + g.direction * d2;
    g.model_ptype[2] = ACCEL_CHANGE_POINT;

    g.model_accel[3] = 0;  // Stopped
    g.model_time[3] = g.model_time[2] + g.model_time[1];
    g.model_speed[3] = 0.0;
    g.model_pos[3] = g.direction * dx;
    g.model_ptype[3] = STOP_POINT;
  }
  else
  {
    g.Npoints = 3; // Not hitting the speed limit

    g.model_accel[1] = -2; // Decelerate after hitting the mid-point
    g.model_time[1] = sqrt(dx / g.accel_limit); // Mid-point time
    g.model_speed[1] = g.direction * g.accel_limit * g.model_time[1];
    g.model_pos[1] = g.direction * dx / 2;
    g.model_ptype[1] = ACCEL_CHANGE_POINT;

    g.model_accel[2] = 0;  // Stopped
    g.model_time[2] = 2.0 * g.model_time[1];
    g.model_speed[2] = 0.0;
    g.model_pos[2] = g.direction * dx;
    g.model_ptype[2] = STOP_POINT;
  }

  // For GoTo motions, direction doesn't change:
  for (byte i = 0, i < g.Npoints)
    g.model_dir[i] = g.direction;

  g.pos_stop_flag = 0;
  g.i_point = 0;
  g.model_init = 1;
  g.started_moving = 1;
  g.ipos0 = g.ipos;  // Initial coordinate
  // Initial time (model_time0) is not set here, as it'll be set in motor_control()

  return;

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
  g.model_change = 0;
  g.t_old = g.t;

  /*
    if (g.telescope == 0)
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
  if (!g.telescope)
    EEPROM.put( ADDR_POS, g.ipos );

  if (g.stacker_mode >= 2 && g.backlashing == 0 && g.continuous_mode == 1)
  {
    // Ending focus stacking
    g.stacker_mode = 0;
  }

  // We can lower the breaking flag now, as we already stopped:
  g.uninterrupted = 0;
  g.backlashing = 0;
  g.speed = 0.0;
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

  g.dt_lost = 0; // Now at rest, so model time is identical to the real time

#ifdef EXTENDED_REWIND
  g.no_extended_rewind = 0;
#endif
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
      g.dir = 0;
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


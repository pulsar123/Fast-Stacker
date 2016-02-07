void motor_control()
/* Controlling the stepper motor, based on current time, target speed (speed1), acceleration (accel),
   values at the last accel change (t0, pos0, speed0), and old integer position pos_old_short.

   Important: g.moving can be set to zero only here (by calling stop_now())! Also, it should be set to 1 only outside of this function.
 */
{
  unsigned long dt, dt_a;
  float dV;
  char new_accel;
  byte instant_stop, i_case;

  g.t_old = g.t;
  // Current time in microseconds:
#ifdef PRECISE_STEPPING
  // Moving the motor timer back in time if skipped steps were detected in this travel:
  g.t = micros() - g.dt_backlash;
#else
  g.t = micros();
#endif

  // If we initiated a movement elsewhere (by setting started_moving=1), we should only update g.t0 here. Meaning
  // that the motion is only actually initiated here, skipping all the potential delays (especially when using
  // SAVE_ENERGY).
  if (g.started_moving == 1)
  {
    g.started_moving = 0;
    g.moving = 1;
    g.t0 = g.t;
    // We skip this loop, as no point solving the equation of motion for the t=t0 point (dt=0)
    return;
  }

  // moving=0 means no motion, so simply returning:
  if (g.moving == 0)
    return;


  ////////   PART 1: estimating the current position, pos (solving the equation of motion)

  // Time in microseconds since the last accel change:
  dt = g.t - g.t0;
  // Storing the current accel value:
  new_accel = g.accel;
  instant_stop = 0;

  if (g.accel != 0)
    // Accelerating/decelerating cases
  {
    // Change of speed (assuming accel hasn't changed from t0 to t);
    // can be negative or positive:
    dV = g.accel_v[2 + g.accel] * (float)dt;

    // Current speed (can be positive or negative):
    g.speed = g.speed0 + dV;

    // If going beyond the target speed, stop accelerating:
    if ((g.accel > 0 && g.speed >= g.speed1) || (g.accel < 0 && g.speed <= g.speed1))
    {
      i_case = 1;
      // t_a : time in the past (between t0 and t) when acceleration should have changed to 0, to prevent going beyong the target speed
      // dt_a = t_a-t0; should be >0, and <dt:
      dt_a = (g.speed1 - g.speed0) / g.accel_v[2 + g.accel];
      // Current position has two components: first one (from t0 to t_a) is still accelerated,
      // second one (t_a ... t) has accel=0:
      g.pos = g.pos0 + (float)dt_a * (g.speed0 + 0.5 * g.accel_v[2 + g.accel] * (float)dt_a) + g.speed1 * (float)(dt - dt_a);
      g.speed = g.speed1;
      new_accel = 0;
      // If the target speed was zero, stop now
      if (fabs(g.speed1) < SPEED_TINY)
      {
        // At this point we stopped, so no need to revisit the motor_control module
        instant_stop = 1;
        stop_now();
      }
    }
    else
    {
      i_case = 2;
      // Current position when accel !=0 :
      g.pos = g.pos0 +  (float)dt * (g.speed0 + 0.5 * dV );
    }
  }
  else
  {
    i_case = 3;
    // Current position when accel=0
    g.pos = g.pos0 +  (float)dt * g.speed0;
  }

  //////////  PART 2: Estimating if we need to make a step, and making the step if needed


  // Integer position (in microsteps):
  COORD_TYPE pos_short = floorMy(g.pos);

  // If speed changed the sign since the last step, change motor direction:
  if (g.speed > 0.0 && g.speed_old <= 0.0)
  {
#ifndef DISABLE_MOTOR  
    digitalWrite(PIN_DIR, g.straight);
#endif    
    delayMicroseconds(STEP_LOW_DT);
  }
  else if (g.speed < 0.0 && g.speed_old >= 0.0)
  {
#ifndef DISABLE_MOTOR  
    digitalWrite(PIN_DIR, 1-g.straight);
#endif    
    delayMicroseconds(STEP_LOW_DT);
  }

  // If the pos_short changed since the last step, do another step
  // This implicitely assumes that Arduino loop is shorter than the time interval between microsteps at the largest allowed speed
  if (pos_short != g.pos_short_old)
  {
    // One microstep (driver direction pin should have been written to elsewhere):
#ifndef DISABLE_MOTOR  
    digitalWrite(PIN_STEP, LOW);
#endif    
    // For Easydriver, the delay should be at least 1.0 us:
    delayMicroseconds(STEP_LOW_DT);
#ifndef DISABLE_MOTOR  
    digitalWrite(PIN_STEP, HIGH);
#endif    

    // How many steps we'd need to take at this call:
    // If it is > 1, we've got a problem (skipped steps), potential solution is below, in PRECISE_STEPPING module
    COORD_TYPE d = abs(pos_short - g.pos_short_old);

#ifdef PRECISE_STEPPING               //  Precise stepping module
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Fixing the rare occasions of a skipped motor step, by adjusting the time delay constant (g.dt_backlash) to the point
    // when we are back in the past around the time the correct step should have been taken.
    // This is just a fix, not a good solution if your SPEED_LIMIT is so high that the Arduino loop becomes
    // comparable or longer than the time interval between motor steps at the highest speed allowed.
    // Do some TIMING tests to figure out the timings, if you use parts with different specs (motor, rail, LCD, keypad).
    // In my setup, average Arduino loop length is 250 us when moving, or
    // about 50% of the microstep interval when moving at the maximum (5 mm/s) speed; the longest loops are
    // around 120%, but my rail skips only a couple of steps per 10,000 steps on average. This is easily
    // fixable (see below). If you get a sizable fraction (say, more than 5 percent) of the steps skipped,
    // you need to lower down your SPEED_LIMIT. For an arbitrary rail and motor, make sure the following condition is met:
    // 10^6 * MM_PER_ROTATION / (MOTOR_STEPS * N_MICROSTEPS * SPEED_LIMIT_MM_S) >~ 500 microseconds
    char d_sign;
    if (d > 1)
    {
      // The single step with a corresponding sign which should have been taken
      if (pos_short > g.pos_short_old)
        d_sign = 1;
      else
        d_sign = -1;

      // Time correction depends on the travel history between t_old and now
      short dt1_backlash = 0;
      float pos_a;
      COORD_TYPE pos_short_new = g.pos_short_old + d_sign;
      float pos_new = (float)pos_short_new;
      byte solve_square_equation = 0;
      switch (i_case)
      {
        case 1: // The most difficult case when acceleration changed to zero since t_old, when we hit the target speed
          // Coordinate corresponding to t_a (when accel changed to zero; in the past; should be between g.pos_old and g.pos):
          pos_a = g.pos0 + (float)dt_a * (g.speed0 + 0.5 * g.accel_v[2 + g.accel] * (float)dt_a);
          // Two subcases
          if ((pos_new >= pos_a && pos_new <= g.pos) || (pos_new <= pos_a && pos_new >= g.pos))
            // First subcase: the single step should have happened during the latter (accel=0) part of the time interval since t_old
          {
            if (g.speed1 != 0.0)
              dt1_backlash = dt - dt_a - (pos_new - pos_a) / g.speed1;
          }
          else if ((pos_new >= g.pos_old && pos_new < pos_a) || (pos_new <= g.pos_old && pos_new > pos_a))
            // Second subcase: the step should have happened in the first (accel!=0) part of the time interval since t_old
          {
            solve_square_equation = 1;
          }
          break;

        case 2: // The intermediate difficulty case when the acceleration was constant since t0
          solve_square_equation = 1;
          break;

        case 3: // The simplest case when we had zero acceleration since t0
          if (g.speed0 != 0.0)
            dt1_backlash = dt - (pos_new - g.pos0) / g.speed0;
          break;
      }

      if (solve_square_equation)
      {
        float D2;
        // We have to solve a square equation to recover the time from coordinate
        float D = g.speed0 * g.speed0 - 2.0 * g.accel_v[2 + g.accel] * (g.pos0 - pos_new);
        // Checking if there is at least one real solution:
        if (D >= 0.0)
        {
          D2 = sqrt(D);
          // Two possible solutions:
          float dt1 = (-g.speed0 - D2) / (g.accel_v[2 + g.accel]);
          float dt2 = (-g.speed0 + D2) / (g.accel_v[2 + g.accel]);
          // Picking the right solution (if any):
          if (dt - dt1 > 0 && dt - dt1 < g.t - g.t_old)
            dt1_backlash = dt - dt1;
          else if (dt - dt2 > 0 && dt - dt2 < g.t - g.t_old)
            dt1_backlash = dt - dt2;
        }
      }

      // Sanity checks:
      // The single step event should have happened somewhere between t_old and t:
      if (dt1_backlash > 0 && dt1_backlash < g.t - g.t_old)
      {
        // Moving back in time:
        g.t = g.t - dt1_backlash;
        dt = dt - dt1_backlash;
        // Time lag correction is cumulative for the current travel (gets reset to 0 when reaching stop_now):
        g.dt_backlash = g.dt_backlash + dt1_backlash;
        // Now the current position only differs from pos_short_old by a single step:
        pos_short = pos_short_new;
        g.pos = pos_new;
        d = 1;
      }

    }  // if (d > 1)
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#endif // PRECISE_STEPPING

    // Measuring backlash (it gets larger as we move in the bad - negative - direction,
    // and gets smaller as we move in the good - positive - direction):
    g.BL_counter = g.BL_counter + (g.pos_short_old - pos_short);
    // Backlash cannot be negative:
    if (g.BL_counter < (COORD_TYPE)0)
      g.BL_counter = 0;
    // and cannot be larger than g.backlash:
    if (g.BL_counter > g.backlash)
      g.BL_counter = g.backlash;

    // Saving the current position as old:
    g.pos_short_old = pos_short;
    g.pos_old = g.pos;
    // Old speed (to use to detect when the direction has to change):
    g.speed_old = g.speed;
  }  // if (pos_short != g.pos_short_old)

  if (g.moving_mode == 1)
    // Used in go_to mode
  {
    // For small enough speed, we stop instantly when reaching the target location (or overshoot the precise location):
    if ((g.speed1 >= 0.0 && g.speed >= 0.0 && g.pos >= g.pos_goto || g.speed1 <= 0.0 && g.speed <= 0.0 && g.pos <= g.pos_goto))
      // Just a hack for now (to fix a rare bug when rail keeps moving and not stopping)
      //        && fabs(g.speed) < SPEED_SMALL + SPEED_TINY)
    {
      new_accel = 0;
      instant_stop = 1;
      stop_now();
    }

    if (instant_stop == 0)
    {
      // Final position  if a full break were enabled now:
      // Breaking is always done at maximum deceleration
      if (g.speed >= 0.0)
        //The additional -/+1.0 factor is to make the rail stop 1 step later on average, to deal with round-off errors
        g.pos_stop = g.pos - 1.0 + 0.5 * (g.speed * g.speed) / ACCEL_LIMIT;
      else
        g.pos_stop = g.pos + 1.0 - 0.5 * (g.speed * g.speed) / ACCEL_LIMIT;

      // Checking if pos_goto is bracketed between pos_stop_old and pos_stop (not checked first time):
      if (g.pos_stop_flag == 1 && ((g.pos_goto > g.pos_stop && g.pos_goto < g.pos_stop_old) || (g.pos_goto < g.pos_stop && g.pos_goto > g.pos_stop_old)))
        // Time to break happened between the previous and current motor_control calls
        // If we initiate breaking now, we'll always slightly overshoot the target position (so the previous part
        // with the instant stop when speed is very small makes sense)
      {
        // Initiating breaking at maximum (2) acceleration index:
        if (g.speed >= 0.0)
          new_accel = -2;
        else
          new_accel = 2;
        g.speed1 = 0.0;
      }
      g.pos_stop_old = g.pos_stop;
      g.pos_stop_flag = 1;
    }
  }


  //////////  PART 3: Finalizing

  // If accel was modified here, update pos0, t0 to the current ones:
  if (new_accel != g.accel || instant_stop == 1)
  {
    g.t0 = g.t;
    g.pos0 = g.pos;
    g.speed0 = g.speed;
    g.accel = new_accel;
  }


  return;
}



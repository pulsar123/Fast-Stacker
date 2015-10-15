//#include <Arduino.h>
//#include "stacker.h"

void motor_control()
/* Controlling the stepper motor, based on current time, target speed (speed1), acceleration (accel),
   values at the last accel change (t0, pos0, speed0), and old integer position pos_old_short.

   Important: g.moving can be set to zero only here (by calling stop_now())! Also, it should be set to 1 only outside of this function.
 */
{
  unsigned long dt, dt_a;
  float dV;
  short new_accel, instant_stop;

  // Current time in microseconds:
  g.t = micros();

  // moving=0 means no motion, so simply returning:
  if (g.moving == 0)
    return;

  ////////   PART 1: estimating the current position, pos

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
    dV = (float)g.accel * ACCEL_LIMIT * (float)dt;

    // Current speed (can be positive or negative):
    g.speed = g.speed0 + dV;

    // If going beyond the target speed, stop accelerating:
    if ((g.accel == 1 && g.speed >= g.speed1) || (g.accel == -1 && g.speed <= g.speed1))
    {
      // t_a : time in the past (between t0 and t) when acceleration should have changed to 0, to prevent going beyong the target speed
      // dt_a = t_a-t0; should be >0, and <dt:
      dt_a = (float)g.accel * (g.speed1 - g.speed0) / ACCEL_LIMIT;
      // Current position has two components: first one (from t0 to t_a) is still accelerated,
      // second one (t_a ... t) has accel=0:
      g.pos = g.pos0 + (float)dt_a * (g.speed0 + 0.5 * (float)g.accel * ACCEL_LIMIT * (float)dt_a) + g.speed1 * (float)(dt - dt_a);
      g.speed = g.speed1;
      new_accel = 0;
      // If the target speed was zero, stop now
      if (fabs(g.speed1) < SPEED_TINY)
      {
        // At this point we stopped, so no need to revisit the motor_control module
        stop_now();
      }
    }
    else
    {
      // Current position when accel !=0 :
      g.pos = g.pos0 +  (float)dt * (g.speed0 + 0.5 * dV );
    }
  }
  else
  {
    // Current position when accel=0
    g.pos = g.pos0 +  (float)dt * g.speed0;
  }

  //////////  PART 2: Estimating if we need to make a step, and making the step if needed


  // Integer position (in microsteps):
  short pos_short = floorMy(g.pos);

  // If speed changed the sign since the last step, change motor direction:
  if (g.speed > 0.0 && g.speed_old <= 0.0)
  {
#ifndef DEBUG
    digitalWrite(PIN_DIR, HIGH);
#endif
    delayMicroseconds(STEP_LOW_DT);
  }
  else if (g.speed < 0.0 && g.speed_old >= 0.0)
  {
#ifndef DEBUG
    digitalWrite(PIN_DIR, LOW);
#endif
    delayMicroseconds(STEP_LOW_DT);
  }

  // If the pos_short changed since the last step, do another step
  // This implicitely assumes that there are more than one arduino loops per step; if not, perhaps
  // I should allow more than one step here, in rapid sequence? This can result in high accelerations though
  if (pos_short != g.pos_short_old)
  {
    // One microstep (driver direction pin should have been written to elsewhere):
#ifndef DEBUG
    digitalWrite(PIN_STEP, LOW);
#endif
    // For Easydriver, the delay should be at least 1.0 us:
    delayMicroseconds(STEP_LOW_DT);
#ifndef DEBUG
    digitalWrite(PIN_STEP, HIGH);
#endif

    // Saving the current position as old:
    g.pos_short_old = pos_short;
    // Old speed (to use to detect when the dirtection has to change):
    g.speed_old = g.speed;
  }

  if (g.moving_mode == 1)
    // Used in go_to mode
  {
    // For small enough speed, we stop instantly when reaching the target location (or overshoot the precise location):
    if ((g.speed1 >= 0.0 && g.speed >= 0.0 && g.pos >= g.pos_goto || g.speed1 <= 0.0 && g.speed <= 0.0 && g.pos <= g.pos_goto)
        && fabs(g.speed) < SPEED_SMALL + SPEED_TINY)
    {
      new_accel = 0;
      instant_stop = 1;
      stop_now();
    }

    if (instant_stop == 0)
    {
      // Final position  if a full break were enabled now:
      if (g.speed >= 0.0)
        g.pos_stop = g.pos + 0.5 * (g.speed * g.speed) / ACCEL_LIMIT;
      else
        g.pos_stop = g.pos - 0.5 * (g.speed * g.speed) / ACCEL_LIMIT;

      // Checking if pos_goto is bracketed between pos_stop_old and pos_stop (not checked first time):
      if (g.pos_stop_flag == 1 && ((g.pos_goto > g.pos_stop && g.pos_goto < g.pos_stop_old) || (g.pos_goto < g.pos_stop && g.pos_goto > g.pos_stop_old)))
        // Time to break happened between the previous and current motor_control calls
        // If we initiate breaking now, we'll always slightly overshoot the target position (so the previous part
        // with the instant stop when speed is very small makes sense)
      {
        // Initiating breaking:
        if (g.speed >= 0.0)
          new_accel = -1;
        else
          new_accel = 1;
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



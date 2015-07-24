void limiters()
/* Processing inputs from the two limiting switches, plus all the relevant calculations.

 */
{
  short dx, dx_break;

  // Nothing to do if not moving (the special case of limiters enabled at power up time
  // is handled elsewhere - in setup() etc.
  if (direction == 0)
    return;

  // Priority action: read the input from the limiting switches:
  limit_on = digitalRead(PIN_LIMITERS);

  //////// Hard limits //////
  // If a limiter is on:
  if (limit_on == HIGH)
    // Triggering the limiter is an exceptional event, should rarely happen, and will
    // necessitate re-calibration of the rail
  {
    // Emergency breaking (cannot be interrupted):
    // The breaking flag should be read in keypad module, and override keypad actions if =1
    breaking = 1;
    // No need to initiate break if we are already decelerating:
    if (accel != -1)
    {
      accel = -1;
      t0 = t;
      pos0 = pos;
      speed0 = speed;
      speed1 = 0.0;
    }
    // Requesting immediate (after safely breaking the rail) calibration:
    if (direction == -1)
      // We hit the foreground switch, so only the background one remains to be calibrated:
      calibrate = 2;
    else
      // We hit the background switch, so only the foreground one remains to be calibrated:
      calibrate = 1;
    calibrate_init = calibrate;
    // Memorizing the new limit for the current switch; this should be stored in EEPROM later, when direction=0
    limit_tmp = pos_short_old;
  }
  else
    ////// Soft limits ///////
  {
    // No soft limits enforced when doing calibration:
    if (calibrate_init == 0)
    {
      // Checking how far we are from a limiter in the direction we are moving
      if (direction == -1)
        dx = pos_short_old - limit1;
      else
        dx = limit2 - pos_short_old;
      if (dx <= roundMy(BREAKING_DISTANCE))
      {
        // Breaking distance at the current speed:
        dx_break = roundMy(0.5 * speed * speed / ACCEL_LIMIT);
        if (dx <= dx_break)
          // Emergency breaking, to avoid hitting the limiting switch
        {
          // The breaking flag should be read in keypad module, and override keypad actions if =1
          breaking = 1;
          // No need to break if we are already decelerating:
          if (accel != -1)
          {
            accel = -1;
            t0 = t;
            pos0 = pos;
            speed0 = speed;
            speed1 = 0.0;
          }
        }
      }
    }
  }

  return;
}



void limiters()
/* Processing inputs from the two limiting switches, plus all the relevant calculations.

 */
{
  short dx, dx_break;

  // Priority action: read the input from the limiting switches:
  limit_on = digitalRead(PIN_LIMITERS);

  //////// Hard limits //////
  // If a limiter is on:
  if (limit_on == HIGH)
    // Triggering the limiter is an exceptional event, should rarely happen, and will
    // necessitate re-calibration of the rail
  {
    // Emergency breaking (cannot be interrupted):
    // The breaking flag should be read in change_speed
    change_speed(0.0,0);
    // This should be after change_speed(0.0):
    breaking = 1;
    // Requesting immediate (after safely breaking the rail) calibration:
    if (speed < -SMALL)
      // We hit the foreground switch, so only the background one remains to be calibrated:
      calibrate = 2;
    else if (speed > SMALL) 
      // We hit the background switch, so only the foreground one remains to be calibrated:
      calibrate = 1;
    else
    // Speed was 0 when a limiter was triggered; normally shouldn't happen; just in case calibrating both limiters:
      calibrate = 3;
    calibrate_init = calibrate;
    // Memorizing the new limit for the current switch; this should be stored in EEPROM later, when moving=0
    limit_tmp = pos_short_old;
    // Only initial limiter initates calibration; second time we hit it is a part of automatically initiated calibration:
    if (calibrate_flag == 0)
      calibrate_flag = 1;
  }
  else
    ////// Soft limits ///////
  {
    // No soft limits enforced when doing calibration:
    if (calibrate_init == 0)
    {
      // Checking how far we are from a limiter in the direction we are moving
      if (speed < 0.0)
        dx = pos_short_old - limit1;
      else
        dx = limit2 - pos_short_old;
      // Preliminary test (for highest possible speed):
      if (dx <= roundMy(BREAKING_DISTANCE))
      {
        // Breaking distance at the current speed:
        dx_break = roundMy(0.5 * speed * speed / ACCEL_LIMIT);
        // Accurate test (for the current speed):
        if (dx <= dx_break)
          // Emergency breaking, to avoid hitting the limiting switch
        {
          change_speed(0.0,0);
          breaking = 1;          
        }
      }
    }
  }

  return;
}



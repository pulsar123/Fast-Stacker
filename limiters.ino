void limiters()
/* Processing inputs from the two limiting switches, plus all the relevant calculations.

 */
{
  short dx, dx_break;

  // Priority action: read the input from the limiting switches:
  g.limit_on = digitalRead(PIN_LIMITERS);

  //////// Hard limits //////
  // If a limiter is on:
  if (g.limit_on == HIGH)
    // Triggering the limiter is an exceptional event, should rarely happen, and will
    // necessitate re-calibration of the rail
  {
    // Emergency breaking (cannot be interrupted):
    // The breaking flag should be read in change_speed
    change_speed(0.0, 0);
    // No more stacking if we hit a limiter:
    g.stacker_mode = 0;
    // This should be after change_speed(0.0):
    g.breaking = 1;
    letter_status("B");
    display_comment_line("Hit a limiter ");
    // Requesting immediate (after safely breaking the rail) calibration:
    if (g.speed < -SPEED_TINY)
      // We hit the foreground switch, so only the background one remains to be calibrated:
      g.calibrate = 2;
    else if (g.speed > SPEED_TINY)
      // We hit the background switch, so only the foreground one remains to be calibrated:
      g.calibrate = 1;
    else
      // Speed was 0 when a limiter was triggered; normally shouldn't happen; just in case calibrating both limiters:
      g.calibrate = 3;
    g.calibrate_init = g.calibrate;
    // Memorizing the new limit for the current switch; this should be stored in EEPROM later, when moving=0
    g.limit_tmp = g.pos_short_old;
    // Only initial limiter initates calibration; second time we hit it is a part of automatically initiated calibration:
    if (g.calibrate_flag == 0)
      g.calibrate_flag = 1;
  }
  else
    ////// Soft limits ///////
  {
    // No soft limits enforced when doing calibration:
    if (g.calibrate_init == 0)
    {
      // Checking how far we are from a limiter in the direction we are moving
      if (g.speed < 0.0)
        dx = g.pos_short_old - g.limit1;
      else
        dx = g.limit2 - g.pos_short_old;
      // Preliminary test (for highest possible speed):
      if (dx <= roundMy(BREAKING_DISTANCE))
      {
        // Breaking distance at the current speed:
        dx_break = roundMy(0.5 * g.speed * g.speed / ACCEL_LIMIT);
        // Accurate test (for the current speed):
        if (dx <= dx_break)
          // Emergency breaking, to avoid hitting the limiting switch
        {
          change_speed(0.0, 0);
          g.breaking = 1;
          letter_status("B");
        }
      }
    }
  }

  return;
}



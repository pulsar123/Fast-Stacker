void limiters()
/* Processing inputs from the two limiting switches, plus all the relevant calculations.

*/
{
  COORD_TYPE dx, dx_break;

  if (g.moving == 0 || g.uninterrupted == 1 || g.error > 0)
    return;

#ifdef TEST_SWITCH
  if (g.test_flag == 1 && g.test_N == 0)
    return;
#endif

  // If we are moving towards the second limiter (after hitting the first one), don't test for the limiter sensor until we moved DELTA_LIMITER beyond the point where we hit the first limiter:
  // This ensures that we don't accidently measure the original limiter as the second one.
  if (!g.telescope)
    //    if (g.calibrate_flag == 3 && ((g.calibrate == 1 && g.pos_short_old > g.pos_limiter_off - DELTA_LIMITER) || (g.calibrate == 2 && g.pos_short_old < g.pos_limiter_off + DELTA_LIMITER)))
    if (g.calibrate_flag == 3 && g.pos_short_old > g.limit2 - DELTA_LIMITER || g.accident && g.calibrate_flag == 1 && g.pos_short_old < g.limit1 + DELTA_LIMITER )
      return;

  // Read the input from the limiting switches:
#ifdef MOTOR_DEBUG
  g.limit_on = 0;
#else
  g.limit_on = digitalRead(PIN_LIMITERS);
#endif

  //////// Hard limits //////
  // If a limiter is on:
  if (g.limit_on == HIGH)
    // Triggering the limiter is an exceptional event, should rarely happen, and will
    // trigger an automatic re-calibration of the rail
    // This happens by design during the rail calibration.
  {
#ifdef TEST_SWITCH
    // The flag=2 regime (moving in the opposite direction after hitting a limiter followed by emergency breaking): ignoring the limit_on-HIGH state:
    // Same in flag=5 mode (rewinding into safe zone after hitting the second limiter)
    if (g.test_flag != 3 && g.test_flag != 5)
      return;
    else
    {
      if (g.test_limit_on[0] == 0)
      { p
        g.pos_tmp = g.pos;
        g.count[0]++;
        g.test_limit_on[0] = 1;
        change_speed(0.0, 0, 2);
      }
      if (g.test_limit_on[1] == 1)
      {
        g.test_limit_on[1] = 0;
      }
    }
    return;
#endif

    if (g.calibrate_flag == 0)
    {
      // Calibration initiated by hitting limit2 switch by accident:
      if (g.speed > SPEED_TINY)
      {
        g.calibrate_flag = 2;
        g.limit2 = g.pos_short_old;
      }
      else if (g.speed < -SPEED_TINY)
        // Calibration initiated by hitting limit1 switch by accident:
      {
        g.calibrate_flag = 1;
        // Temporary value for limit1 coordinate:
        g.limit1 = g.pos_short_old;
        g.accident = 1;
      }
      else
        return;
      g.error = 4;
    }

    if (g.calibrate_flag == 1 && !g.accident)
      // We just triggered limit2 switch (turned it on for the first time), so immediately updating limit2 (will update EEPROM
      // when stopped after the calibration is done)
    {
      g.calibrate_flag = 2;
      g.limit2 = g.pos_short_old;
    }
    // Moving forward right before calibrating limit1, switch is still on
    if (g.calibrate_flag == 4)
      return;

    // Emergency breaking (cannot be interrupted):
    start_breaking();
    display_comment_line("Hit a limiter ");


    /*
        if (g.calibrate_flag == 0)
        {
          g.calibrate_flag = 1;
          if (g.calibrate == 0)
          {
            if (g.speed < 0.0)
              // We hit the foreground switch, so only the background one remains to be calibrated:
              g.calibrate = 2;
            else
              // We hit the background switch, so only the foreground one remains to be calibrated:
              g.calibrate = 1;
          }
          else
            //  g.calibrate=3
          {
            g.calibrate = 1;
          }

          // No more stacking if we hit a limiter:
          g.stacker_mode = 0;
        }
        else
          // If calibrate_flag = 3
        {
          g.calibrate_flag = 4;
        }
    */

  }
  else


    /////////////////////////////////////////////// Soft limits ///////////////////////////////////////////////////////////////////
#ifdef TEST_SWITCH
  {
    if (g.test_N > 0 && g.test_flag == 1)
    {
      g.pos_tmp2 = g.pos;
      g.test_flag = 5;
    }

    if (g.test_flag == 3 && g.test_limit_on[0] == 1)
    {
      g.test_limit_on[0] = 0;
    }
    if (g.test_flag == 5 && g.test_limit_on[1] == 0)
    {
      g.test_limit_on[1] = 1;
      g.count[1]++;
    }
  }
#else
  {
    if (g.accident && g.calibrate_flag == 1)
      g.accident = 0;
      
    if (g.calibrate_flag == 4)
      // The switch limit1 just went off for the first time when calibrating limit1; will be used to for absolute calibration
    {
      g.calibrate_flag = 5;
      // Making sure only the very first turn of of limit one is registered (and used for calibration); ignoring the subsequent switch noise
      g.uninterrupted = 1;
      // At the end of calibration new coordinates will be derived from old by adding this parameter to the old ones:
      g.coords_change = -g.pos_short_old;
    }

    /*
        // If we are rewinding in the opposite direction after hitting a limiter and breaking, and limiter went off, we record the position:
        if (g.calibrate_flag == 2)
        {
          g.pos_limiter_off = g.pos_short_old;
          // The third leg of the calibration process: starting to send for limiters again, to calibrate the other side
          g.calibrate_flag = 3;
        }

        if (g.calibrate_flag == 6)
          // We are here when at the end of calibrating limit1 we are moving to the safe zone after triggering the foreground switch, and the switch just turned off for the first time
        {
          // We are using the first instance of the limit1 switch turning off as the absolute calibration point:
          g.limit_tmp = g.pos_short_old;
          // Difference between new and old coordinates (to be applied after calibration is done):
          //      g.coords_change = g.limit1 - g.limit_tmp;
          // Assuming limit1 is always set at 0:
          g.coords_change = -g.limit_tmp;
          // Current foreground limit in old coordinates:
          //      g.limit1 = g.limit_tmp;
          // This ensures that limits (hard and soft) will be ignored until we stop:
          g.calibrate_flag = 7;
        }
    */

#ifdef TELE_SWITCH
    // In initial telescope calibration, start breaking once we are past the switch area and achieved maximum speed (accel=0):
    if (g.calibrate == 5 && g.calibrate_flag == 3 && g.accel == 0)
    {
      g.calibrate_flag = 1;
      g.calibrate = 1;
      start_breaking();
    }
#endif

    // No soft limits enforced when doing calibration:
    if (g.calibrate_flag == 0)
    {
      // Checking how far we are from a limiter in the direction we are moving
      // If current speed is non-zero, we use its sign to determine the direction we are moving to
      if (g.speed < -SPEED_TINY || g.speed > SPEED_TINY)
      {
        if (g.speed < 0.0)
          dx = g.pos_short_old - LIMITER_PAD2;
        else
          dx = g.limit2 - g.pos_short_old - LIMITER_PAD2;
      }
      else
        // Otherwise, we use the target direction sign, speed1:
      {
        if (g.speed1 < -SPEED_TINY)
          dx = g.pos_short_old - LIMITER_PAD2;
        else if (g.speed1 > SPEED_TINY)
          dx = g.limit2 - g.pos_short_old - LIMITER_PAD2;
        else
          return;
      }

      // Something went wrong:
      if (dx < 0)
        return;

      // Preliminary test (for highest possible speed):
      if (g.telescope == 0 && dx <= roundMy(BREAKING_DISTANCE) || g.telescope == 1 && dx <= roundMy(BREAKING_DISTANCE_TEL))
      {
        // Breaking distance at the current speed:
        dx_break = roundMy(0.5 * g.speed * g.speed / g.accel_limit);
        // Accurate test (for the current speed):
        if (dx <= dx_break)
          // Emergency breaking, to avoid hitting the limiting switch
          start_breaking();
      }
    }
  }
#endif // TEST_SWITCH  

  return;
}



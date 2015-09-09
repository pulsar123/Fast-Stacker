void calibration()
/* Calibration of either one or both limiting switches - placing soft limits (limi1, limit2) such that the hard
   limits (triggering the limiting switches) normally doesn't occur.

   For example, we end up runnning this if we hit a limiter and just breaked to a complete stop after that.
 */
{

  if (g.calibrate_init == 0 || g.moving == 1 || g.breaking == 1)
    return;


  /*  calibrate=3 is not implemented yet
    if (g.calibrate_init==3  && g.calibrate_flag==0)
    // The case when both switches need to be calibrated:
    {
      // The flag ensures that this if statement is only eneterd once per limiter
      // (should be zeroed at the end of each limiter calibration)
      g.calibrate_flag = 1;
      letter_status("C");
      display_comment_line(" Calibrating  ");
      if (g.calibrate == 3)
        // First we calibrate limit1:
        g.calibrate = 1;
      else if (g.calibrate == 1)
        // Next we calibrate limit2:
        g.calibrate = 2;
      else
      {
        // When both limits are done, clearing the flag and exiting:
        g.calibrate = 0;
        g.calibrate_init = 0;
        g.calibrate_flag = 0;
        letter_status(" ");
        return;
      }
    }
  */

  if (g.calibrate == 1 && g.calibrate_flag == 1)
    // We need to calibrate the foreground switch (limit1)
  {
    if (g.calibrate_init == 1)
      // Calibration triggered by hitting the background limiter
    {
      // Saving the backround limit, minus a constant:
      g.limit2 = g.limit_tmp - LIMITER_PAD;
      EEPROM.put( ADDR_LIMIT2, g.limit2);
    }
    // Moving towards switch 1 for its calibration:
    change_speed(-SPEED_LIMIT, 0);
    // This ensures that any other speed changes requests will be ignored until the calibration leg is over:
    g.calibrate_flag = 2;
    letter_status("C");
  }

  if (g.calibrate == 2 && g.calibrate_flag == 1)
    // We need to calibrate the background switch (limit2)
  {
    if (g.calibrate_init == 2)
      // Calibration triggered by hitting a limiter
    {
      // Saving the foreround limit:
      g.limit1 = g.limit_tmp + LIMITER_PAD;
      EEPROM.put( ADDR_LIMIT1, g.limit1);
    }
    // Moving towards switch 2 for its calibration:
    change_speed(SPEED_LIMIT, 0);
    // This ensures that any other speed changes requests will be ignored until the calibration leg is over:
    g.calibrate_flag = 2;
    letter_status("C");
  }

  // Cleanup after a single limiter (emergency) calibration is done
  if (g.calibrate_init < 3 && g.calibrate_flag == 5)
  {
    if (g.calibrate_init == 2)
    {
      g.limit2 = g.limit_tmp - LIMITER_PAD;
      EEPROM.put( ADDR_LIMIT2, g.limit2);
      // Travelling back into safe area:
      go_to(g.limit2 - DELTA_LIMITER, SPEED_LIMIT);
    }
    else if (g.calibrate_init == 1)
    {
      // Saving the foreround limit:
      g.limit1 = g.limit_tmp + LIMITER_PAD;
      EEPROM.put( ADDR_LIMIT1, g.limit1);
      // Travelling back into safe area:
      go_to(g.limit1 + DELTA_LIMITER, SPEED_LIMIT);
    }
    g.calibrate_init = 0;
    g.calibrate = 0;
    letter_status(" ");
  }

  return;
}



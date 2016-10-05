void calibration()
/* Calibration of either one or both limiting switches - placing soft limits (limit1, limit2) such that the hard
   limits (triggering the limiting switches) normally doesn't occur.

   For example, we end up runnning this if we hit a limiter and just breaked to a complete stop after that.
*/
{
#ifdef TEST_SWITCH
  return;
#endif

  // This module only works when not moving:
  if (g.calibrate_flag == 0 || g.moving == 1 || g.started_moving == 1 || g.backlashing == 1 || g.error > 0)
    return;
#ifndef TELE_SWITCH
  if (g.telescope)
    return;
#endif

  switch (g.calibrate_flag)
  {
    case 1: // Initiating full calibration
      // Moving towards switch 2 for its calibration, with maximum speed and acceleration:
      change_speed(g.speed_limit, 0, 2);
      letter_status("C");
      break;

    case 2: // Triggered limit2 and stopped, will now move towards limit1
      change_speed(-g.speed_limit, 0, 2);
      g.calibrate_flag = 3;
      break;

    case 3: // Triggered limit1 and stopped, will now move forward to calibrate limit1 on the first switch-off position
      go_to(g.pos + 2 * BREAKING_DISTANCE, g.speed_limit);
      g.calibrate_flag = 4;
      break;

    case 5: // End of calibration; updating coordinates
      letter_status(" ");
      g.pos = g.pos + (float)g.coords_change;
      g.pos_short_old = g.pos_short_old + g.coords_change;
      g.t0 = g.t;
      g.pos0 = g.pos;
      g.pos_old = g.pos;
      if (!g.telescope)
      {
        g.limit2 = g.limit2 + g.coords_change;
        EEPROM.put( ADDR_LIMIT2, g.limit2);
        // Saving the current position to EEPROM:
        EEPROM.put( ADDR_POS, g.pos );
      }
      display_all();
      g.calibrate_flag = 0;
      g.accident = 0;
      break;
  }


  /*

    if (g.calibrate == 1 && g.calibrate_flag == 1)
      // We need to calibrate the foreground switch (limit1)
    {
      // Calibration triggered by hitting the background limiter
      // Saving the backround limit, minus a constant:
      if (!g.telescope)
      {
        g.limit2 = g.limit_tmp - LIMITER_PAD;
        EEPROM.put( ADDR_LIMIT2, g.limit2);
      }
      // Moving towards switch 1 for its calibration, with maximum acceleration:
      change_speed(-g.speed_limit, 0, 2);
      // This ensures that any other speed changes requests will be ignored until the calibration leg is over:
      g.calibrate_flag = 2;
      letter_status("C");
    }

    if (g.calibrate == 2 && g.calibrate_flag == 1)
      // We need to calibrate the background switch (limit2)
    {
      // Calibration triggered by hitting the foreground limiter
      // Difference between new and old coordinates (to be applied after calibration is done):
      // (We need this because all rail coordinates are counted from g.limit1)
      g.coords_change = -g.limit_tmp;
      // Current foreground limit in old coordinates:
      //    g.limit1 = g.limit_tmp;
      // Moving towards switch 2 for its calibration:
      change_speed(g.speed_limit, 0, 2);
      // This ensures that any other speed changes requests will be ignored until the calibration leg is over:
      g.calibrate_flag = 2;
      letter_status("C");
    }

    // Cleanup after a single limiter (emergency) calibration is done
    if (g.calibrate < 3 && g.calibrate_flag == 5)
    {
      if (g.calibrate == 2)
      {
        g.limit2 = g.limit_tmp - LIMITER_PAD;
        EEPROM.put( ADDR_LIMIT2, g.limit2);
        // Travelling back into safe area:
        go_to((float)(g.limit2 - DELTA_LIMITER) + 0.5, g.speed_limit);
        g.calibrate = 0;
        letter_status(" ");
      }
      else if (g.calibrate == 1)
      {
        go_to(g.pos + 2 * BREAKING_DISTANCE, g.speed_limit);
        g.calibrate_flag = 6;
      }
      // !!!
      //    g.calibrate = 0;
      //    letter_status(" ");
    }
  */
  return;
}



void calibration()
/* Calibration of either one or both limiting switches - placing soft limits (limit1, limit2) such that the hard
   limits (triggering the limiting switches) normally doesn't occur.

   All opeations here are done only when not moving.

   For example, we end up runnning this if we hit a limiter and just breaked to a complete stop after that.

*/
{
#if defined(TEST_SWITCH)
  return;
#endif

  // This module only works when not moving:
  if (g.calibrate_flag == 0 || g.moving == 1 || g.model_init == 1 || g.backlashing == 1 || g.error > 0)
    return;

  switch (g.calibrate_flag)
  {
    case 1: // Initiating full calibration
      // Moving towards switch 2 for its calibration, with maximum speed and acceleration:
      go_to(g.limit2, g.speed_limit);
      display_all();
      letter_status("C");
      break;

    case 2: // Triggered limit2 and stopped, will now move towards limit1
      go_to(g.limit1, g.speed_limit);
      display_all();
      g.calibrate_flag = 3;
      break;

    case 3: // Triggered limit1 and stopped, will now move forward to calibrate limit1 on the first switch-off position
    // Warning: here we should move far enough, so by the time we stop the limiter has to be off again (the "4" multiplier) !!!
      go_to(g.ipos + 4 * BREAKING_DISTANCE, g.speed_limit);
      display_all();
      g.calibrate_flag = 4;
      break;

    case 5: // End of calibration; updating coordinates
      letter_status(" ");
      g.ipos = g.ipos + g.coords_change;
        g.limit2 = g.limit2 + g.coords_change;
        EEPROM.put( ADDR_LIMIT2, g.limit2);
        // Saving the current position to EEPROM:
        EEPROM.put( ADDR_POS, g.ipos );
      g.calibrate_flag = 0;
      g.accident = 0;
      display_all();
      break;
  }

  return;
}



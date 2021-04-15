void calibration()
/* Calibration of either one or both limiting switches - placing soft limits (limit1, limit2) such that the hard
   limits (triggering the limiting switches) normally doesn't occur.

   For example, we end up runnning this if we hit a limiter and just breaked to a complete stop after that.

   In telescope mode (if TELE_SWITCH is defined), this routine is executed every time at boot time, for absolute calibration of the single switch.
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
      display_all();
      letter_status("C");
      break;

    case 2: // Triggered limit2 and stopped, will now move towards limit1
      change_speed(-g.speed_limit, 0, 2);
      display_all();
      g.calibrate_flag = 3;
      break;

    case 3: // Triggered limit1 and stopped, will now move forward to calibrate limit1 on the first switch-off position
      go_to(g.pos + 2 * BREAKING_DISTANCE, g.speed_limit);
      display_all();
      g.calibrate_flag = 4;
      break;

    case 5: // End of calibration; updating coordinates
      letter_status(" ");
      g.pos = g.pos + (float)g.coords_change;
      g.pos_short_old = g.pos_short_old + g.coords_change;
      g.t0 = g.t;
      g.pos0 = g.pos;
      g.pos_old = g.pos;
      if (g.telescope)
      {
        g.limit2 = (COORD_TYPE)TEL_LENGTH;
      }
      else
      {
        g.limit2 = g.limit2 + g.coords_change;
        EEPROM.put( ADDR_LIMIT2, g.limit2);
        // Saving the current position to EEPROM:
        EEPROM.put( ADDR_POS, g.pos );
        EEPROM.commit();
      }
      g.calibrate_flag = 0;
      g.accident = 0;
      display_all();
      break;

    // Telescope specific mode:
    case 10: // Initiating telescope calibration: moving forward until the switch goes off and the maximum speed is reached (accel=0)
      change_speed(g.speed_limit, 0, 2);
      display_all();
      break;
  }

  return;
}



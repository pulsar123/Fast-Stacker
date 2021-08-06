/* Backlash compensation routines.
*/

void backlash()
/*  Monitors the value of the backlash counter BL_counter (updated in motor_control). When it detects that
    BL_counter > 0 (and only when the rail is at rest) it initiates a go_to travel to compensate for backlash.

    This routine should work in concert with go_to() function, where all moves ending in the bad (negative) direction
    should always travel g.backlash further back. The backlash() picks up where go_to() left, after the rail stops, and rewinds
    the rail back in the good direction, to completely compensate the backlash.
*/
{
#if defined(TEST_SWITCH)
  return;
#endif

  // Should not be moving or breaking, and should need >0 backlash compensation:
  // Don't do anything while calibrating.
  if (g.calibrate_flag || g.uninterrupted || g.moving || g.Backlashing || g.BL_counter == 0)
    return;

  if (g.reg.backlash_on*g.BL_counter > 0)
  {
    // Backlash compensation.
    go_to(g.ipos + g.BL_counter, SPEED_LIMIT);

    // This should be done after go_to call:
    g.Backlashing = 1;
  }

  // This is the special case, only used for the final move of backlash compensation after reversing the rail direction (*1):
  if (g.Backlash_init == 3)
  {
    // Backlash compensation.
    go_to(g.ipos + g.reg.backlash_on*BACKLASH_2, SPEED_LIMIT);

    // This should be done after go_to call:
    g.Backlashing = 1;
    g.Backlash_init = 0;
  }

  return;
}


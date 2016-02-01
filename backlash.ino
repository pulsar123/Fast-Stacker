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

  // Should not be moving or breaking, and should need >0 backlash compensation:
  // Don't do anything while calibrating.
  if (g.calibrate || g.breaking || g.moving || g.backlashing || g.BL_counter == (COORD_TYPE)0)
    return;

  if (g.backlash_init == 0)
  {
    // Backlash compensation.
    go_to(g.pos + (float)g.BL_counter, g.speed_limit);

    // This should be done after go_to call:
    g.backlashing = 1;
  }

  // This is the special case, only used for the final move of backlash compensation after reversing the rail direction (*1):
  if (g.backlash_init == 3)
  {
    // Backlash compensation.
    go_to(g.pos + (float)BACKLASH_2, g.speed_limit);

    // This should be done after go_to call:
    g.backlashing = 1;
    g.backlash_init = 0;
  }

  // First move - only done once, initiated in the very first loop, and only if it's not calibrating, breaking, or moving:
  if (g.backlash_init > 0)
  {
    // First move (only when the rail is powered on), before backlash compensation, in the bad direction:
    // Go_to the current position, with BL>0, will result in a full backlash compensation
    go_to(g.pos, g.speed_limit);

    // This should be done after go_to call:
    g.backlashing = 1;
    if (g.backlash_init == 1)
      g.backlash_init = 0;
    else
      g.backlash_init = 3;
  }


  return;
}


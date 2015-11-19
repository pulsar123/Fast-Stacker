/* Backlash compensation routines.
 */

void backlash()
/*  Monitors the value of the backlash counter BL_counter (updated in motor_control). When it detects that
    BL_counter > 0 (and only when the rail is at rest) it initiates a go_to travel to compensate for backlash. This go_to cannot be interrupted
    (it is treated same way as emergency breaking).

    This routine should work in concert with go_to() function, where all moves ending in the bad (negative) direction
    should always travel BACKLASH further back. The backlash() picks up where go_to() left, after the rail stops, and rewinds
    the rail back in the good direction, to completely compensate the backlash.
 */
{

  // First move - only done once, initiated in the verfy first loop, and only if it's not calibrating, breaking, or moving:
  if (g.first_loop == 1 && g.calibrate == 0 && g.breaking == 0 && g.moving == 0 && g.backlashing == 0 && g.BL_counter > 0)
  {
    // First move (only when the rails is powered on), before backlash compensation, in the bad direction:
    go_to(g.pos - (float)g.BL_counter, SPEED_LIMIT);

    // This is to prevent any interrupting (except for emergency breaking) of the backlash compensation travel.
    // This should be done after go_to call:
    g.backlashing = 1;
  }


  // Should not be moving or breaking, and should need >0 backlash compensation:
  // Don't do anything while calibrating.
  if (g.first_loop == 0 && g.calibrate == 0 && g.breaking == 0 && g.moving == 0 && g.backlashing == 0 && g.BL_counter > 0)
  {
    // Backlash compensation. It's always done at the maximum acceletarion/speed
    go_to(g.pos + (float)g.BL_counter, SPEED_LIMIT);

    // This is to prevent any interrupting of the backlash compensation travel.
    // This should be done after go_to call:
    g.backlashing = 1;
  }

  return;
}


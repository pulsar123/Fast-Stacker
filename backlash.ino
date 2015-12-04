/* Backlash compensation routines.
 */

void backlash()
/*  Monitors the value of the backlash counter BL_counter (updated in motor_control). When it detects that
    BL_counter > 0 (and only when the rail is at rest) it initiates a go_to travel to compensate for backlash.

    This routine should work in concert with go_to() function, where all moves ending in the bad (negative) direction
    should always travel BACKLASH further back. The backlash() picks up where go_to() left, after the rail stops, and rewinds
    the rail back in the good direction, to completely compensate the backlash.

    It also works with the rewind function (key "1"); in this case, unlike go_to case, partial backlash compensation
    (as recorded by the variable g.BL_counter) is allowed. Meaning that for small rewinds (travelling less than BACKLASH
    distance) the compensating go_to initiated here will be smaller than BACKLASH, whcih should make it more convenient
    for fine-tuning fore/background points. A drawback: this relies on backlash known precisely. For example, it will start
    producing some artifacts (overcompensating) when the rail is pointed at an angle (so the gravity will start shrinking
    the effect of backlash).
 */
{

/*
  if (g.first_loop == 1)
  {
      lcd.setCursor(0, 0);  lcd.print(g.BL_counter);lcd.print(g.calibrate);lcd.print(g.breaking);lcd.print(g.moving);lcd.print(g.backlashing);
      delay(10000);
  }
*/
  
  // Should not be moving or breaking, and should need >0 backlash compensation:
  // Don't do anything while calibrating.
  if (g.calibrate || g.breaking || g.moving || g.backlashing || g.BL_counter == 0)
    return;

  // First move - only done once, initiated in the very first loop, and only if it's not calibrating, breaking, or moving:
  if (g.first_loop == 1)
  {
    // First move (only when the rail is powered on), before backlash compensation, in the bad direction:
    //!!!
    // Go_to the current position, with BL>0, will result in a full backlash compensation
    go_to(g.pos, SPEED_LIMIT, 2);

    // This should be done after go_to call:
    g.backlashing = 1;
  }


  if (g.first_loop == 0)
  {
    // Backlash compensation. It's always done at the maximum acceletarion/speed
    go_to(g.pos + (float)g.BL_counter, SPEED_LIMIT, 2);

    // This should be done after go_to call:
    g.backlashing = 1;
  }

  return;
}


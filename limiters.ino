void limiters()
/* Processing inputs from the two limiting switches, plus all the relevant calculations.
 *  
 *  Only processed when moving.

*/
{
  COORD_TYPE dx, dx_break;

#ifdef TEST_LIMITER
// Detecting false readings from the limiting switch:
  Read_limiters();
  if (g.limit_on != g.limiter_ini)
  {
    g.limiter_i++;
    display_current_position();
  }
#endif


  if (g.moving == 0 || g.uninterrupted == 1 || g.error > 0 || g.uninterrupted2 == 1)
    return;

#ifdef TEST_SWITCH
  if (g.test_flag == 1 && g.test_N == 0)
    return;
#else
  // If we are moving towards the second limiter (after hitting the first one), don't test for the limiter sensor until we moved DELTA_LIMITER beyond the point where we hit the first limiter:
  // This ensures that we don't accidently measure the original limiter as the second one.
  if (g.calibrate_flag == 3 && g.pos_int_old > g.limit2 - DELTA_LIMITER || g.accident && g.calibrate_flag == 1 && g.pos_int_old < g.limit1 + DELTA_LIMITER )
    return;
#endif

  // Assigning the limiters' state to g.limit_on:
  Read_limiters();


  /////////////////////////////////////////////// Hard limits ///////////////////////////////////////////////////////////////////
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
      if (g.test_flag == 3 && g.test_limit_on[0] == 0)
      {
        g.count[0]++;
        g.test_limit_on[0] = 1;
        if (g.on_init == 0)
        {
          // Memoryzing the very first switch-on position:
          g.pos_tmp = g.pos;
          change_speed(0.0, 0, 2);
          g.on_init = 1;
        }
      }
      if (g.test_flag == 5 && g.test_limit_on[1] == 1)
      {
        g.test_limit_on[1] = 0;
      }
    }
    return;
#endif

    // Accidental limiter triggering module:
    if (g.calibrate_flag == 0)
    {
      // Hitting telescope limiter by accident initiates calibration:
      if (g.telescope)
        g.calibrate_flag = 10;
      // Calibration initiated by hitting limit2 switch by accident:
      else if (g.speed > SPEED_TINY)
      {
        g.calibrate_flag = 2;
        g.limit2 = g.pos_int_old;
      }
      else if (g.speed < -SPEED_TINY)
        // Calibration initiated by hitting limit1 switch by accident:
      {
        g.calibrate_flag = 1;
        // Temporary value for limit1 coordinate:
        g.limit1 = g.pos_int_old;
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
      g.limit2 = g.pos_int_old;
    }

    // Moving forward right before calibrating limit1, switch is still on
    if (g.calibrate_flag == 4 || !g.error && g.calibrate_flag == 10)
      return;

    // Emergency breaking (cannot be interrupted):
    start_breaking();
    display_comment_line("   Hit a limiter    "); // Expensive 
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
      g.coords_change = -g.pos_int_old;
    }

    // While calibration telescope, during the first move (away from the telescope) switch is off and we reached the maximum speed, so we start breaking
    if (g.calibrate_flag == 10 && g.accel == 0)
    {
      // Initiating limit1 calibration loop:
      g.calibrate_flag = 2;
      // Preliminary value for limit2, just in case:
      g.limit2 = g.pos_int_old + (COORD_TYPE)TEL_LENGTH;
      start_breaking();
    }

    // No soft limits enforced when doing calibration:
    // Because limit1 is calibrated when moving in good (backlash-compensated direction) at the moment switch goes off
    // for the first time, when approaching limit1 (moving in the bad direction), backlash playes a positive role - as an extra
    // safety margin. (If switch has zero internal hysteresis, backlash causes the soft limit to be reached g.backlash microsteps
    // further from the switch.)
    if (g.calibrate_flag == 0)
    {
      // Checking how far we are from a limiter in the direction we are moving
      // If current speed is non-zero, we use its sign to determine the direction we are moving to      
      if (g.speed < -SPEED_TINY || g.speed > SPEED_TINY)
      {
        if (g.speed < 0.0)
          dx = g.pos_int_old - LIMITER_PAD2;
        else
          dx = g.limit2 - g.pos_int_old - LIMITER_PAD2;
      }
      else
        // Otherwise, we use the target direction sign, speed1:
      {
        if (g.speed1 < -SPEED_TINY)
          dx = g.pos_int_old - LIMITER_PAD2;
        else if (g.speed1 > SPEED_TINY)
          dx = g.limit2 - g.pos_int_old - LIMITER_PAD2;
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



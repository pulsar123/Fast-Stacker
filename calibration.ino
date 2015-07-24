void calibration()
/* Calibration of either one or both limiting switches - placing soft limits (limi1, limit2) such that the hard
   limits (triggering the limiting switches) normally doesn't occur.
 */
{

  // Ensuring that the lines before are computed once per switch calibration:
  if (calibrate_flag == 1)
    return;

  if (calibrate_init == 3)
    // The case when both switches need to be calibrated:
  {
    if (calibrate == 3)
      // First we calibrate limit1:
      calibrate = 1;
    else if (calibrate == 1)
      // Next we calibrate limit2:
      calibrate = 2;
    else
    {
      // When both limits are done, clearing the flag and exiting:
      calibrate = 0;
      calibrate_init = 0;
      return;
    }
  }

  if (calibrate == 1)
    // We need to calibrate the foreground switch (limit1)
  {
    if (calibrate_init == 1)
      // Calibration triggered by hitting a limiter
    {
      // Saving the backround limit:
      limit2 = limit_tmp;
      EEPROM.put( ADDR_LIMIT2, limit2);
    }
    // Moving towards switch 1 for its calibration:
    direction = -1;
    accel = 1;
    t0 = t;
    pos0 = pos;
    speed0 = 0.0;
    speed1 = SPEED_LIMIT;
  }

  if (calibrate == 2)
    // We need to calibrate the background switch (limit2)
  {
    if (calibrate_init == 2)
      // Calibration triggered by hitting a limiter
    {
      // Saving the foreround limit:
      limit1 = limit_tmp;
      EEPROM.put( ADDR_LIMIT1, limit1);
    }
    // Moving towards switch 2 for its calibration:
    direction = 1;
    accel = 1;
    t0 = t;
    pos0 = pos;
    speed0 = 0.0;
    speed1 = SPEED_LIMIT;
  }

  if (calibrate_flag == 0)
    calibrate_flag = 1;

  return;
}



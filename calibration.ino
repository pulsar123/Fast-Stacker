void calibration()
/* Calibration of either one or both limiting switches - placing soft limits (limi1, limit2) such that the hard
   limits (triggering the limiting switches) normally doesn't occur.
 */
{
  if (calibrate_init==3  && calibrate_flag==0)
  // The case when both switches need to be calibrated:
  {
    // The flag ensures that this if statement is only eneterd once per limiter
    // (should be zeroed at the end of each limiter calibration)
    calibrate_flag = 1;
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
      calibrate_flag = 0;
      return;
    }
  }

  if (calibrate == 1 && calibrate_flag==1)
    // We need to calibrate the foreground switch (limit1)
  {
    if (calibrate_init == 1)
      // Calibration triggered by hitting the background limiter
    {
      // Saving the backround limit, minus a constant:
      limit2 = limit_tmp - LIMITER_PAD;
      EEPROM.put( ADDR_LIMIT2, limit2);
    }
    // Moving towards switch 1 for its calibration:
    change_speed(-SPEED_LIMIT);
    // This ensures that any other speed changes requests will be ignored until the calibration leg is over:
    calibrate_flag = 2;
  }

  if (calibrate == 2 && calibrate_flag==1)
    // We need to calibrate the background switch (limit2)
  {
    if (calibrate_init == 2)
      // Calibration triggered by hitting a limiter
    {
      // Saving the foreround limit:
      limit1 = limit_tmp + LIMITER_PAD;
      EEPROM.put( ADDR_LIMIT1, limit1);
    }
    // Moving towards switch 2 for its calibration:
    change_speed(SPEED_LIMIT);
    // This ensures that any other speed changes requests will be ignored until the calibration leg is over:
    calibrate_flag = 2;    
  }

  // Cleanup after a single limiter (emergency) calibration is done
  if (calibrate_init<3 && calibrate_flag==0)
  {
    calibrate = 0;
    calibrate_flag = 0;
    // !!! We still have to rewind a bit, to move into non-limiter-triggered region
    if (calibrate_init == 1)
    {
      limit2 = limit_tmp - LIMITER_PAD;
      EEPROM.put( ADDR_LIMIT2, limit2);
      // Travelling back into safe area:
      travel_init(limit2-100);
      // We first accelerate to SPEED1=SPEED_LIMIT/sqrt(2)
//      change_speed(-SPEED1);
    }
    else if (calibrate_init == 2)
    {
      // Saving the foreround limit:
      limit1 = limit_tmp + LIMITER_PAD;
      EEPROM.put( ADDR_LIMIT1, limit1);
      // Travelling back into safe area:
      travel_init(limit1+100);
//      change_speed(SPEED1);
    }
    calibrate_init = 0;
  }

  return;
}



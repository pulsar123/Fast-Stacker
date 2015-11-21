short nintMy(float x)
/*
 My version of nint. Float -> short conversion. Valid for positive/negative/zero.
 */
{
  // Rounding x towards 0:
  short x_short = (short)x;
  float frac;

  if (x >= 0.0)
  {
    frac = x - (float)x_short;
  }
  else
  {
    frac = (float)x_short - x;
  }

  if (frac >= 0.5)
  {
    if (x >= 0.0)
      return x_short + 1;
    else
      return x_short - 1;
  }
  else
    return x_short;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


short floorMy(float x)
/* A limited implementation of C function floor - only to convert from float to short.
   Works with positive, negative numbers and 0.
 */
{
  short m = x;
  if (x >= 0.0)
    return m;
  else
    return m - 1;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
short roundMy(float x)
/* Rounding of float numbers, output - short.
   Works with positive, negative numbers and 0.
 */
{
  if (x >= 0.0)
    return (short)(x + 0.5);
  else
    return (short)(x - 0.5);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void change_speed(float speed1_loc, short moving_mode1)
/* Run the function every time you want to change speed. It will figure out required accel based on current speed and speed1,
   and will update t0, speed0, pos0, if accel changed here.
   Inputs:
    - speed1_loc: new target speed.
    When moving_mode1=1, global moving_mode=1 is  enabled (to be used in go_to).
 */
{
  short new_accel;

  // Ignore any speed change requests during emergency breaking  (after hitting a limiter)
  // DOn't forget to reset breaking=0 somewhere!
  if (g.breaking || g.calibrate_flag == 2)
    return;

  g.moving_mode = moving_mode1;

  if (speed1_loc >= g.speed)
    // We have to accelerate
    new_accel = 1;
  else
    // Have to decelerate:
    new_accel = -1;

  if (new_accel != g.accel)
    // Acceleration changed
  {
    g.accel = new_accel;
    // Memorizing the current values for t, speed and pos:
    g.t0 = g.t;
    g.speed0 = g.speed;
    g.pos0 = g.pos;
  }

  if (g.accel != 0 && g.moving == 0 && g.started_moving == 0)
  {
    // Starting moving
    g.started_moving = 1;
    motion_status();
#ifdef SAVE_ENERGY
    digitalWrite(PIN_ENABLE, LOW);
    delay(ENABLE_DELAY_MS);
#endif
  }

  // Updating the target speed:
  g.speed1 = speed1_loc;
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void go_to(float pos1, float speed)
/* Initiating a travel to pos1 at maximum acceleration and given speed (positive number)
 */
{
  float speed1_loc;
  short speed_changes;

  if (g.breaking || g.backlashing)
    return;

  short pos1_short = floorMy(pos1);

  // We are already there, and no need for backlash compensation, so just returning:
  if (g.moving == 0 && pos1_short == g.pos_short_old && g.BL_counter == 0)
    return;

  // The "shortcut" direction - if there was no acceleration limit and no need for backlash compensation:
  if (pos1 > g.pos)
    g.direction = 1;
  else
    g.direction = -1;
  motion_status();

  // Considering separately the cases when we are at rest, and when we are currently moving
  if (g.moving == 0)
    // We are at rest
  {
    if (g.direction > 0)
      // Will be moving in the good (positive) direction (no need for backlash compensation):
    {
      speed1_loc = speed;
    }
    else
      // Will be moving in the bad (negative) direction (have to overshoot, for backlash compensation):
    {
      // Overshooting by BACKLASH microsteps (this will be compensated in backlash() function after we stop):
      pos1 = pos1 - (float)BACKLASH;
      speed1_loc = -speed;
    }
  }

  else
    // We are currently moving
  {
    // Stopping distance in the current direction:
    float dx_stop = g.speed * g.speed / (2.0 * ACCEL_LIMIT);
    // Travel vector (ignoring acceleration limit and backlash compensation):
    float dx_vec = pos1 - g.pos;
    float dx = fabs(dx_vec);
    // Number of whole steps to take if going straight to the target:
    short dx_steps = pos1_short - g.pos_short_old;

    // All the cases when speed sign will change while traveling to the target:
    // When we move in the correct direction, but cannot stop in time because of the acceleration limit
    if (dx < dx_stop && (g.direction > 0 && g.speed > 0.0 || g.direction < 0 && g.speed <= 0.0) ||
        // When we are moving in the wrong direction
        g.direction > 0 && g.speed <= 0.0 || g.direction < 0 && g.speed > 0.0)
      speed_changes = 1;
    else
      // In all other cases speed sign will be constant:
      speed_changes = 0;

    // Identifying all the cases when to achieve a full backlash compensation we need to use goto twice: first goto pos1-BACKLASH, then goto pos1
    // (The second goto is initiated in backlash() )
    // Doing an overkill here (shouldn't be an issue with go_to stuff): even if a small amount of BL is expected, we'll do a full BACKLASH overshoot and recovery.
    if (
      // Moving towards the target, in the bad (negative) direction:
      g.speed <= 0.0 && !speed_changes ||
      // Moving towards the target, in the good direction, but might not far enough to compensate for the current backlash:
      g.speed > 0.0 && !speed_changes && g.BL_counter > 0 ||
      // Moving in the bad direction, will have to reverse the direction to the good one, but at the end not enough to compensate for BL:
      g.speed <= 0.0 && speed_changes && floorMy(dx_stop - dx) < BACKLASH ||
      // Initially moving in the good direction, but reverse at the end, so BL compensation is needed:
      g.speed > 0.0 && speed_changes)
    {
      // Current target position (to be achieved in the current go_to call):
      pos1 = pos1 - (float)BACKLASH;
      // In all of these cases, speed1<0.0 in the first go_to
      speed1_loc = -speed;
    }
    else
      // In all other cases we will approach the target with the good (positive) speed sign
    {
      speed1_loc = speed;
    }
  }

  // Global parameter to be used in motor_control():
  g.pos_goto = pos1;

  // Setting the target speed and moving_mode=1:
  change_speed(speed1_loc, 1);

  g.pos_stop_flag = 0;

  return;

}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void stop_now()
/*
 Things to do when we completely stop. Should only be called from motor_control()
 */
{
#ifdef TIMING
  // Update timing stats for the very last loop in motion (before setting g.moving=0):
  timing();
#endif

  g.moving = 0;
  g.t_old = g.t;
  g.pos_old = g.pos;
  g.pos_short_old = floorMy(g.pos);

#ifdef TIMING
  // Displaying the timing data from the last movement:
  display_current_position();
  delay(5000);
  g.i_timing = (unsigned long)0;
  g.dt_max = (short)0;
  g.dt_min = (short)10000;
  g.bad_timing_counter = (short)0;
#endif

  if (g.error == 1)
  {
    unsigned char limit_on = digitalRead(PIN_LIMITERS);
    // If we fixed the error 1 (limiter on initially) by rewinding to a safe area, set error code to 0:
    if (limit_on == LOW)
      g.error = 0;
  }

#ifdef SAVE_ENERGY
  digitalWrite(PIN_ENABLE, HIGH);
  delay(ENABLE_DELAY_MS);
#endif

  // Saving the current position to EEPROM:
  EEPROM.put( ADDR_POS, g.pos );

  if (g.calibrate_flag == 5)
    // At this point any calibration should be done (we are in a safe zone, after calibrating both limiters):
  {
    g.calibrate_flag = 0;
    g.calibrate_init = 0;

    EEPROM.put( ADDR_CALIBRATE, 0 );
  }

  if (g.calibrate_flag == 4)
    g.calibrate_flag = 5;

  if ((g.calibrate == 1 || g.calibrate == 2) && g.calibrate_flag == 1)
    g.calibrate_warning = 1;

  // In the initial calibration, disable the warning flag after the first leg:
  if (g.calibrate_init == 3 && g.calibrate_warning == 1)
  {
    g.calibrate_warning = 0;
    //??? To clear garbage in the status line:
    display_status_line("              ");
  }

  if (g.stacker_mode >= 2 && g.backlashing == 0)
  {
    // Ending 2-point focus stacking
    g.stacker_mode = 0;
  }
  // We can lower the breaking flag now, as we already stopped:
  g.breaking = 0;
  g.backlashing = 0;
  g.speed = 0.0;
  // Refresh the whole display:
  display_all("  ");
  g.t_display = g.t;

  if (g.calibrate_flag == 0 && g.coords_change != 0)
    // We apply the coordinate change after doing calibration:
  {
    coordinate_recalibration();
    g.coords_change = 0;
  }

#ifdef MOTOR_DEBUG
  if (g.dt_backlash > dt_backlash)
    dt_backlash = g.dt_backlash;
#endif
#ifdef PRECISE_STEPPING
  g.dt_backlash = 0;
#endif


  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void show_params()
{
  //  Serial.print(pos_short_old);
  //  Serial.print(" ");
  //  Serial.println(pos);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void set_backlight()
// Setting the LCD backlight. 3 levels for now.
{
  switch (g.backlight)
  {
    case 0:
      analogWrite(PIN_LCD_LED, 0);
      break;

    case 1:
      analogWrite(PIN_LCD_LED, 127);
      break;

    case 2:
      analogWrite(PIN_LCD_LED, 255);
      break;
  }
  // Adds stability to backlight change commands:
  //  delay(100);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void coordinate_recalibration()
/*
  Run this every time g.limit1 changes, to recalibrate all the coordinates, with g.limit1 set to zero.
  Should only be run when g.moving=0, after a calibration is done.
 */
{
  if (g.moving)
    return;
  //  EEPROM.put( ADDR_LIMIT1, g.limit1);

  /*
      g.pos = g.pos + (float)g.coords_change;
      g.pos_short_old = g.pos_short_old + g.coords_change;
      g.t0 = g.t;
      g.pos0 = g.pos;
      // Updating g.limit2 (g.limit1-limit1_old is the difference between the new and old coordinates):
      g.limit2 = g.limit2 + g.coords_change;
      EEPROM.put( ADDR_LIMIT2, g.limit2);
      // In new coordinates, g.limit1 is always zero:
      g.limit1 = g.limit1 + g.coords_change;
      */
  g.pos = g.pos + (float)g.coords_change;
  g.pos_short_old = g.pos_short_old + g.coords_change;
  g.t0 = g.t;
  g.pos0 = g.pos;
  // Updating g.limit2 (g.limit1-limit1_old is the difference between the new and old coordinates):
  g.limit2 = g.limit2 + g.coords_change;
  EEPROM.put( ADDR_LIMIT2, g.limit2);
  // In new coordinates, g.limit1 is always zero:
  g.limit1 = g.limit1 + g.coords_change;

  EEPROM.put( ADDR_LIMIT1, g.limit1);
  display_all("  ");

  return;
}


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

  if (g.accel != 0 && g.moving == 0)
  {
    // Starting moving
    g.moving = 1;
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
   Now, with backlash compensation, this function needs to be called once - first explicitely, it will initiate
   travelling to pos1-BACKLASH, then automatically, to the original (actual) target. The automatic part is triggered
   inside stop_now().
 */
{
  float speed1_loc;

  short pos1_short = floorMy(pos1);
  short del_dir1 = g.pos_dir_change_short - pos1_short;

  // We are already there, and no need for backlash compensation:
  if (g.moving == 0 && pos1_short == g.pos_short_old && del_dir1 >= BACKLASH)
    return;

  if (pos1 > g.pos)
    g.direction = 1;
  else
    g.direction = -1;
  motion_status();

  // Stopping distance in the current direction:
  float dx_stop = g.speed * g.speed / (2.0 * ACCEL_LIMIT);
  float dx = pos1 - g.pos;

  // Determining whether the speed will have to change to arrive at the destination:
  short speed_changes = 0;
  if (dx >= 0.0 && g.speed >= 0.0)
    //  Target in the same direction as the current speed, positive speed
    if (dx_stop <= dx)
      // We can make it by just breaking (no speed change involved):
      speed_changes = 0;
    else
      // We can't make it, so will approach the target from the opposite direction (speed change involved):
      speed_changes = 1;
  else if (dx < 0.0 && g.speed < 0.0)
    //  Target in the same direction as the current speed, negative speed
    if (dx >= dx)
      // We can make it by just breaking (no speed change involved):
      speed_changes = 0;
    else
      // We can't make it, so will approach the target from the opposite direction (speed change involved):
      speed_changes = 1;
  else if (dx >= 0.0 && g.speed < 0.0)
    // Moving in the wrong direction, have to change direction (negative speed)
    speed_changes = 1;
  else if (dx < 0.0 && g.speed >= 0.0)
    // Moving in the wrong direction, have to change direction (positive speed)
    speed_changes = 1;

  // Backlash is only compensated for non-stacking moves (rewind etc.):
  if (g.backlash_step == 0 && g.stacker_mode < 2 &&
      // Identifying all the cases when to achieve full backlash compensation we need to use goto twice: first goto pos1+BACKLASH, then goto pos1
      (g.speed > 0.0 && !speed_changes ||
       g.speed <= 0.0 && !speed_changes && del_dir1 < BACKLASH ||
       g.speed > 0.0 && speed_changes && floorMy(dx_stop - dx) < BACKLASH ||
       g.speed <= 0.0 && speed_changes ||
       g.moving == 0 && pos1_short == g.pos_short_old && del_dir1 < BACKLASH))
  {
    // Actual target position (to be achieved in the second go_to call, when backlash_step=1):
    g.actual_target = pos1;
    // Current target position (to be achieved in the current go_to call):
    pos1 = pos1 + (float)BACKLASH;
    // ??? A spot to fix the potential issue of going over g.limit2 - probably not needed as we already use a smaller (by LIMITER_PAD) value for g.limit2
    // In all of these cases, speed1>0.0 in the first go_to
    speed1_loc = speed;
    g.backlash_step = 1;
  }
  else
    // Either single-step go_to operations, or second go_to in a two-step procedure - for backlash compensation
  {
    if (g.speed >= 0.0 && !speed_changes || g.speed < 0.0 && speed_changes)
      speed1_loc = speed;
    else
      speed1_loc = -speed;
  }

  if (g.backlash_step == 2)
    g.backlash_step = 0;

  // Setting the target speed and moving_mode=1:
  change_speed(speed1_loc, 1);

  g.pos_stop_flag = 0;

  // Global parameter to be used in motor_control():
  g.pos_goto = pos1;

  return;

}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void stop_now()
/*
 Things to do when we completely stop. Should only be called from motor_control()
 */
{
  g.moving = 0;

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

  // At this point any calibration should be done (we are in a safe zone, after calibrating both limiters):
  if (g.calibrate_flag == 5)
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

  // We can lower the breaking flag now, as we already stopped:
  g.breaking = 0;
  g.speed = 0.0;
  if (g.stacker_mode >= 2)
  {
    // Ending 2-point focus stacking
    g.stacker_mode = 0;
  }
  // Refresh the whole display:
  display_all("  ");
  g.t_display = g.t;

  if (g.calibrate_flag == 0 && g.coords_change != 0)
    // We apply the coordinate change after doing calibration:
  {
    coordinate_recalibration();
    g.coords_change = 0;
  }

  // Second (small) go_to travel, to enable backlash compensation
  if (g.backlash_step == 1)
  {
    g.backlash_step == 2;
    // Using maximum spped for the second (usually smaller) go_to in backlash compensation sequence, as it is never used during stacking:
    go_to(g.actual_target, SPEED_LIMIT);
  }

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


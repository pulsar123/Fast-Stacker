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

#ifdef DEBUG
  Serial.println(111);
  Serial.print(" speed1_loc=");
  Serial.println(speed1_loc, 6);
#endif

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
#ifdef DEBUG
  Serial.println(222);
  Serial.print(" speed1=");
  Serial.println(g.speed1, 6);
#endif
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void go_to(short pos1_short, float speed)
/* Initiating a travel to pos1 at maximum acceleration and given speed (positive number)
 */
{
  float speed1_loc;

  // We are already there:
  if (g.moving == 0 && pos1_short == g.pos_short_old)
    return;

  if (pos1_short > g.pos_short_old)
    g.direction = 1;
  else
    g.direction = -1;
  motion_status();

  // Stopping distance in the current direction:
  float dx_stop = g.speed * g.speed / (2.0 * ACCEL_LIMIT);
  short dx_short = pos1_short - g.pos_short_old;
#ifdef HIGH_ACCURACY
  // Reducing the required travel distance by DELTA_POS (for precise position stopping in motor_control()):
  // The 10* factor is pretty arbitrary - need a better handle on this
  if (dx_short > 10 * DELTA_POS)
    dx_short = dx_short - DELTA_POS;
  else if (dx_short < -10 * DELTA_POS)
    dx_short = dx_short + DELTA_POS;
#endif
  if (dx_short > 0 && g.speed >= 0.0)
    //  Target in the same direction as the current speed, positive speed
    if (dx_stop <= (float)dx_short)
      // We can make it by just breaking (no speed change involved):
      speed1_loc = speed;
    else
      // We can't make it, so will approach the target from the opposite direction (speed change involved):
      speed1_loc = -speed;
  else if (dx_short < 0 && g.speed < 0.0)
    //  Target in the same direction as the current speed, negative speed
    if (dx_stop >= (float)dx_short)
      // We can make it by just breaking (no speed change involved):
      speed1_loc = -speed;
    else
      // We can't make it, so will approach the target from the opposite direction (speed change involved):
      speed1_loc = speed;
  else if (dx_short > 0 && g.speed < 0.0)
    // Moving in the wrong direction, have to change direction (negative speed)
    speed1_loc = speed;
  else if (dx_short < 0 && g.speed >= 0.0)
    // Moving in the wrong direction, have to change direction (positive speed)
    speed1_loc = -speed;

  // Setting the target speed and moving_mode=1:
  change_speed(speed1_loc, 1);

  g.pos_stop_flag = 0;

  // Global parameter to be used in motor_control():
  g.pos_goto_short = pos1_short;

  return;

}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void stop_now()
/*
 Things to do when we completely stop. Should only be called from motor_control()
 */
{
  g.moving = 0;

#ifdef SAVE_ENERGY
  digitalWrite(PIN_ENABLE, HIGH);
  delay(ENABLE_DELAY_MS);
#endif

  // Things to do if we are not in the emergency breaking mode:
  if (g.breaking == 0)
  {
    // Saving the current position to EEPROM:
    EEPROM.put( ADDR_POS, g.pos );
  }

  // At this point any calibration should be done (we are in a safe zone, afyter calibrating both limiters):
  if (g.calibrate_flag == 5)
    g.calibrate_flag = 0;

  if (g.calibrate_flag == 4)
    g.calibrate_flag = 5;

  // We can lower the breaking flag now, as we already stopped:
  g.breaking = 0;
  g.speed = 0.0;
  if (g.stacker_mode >= 2)
  {
    // Ending 2-point focus stacking
    g.stacker_mode = 0;
  }
  // Refresh the whole display:
  display_all(" ");
#ifdef DEBUG
  Serial.println(444);
  Serial.print(" g.moving=");
  Serial.println(g.moving);
#endif

  return;
}

void show_params()
{
  //  Serial.print(pos_short_old);
  //  Serial.print(" ");
  //  Serial.println(pos);
  return;
}

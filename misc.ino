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
    // Starting moving
    g.moving = 1;

  // Updating the target speed:
  g.speed1 = speed1_loc;

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void go_to(short pos1_short)
/* Initiating a travel to pos1 at maximum acceleration/speed
 */
{
  float speed1_loc; 
  
  // We are already there:
  if (g.moving == 0 && pos1_short == g.pos_short_old)
    return;

  // Stopping distance in the current direction:
  float dx_stop = g.speed * g.speed / (2.0 * ACCEL_LIMIT);
  short dx_short = pos1_short - g.pos_short_old;

  if (dx_short > 0 && g.speed >= 0.0)
    //  Target in the same direction as the current speed, positive speed
  {
    if (dx_stop <= (float)dx_short)
      // We can make it by just breaking (no speed change involved):
    {
      speed1_loc = SPEED_LIMIT;
    }
    else
      // We can't make it, so will approach the target from the opposite direction (speed change involved):
    {
      speed1_loc = -SPEED_LIMIT;
    }
  }
  else if (dx_short < 0 && g.speed < 0.0)
    //  Target in the same direction as the current speed, negative speed
  {
    if (dx_stop >= (float)dx_short)
      // We can make it by just breaking (no speed change involved):
    {
      speed1_loc = -SPEED_LIMIT;
    }
    else
      // We can't make it, so will approach the target from the opposite direction (speed change involved):
    {
      speed1_loc = SPEED_LIMIT;
    }
  }
  else if (dx_short > 0 && g.speed < 0.0)
    // Moving in the wrong direction, have to change direction (negative speed)
  {
    speed1_loc = SPEED_LIMIT;
  }
  else if (dx_short < 0 && g.speed >= 0.0)
    // Moving in the wrong direction, have to change direction (positive speed)
  {
    speed1_loc = -SPEED_LIMIT;
  }

// Setting the target speed and moving_mode=1:
  change_speed(speed1_loc, 1);

  g.pos_stop_flag = 0;
  
// Global parameter to be used in motor_control():
  g.pos_goto_short = pos1_short;

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

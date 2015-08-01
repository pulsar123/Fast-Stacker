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
void change_speed(float speed1_loc)
/* Run the function every time you want to change speed. It will figure out required accel based on current speed and speed1,
   and will update t0, speed0, pos0, if accel changed here.
   Inputs:
    - speed1_loc: new target speed.
 */
{
  short new_accel;

  // Ignore any speed change requests during emergency breaking  (after hitting a limiter)
  // DOn't forget to reset breaking=0 somewhere!
  if (breaking || calibrate_flag == 2)
    return;

  if (speed1_loc >= speed)
    // We have to accelerate
    new_accel = 1;
  else
    // Have to decelerate:
    new_accel = -1;

  if (new_accel != accel)
    // Acceleration changed
  {
    accel = new_accel;
    // Memorizing the current values for t, speed and pos:
    t0 = t;
    speed0 = speed;
    pos0 = pos;
  }

  if (accel != 0 && moving == 0)
    // Starting moving
    moving = 1;

  // Updating the target speed:
  speed1 = speed1_loc;

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void travel_init(short pos1_short)
/* Initiating a travel to pos1 at maximum acceleration/speed
 */
{
  // Have to be still to travel:
  if (moving > 0 || pos1_short==pos_short_old)
    return;

  if (pos1_short > pos_short_old)
  {
    speed_change(SPEED_LIMIT);
  }
  else if (pos1_short < pos_short_old)
  {
    speed_change(-SPEED_LIMIT);
  }
  
  travel_flag = 1;
  pos_travel_short = pos1_short;

  return;

}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void travel()
/* Travelling to predefined position, pos_travel_short
 */
{
  if (travel_flag==0 || breaking==1)
    return;

 // Checking how far we are from a limiter in the direction we are moving
  if (speed < 0.0)
    dx = pos_short_old - pos_travel_short;
  else
    dx = pos_travel_short - pos_short_old;
  // Preliminary test (for highest possible speed):
  if (dx <= roundMy(BREAKING_DISTANCE))
  {
    // Breaking distance at the current speed:
    dx_break = roundMy(0.5 * speed * speed / ACCEL_LIMIT);
    // Accurate test (for the current speed):
    if (dx <= dx_break)
      // Emergency breaking, to avoid hitting the limiting switch
    {
      change_speed(0.0);
      breaking = 1;
    }
  }


}

void show_params()
{
  //  Serial.print(pos_short_old);
  //  Serial.print(" ");
  //  Serial.println(pos);
  return;
}

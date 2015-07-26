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

  if (speed1 >= speed)
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


void show_params()
{
  //  Serial.print(pos_short_old);
  //  Serial.print(" ");
  //  Serial.println(pos);
  return;
}

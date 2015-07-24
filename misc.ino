short floor(float x)
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
void accel_change(short direction_loc, short accel_loc, float speed1_loc)
/* Run the function every time you want to change acceleration.
   Inputs:
    - direction_loc: new direction; ignored if =0 - convenient for the cases when you just want to keep the old
        direction; direction is only updated when accel changed and old direction=0;
    - accel_loc: the new value for acceleration
    - speed1_loc: new target speed.
 */
{
  if (accel_loc != accel)
    // Only do something when the acceleration actually changed:
  {
    accel = accel_loc;
    // Memorizing the current values for t, speed and pos:
    t0 = t;
    speed0 = speed;
    pos0 = pos;
    if (direction == 0 && direction_loc != 0)
      {
        // We are here only if we are to change the direction (from rest only)
        direction = direction_loc;
        // Sending the direction sigmal to the motor:
        if (direction == -1)
          digitalWrite(PIN_DIR, LOW);
        else
          digitalWrite(PIN_DIR, HIGH);
      }
    // Updating the target speed:
    speed1 = speed1_loc;
  }
  return;
}


void show_params()
{
//  Serial.print(pos_short_old);   
//  Serial.print(" ");
//  Serial.println(pos);
  return;
}

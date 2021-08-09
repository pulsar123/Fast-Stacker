

void my_setCursor(byte pos, byte line, byte set)
/*
   Added in h2.0. Translates old position/line coordinates (0...13 / 0...5) to new display pixel coordinates -
   stored in global vars g.x0, g.y0. If set=1, also execute tft.setCursor with these coordinates
*/
{

  g.y0 = TOP_GAP - 1 + line * (LINE_GAP + FONT_HEIGHT);

#ifdef TIMING
  if (line == TFT_NY)
    g.y0 = 128 - TOP_GAP - 1 - 2 * FONT_HEIGHT - 6;
#endif

  g.x0 = LEFT_GAP + pos * FONT_WIDTH;

  //  if (set == 1)
  tft.setCursor(g.x0, g.y0);

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


float target_speed ()
// Estimating the required speed in microsteps per microsecond
{
  return 1e-6 * FPS[g.reg.i_fps] * MSTEP_PER_FRAME[g.reg.i_mm_per_frame];
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


short Nframes ()
/* Computing the "Nframes" parameter (only for 2-point stacking) - redo this every time either of
   g.reg.point[FOREGROUND], or g.reg.point[BACKGROUND] changes.
*/
{
  return short(((float)(g.reg.point[BACKGROUND] - g.reg.point[FOREGROUND])) / (float)MSTEP_PER_FRAME[g.reg.i_mm_per_frame]) + 1;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char *ftoa(char *a, float f, int precision)
// Converting float to string (up to 9 decimals)
{
  long p[] = {0, 10, 100, 1000, 10000};

  char *ret = a;
  long heiltal = (long)f;
  //  itoa(heiltal, a, 10);
  sprintf(a, "%d", heiltal);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  // Filling up with leading zeros if needed:
  for (byte i = snprintf(0, 0, "%+d", desimal) - 1; i < precision; i++)
    *a++ = '0';
  //  itoa(desimal, a, 10);
  sprintf(a, "%d", desimal);
  return ret;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

COORD_TYPE roundMy(float x)
/* Rounding of float numbers, output - COORD_TYPE.
   Works with positive, negative numbers and 0.
*/
{
  if (x >= 0.0)
    return (COORD_TYPE)(x + 0.5);
  else
    return (COORD_TYPE)(x - 0.5);
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void coordinate_recalibration()
/*
  Run this every time g.limit1 changes, to recalibrate all the coordinates, with g.limit1 set to zero.
  Should only be run when g.moving=0, after a calibration is done.
*/
{
#ifdef TEST_SWITCH
  return;
#endif
  if (g.moving)
    return;


  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



void set_accel_v()
{
  // Five possible floating point values for acceleration
  g.accel_v[0] = -ACCEL_LIMIT;
  g.accel_v[1] = -ACCEL_LIMIT / (float)ACCEL_FACTOR[g.reg.i_accel_factor];
  g.accel_v[2] = 0.0;
  g.accel_v[3] =  ACCEL_LIMIT / (float)ACCEL_FACTOR[g.reg.i_accel_factor];
  g.accel_v[4] =  ACCEL_LIMIT;
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



void rail_reverse(byte fix_points)
/* Reversing the rail operation - either manually (*1 function), or automatically, when loading one of the memory registers
   If fix_points=1, update the current point1,2 accordingly.
*/
{
  COORD_TYPE d_ipos, ipos_target;

  // We need to do a full backlash compensation loop when reversing the rail operation:
  g.BL_counter = g.backlash;
  // This will instruct the backlash module to do BACKLASH_2 travel at the end, to compensate for BL in reveresed coordinates
  d_ipos = g.limit2 + g.backlash;
  if (g.reg.backlash_on != 0)
  {
    d_ipos = d_ipos - g.reg.backlash_on * BACKLASH_2;
    g.Backlash_init = 2;
  }
  else
  {
    g.Backlash_init = 1;
  }
  // Updating the current coordinate in the new (reversed) frame of reference:
  g.ipos = d_ipos - g.ipos;
  EEPROM.put( ADDR_POS, g.ipos );
  if (fix_points)
  {
    // Updating the current two points positions:
    ipos_target = d_ipos - g.reg.point[g.point2];
    g.reg.point[g.point2] = d_ipos - g.reg.point[g.point1];
    g.reg.point[g.point1] = ipos_target;
    EEPROM.put( g.addr_reg[0], g.reg);
  }


  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



COORD_TYPE frame_coordinate()
// Coordinate (COORD_TYPE type) of a frame given by g.frame_number, in 2-point stacking
{
  // Stacking direction depends on the backlash direction (we always move in the good direction)
  if (g.reg.backlash_on >= 0)
    return g.starting_point + g.frame_counter * MSTEP_PER_FRAME[g.reg.i_mm_per_frame];
  else
    return g.starting_point - g.frame_counter * MSTEP_PER_FRAME[g.reg.i_mm_per_frame];
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




void read_params(byte n)
/*
   Read register n (1...N_REGS) from EEPROM
*/
{
  byte straight_old = g.reg.straight;
  EEPROM.get( g.addr_reg[n], g.reg);
  // Memorizing as default environment:
  EEPROM.put( g.addr_reg[0], g.reg);
  g.Nframes = Nframes();
  display_all();
  sprintf(g.buffer, "   Loading Reg %2d   ", n);
  display_comment_line(g.buffer);
  if (g.reg.straight != straight_old)
    // If the rail needs a rail reverse, initiate it:
  {
    // Not updating point1,2:
    rail_reverse(0);
  }
  // Loading new register clears the current memory point index:
  g.current_point = -1;
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



void save_params(byte n)
// Now only used in macro mode
{
  EEPROM.put( g.addr_reg[n], g.reg);
  sprintf(g.buffer, "   Saved to Reg%1d    ", n);
  display_comment_line(g.buffer);
  //  tft.print("       ");
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



void update_backlash()
// Call this every time g.reg.backlash_on changes
{
  if (g.reg.backlash_on != 0)
  {
    g.backlash = g.reg.backlash_on * BACKLASH; // Now can be + or -
    g.BL_counter = g.backlash;
    g.Backlash_init = 1;
  }
  else
    g.backlash = 0;

  if (g.reg.backlash_on >= 0)
  {
    g.point1 = FOREGROUND;
    g.point2 = BACKGROUND;
  }
  else
  {
    g.point2 = FOREGROUND;
    g.point1 = BACKGROUND;
  }


  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



void update_save_energy()
// Call it every time g.reg.save_energy is changed
{
#ifdef DISABLE_MOTOR
  return;
#else
  if (g.reg.save_energy)
  {
    // Not using the holding torque feature (to save batteries)
    g.enable_flag = HIGH;
    iochip.digitalWrite(EPIN_ENABLE, g.enable_flag);
  }
  else
  {
    // Using the holding torque feature (bad for batteries; good for holding torque and accuracy)
    g.enable_flag = LOW;
    iochip.digitalWrite(EPIN_ENABLE, g.enable_flag);
  }
  return;
#endif
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void move_to_next_frame(COORD_TYPE * pos_target, short * frame_counter0)
/*
   Make a step to pos_target, update frame counter
*/
{
  // This 100 steps padding is just a hack, to fix the occasional bug when a combination of single frame steps and rewind can
  // move the rail beyond g.limit1
  if (*pos_target < 100 || *pos_target > g.limit2 - 100 || g.paused && (g.frame_counter < 0 || g.frame_counter >= g.Nframes))
  {
    // Recovering the original frame counter if aborting:
    g.frame_counter = *frame_counter0;
    return;
  }
  go_to(*pos_target, SPEED_LIMIT);
  display_frame_counter();
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void set_memory_point(char n)
// Setting one of the two memory points - current coordinate
{
  if (g.paused || g.moving)
    return;
  g.current_point = n - 1;
  g.reg.point[g.current_point] = g.ipos;
  // Saving the changed register as default one:
  EEPROM.put( g.addr_reg[0], g.reg);
  g.Nframes = Nframes();
  display_all();
  sprintf(g.buffer, "     P%1d was set     ", n);
  display_comment_line(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void goto_memory_point(char n)
// Go to memory point # n (1..2)
{
  if (g.paused || n == 0)
    return;
  g.current_point = n - 1;
  // Travelling to the memory point position
  go_to(g.reg.point[g.current_point], SPEED_LIMIT);
  sprintf(g.buffer, "    Going to P%1d     ", n);
  display_comment_line(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



void Read_limiters()
/* Read the input from the limiting switches:
   Sets g.limit_on to HIGH/1 if any limiter is enabled; sets it to LOW/0 otherwise.
*/
{
  byte limit_on;
#ifdef MOTOR_DEBUG
  g.limit_on = 0;
  return;
#else
  limit_on = digitalRead(PIN_LIMITERS);
#endif

  g.limit_on = limit_on;
  /*
    // Impulse noise suppression:
    if (limit_on == 1)
      {
        g.limiter_counter++;
        if (g.limiter_counter >= N_LIMITER)
        {
          g.limit_on = 1;
        }
      }
      else
      {
        g.limit_on = 0;
        g.limiter_counter = 0;
      }
  */

  return;
}


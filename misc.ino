

void my_setCursor(byte pos, byte line, byte set)
/*
 * Added in h2.0. Translates old position/line coordinates (0...13 / 0...5) to new display pixel coordinates -
 * stored in global vars g.x0, g.y0. If set=1, also execute tft.setCursor with these coordinates
 */
{

  if (line < TFT_NY-1)
    g.y0 = TOP_GAP - 1 + line*(LINE_GAP+FONT_HEIGHT);
    else
    // Bottom (status) line is special, separated from the rest:
    g.y0 = 128 - TOP_GAP - 1 - FONT_HEIGHT -3;

#ifdef TIMING
  if (line == TFT_NY)
    g.y0 = 128 - TOP_GAP - 1 - 2*FONT_HEIGHT -6;
#endif

  if (pos < 7)
    g.x0 = LEFT_GAP + pos*FONT_WIDTH;
    else
    g.x0 = LEFT_GAP + (pos+4)*FONT_WIDTH;

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
   g.msteps_per_frame, g.reg.point[0], or g.reg.point[3] changes.
*/
{
  return short(((float)(g.reg.point[3] - g.reg.point[0])) / (float)MSTEP_PER_FRAME[g.reg.i_mm_per_frame]) + 1;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char *ftoa(char *a, float f, int precision)
// Converting float to string (up to 9 decimals)
{
  long p[] = {0, 10, 100, 1000};

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



void set_backlight()
// Setting the LCD backlight. Up to 32 levels.
// Unused in h2.0!
{
  byte level;

//  analogWrite(PIN_LCD_LED, Backlight[g.backlight]);

//  EEPROM.put( ADDR_BACKLIGHT, g.backlight);

  return;
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
  g.accel_v[0] = -g.accel_limit;
  g.accel_v[1] = -g.accel_limit / (float)ACCEL_FACTOR[g.reg.i_accel_factor];
  g.accel_v[2] = 0.0;
  g.accel_v[3] =  g.accel_limit / (float)ACCEL_FACTOR[g.reg.i_accel_factor];
  g.accel_v[4] =  g.accel_limit;
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



void rail_reverse(byte fix_points)
/* Reversing the rail operation - either manually (*1 function), or automatically, when loading one of the memory registers
   If fix_points=1, update the current point1,2 accordingly.
*/
{
  COORD_TYPE d_pos, pos_target;

  // Disabling rail reverse in telescope mode:
  if (g.telescope)
    return;

  // We need to do a full backlash compensation loop when reversing the rail operation:
  g.BL_counter = g.backlash;
  // This will instruct the backlash module to do BACKLASH_2 travel at the end, to compensate for BL in reveresed coordinates
  d_pos = g.limit2 + g.backlash;
  if (g.reg.backlash_on)
  {
    d_pos = d_pos - BACKLASH_2;
    g.backlash_init = 2;
  }
  else
  {
    g.backlash_init = 1;
  }
  // Updating the current coordinate in the new (reversed) frame of reference:
  g.pos = d_pos - g.pos;
  EEPROM.put( ADDR_POS, g.ipos );
  g.pos0 = g.pos;
  g.pos_old = g.pos;
  g.pos_int_old = (COORD_TYPE)floor(g.pos);
  if (fix_points)
  {
    // Updating the current two points positions:
    pos_target = d_pos - g.reg.point[3];
    g.reg.point[3] = d_pos - g.reg.point[0];
    g.reg.point[0] = pos_target;
    EEPROM.put( g.addr_reg[0], g.reg);
  }


  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



COORD_TYPE frame_coordinate()
// Coordinate (COORD_TYPE type) of a frame given by g.frame_number, in 2-point stacking
{
  return g.starting_point + (COORD_TYPE)g.frame_counter * (COORD_TYPE)MSTEP_PER_FRAME[g.reg.i_mm_per_frame];
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




void read_params(byte n)
/*
   Read register n (1...g.n_regs) from EEPROM
*/
{
  byte straight_old = g.reg.straight;
  EEPROM.get( g.addr_reg[n], g.reg);
  if (g.telescope)
  {
    g.ireg = n;
    EEPROM.put( ADDR_IREG, g.ireg );
#ifdef TEMPERATURE
    for (byte i = 0; i < 4; i++)
      g.Temp0[i] = compute_temperature(g.reg.raw_T[i]);
    // This computes delta_pos[]:
    measure_temperature();
#endif
  }
  else
  {
    // Memorizing as default environment:
    EEPROM.put( g.addr_reg[0], g.reg);
    //  g.msteps_per_frame = MSTEP_PER_FRAME[g.reg.i_mm_per_frame];
    g.Nframes = Nframes();
  }
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
  if (g.reg.backlash_on)
  {
    if (g.telescope)
      g.backlash = BACKLASH_TEL;
    else
      g.backlash = BACKLASH;
    g.BL_counter = g.backlash;
    g.backlash_init = 1;
  }
  else
    g.backlash = 1;
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
    // Not using the holding torque feature (to save batteries)
    iochip.digitalWrite(EPIN_ENABLE, HIGH);
  else
    // Using the holding torque feature (bad for batteries; good for holding torque and accuracy)
    iochip.digitalWrite(EPIN_ENABLE, LOW);
  return;
#endif
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



#ifdef TEMPERATURE
float compute_temperature(float raw_T)
// Computing Celsius temperature from raw temperature
{
  // Logarithm of the resistance of the thermistor (from voltage divider equation):
  float lnR = log(R_pullup / (1024.0 / raw_T - 1.0));
  // Using  Steinhart–Hart equation to compute the temperature (in K):
  return 1.0 / (SH_a + SH_b * lnR + SH_c * lnR * lnR * lnR) - TEMP0_K;
}


void measure_temperature()
// Measuring temperature in telescope mode. Needs Temp0[0..3] precomputed.
{
  if (!g.telescope)
    return;

  // reading the raw value at PIN_AF (which connects to a grounded thermistor in telescope mode):
  float raw = 0.0;
  for (byte i = 0; i < N_TEMP; i++)
    raw = raw + (float)analogRead(PIN_AF);
  raw = raw / N_TEMP;
  g.raw_T = (int)(raw + 0.5);

  // Current temperature (Celsius):
  g.Temp = compute_temperature(raw);
  for (byte i = 0; i < 4; i++)
  {
    if (g.reg.mirror_lock)
      // Temperature-induced shift in the focal plane of the telescope caused by the thermal expansion of the telescope tube, in microsteps:
      // +0.5 is for proper round-off
      // It is minus CTE because for positive CTE (telescope tube expanding with temperature increasing) the focus point moves closer to the
      // telescope (meaning coordinate becomes smaller).
      g.delta_pos[i] = (COORD_STYPE)(-CTE * (g.Temp - g.Temp0[i]) / g.mm_per_microstep + 0.5);
    else
      g.delta_pos[i] = 0;
  }

  return;
}
#endif
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
  go_to(*pos_target, g.speed_limit);
  display_frame_counter();
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void set_memory_point(char n)
// Setting one of the two (macro mode) or four (telescope mode) memory points - current coordinate and (telescope mode only)
// current raw temperature. n=1...4
{
  if (g.paused || g.moving)
    return;
  if (g.locked[g.ireg - 1])
  {
    display_comment_line("       Locked       ");
    return;
  }
  g.current_point = n - 1;
  g.reg.point[g.current_point] = g.pos_int_old;
#ifdef TEMPERATURE
  if (g.telescope)
  {
    // Measuring current temperature g.raw_T / g.Temp
    measure_temperature();
    g.reg.raw_T[g.current_point] = g.raw_T;
    g.Temp0[g.current_point] = g.Temp;
    g.delta_pos[g.current_point] = 0;
    g.delta_pos_curr = 0;
  }
#endif
  // Saving the changed register as default one (macro mode) or as the current (ireg) one:
  EEPROM.put( g.addr_reg[g.ireg], g.reg);
  g.Nframes = Nframes();
  display_all();
  sprintf(g.buffer, "     P%1d was set     ", n);
  display_comment_line(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void goto_memory_point(char n)
// Go to memory point # n (1..4)
{
  if (g.paused || n == 0)
    return;
#ifdef TEMPERATURE
  if (g.telescope)
  {
    // Computing delta_pos based on the current temperature
    measure_temperature();
  }
#endif
  g.current_point = n - 1;
  g.delta_pos_curr = g.delta_pos[g.current_point];
  // Travelling to the memory point position corrected for the current temperature:
  go_to(g.reg.point[g.current_point] + g.delta_pos[g.current_point], g.speed_limit);
  sprintf(g.buffer, "    Going to P%1d     ", n);
  display_comment_line(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void start_breaking()
// Initiating breaking at the highest deceleration allowed
{
  change_speed(0.0, 0, 2);
  g.uninterrupted = 1;
  letter_status("B");
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
#ifdef HALL_SENSOR
//!!!!
//  g.limit_on = g.telescope - g.limit_on;
  g.limit_on = 1 - g.limit_on;
#endif
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


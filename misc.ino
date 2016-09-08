float target_speed ()
// Estimating the required speed in microsteps per microsecond
{
  float x = FPS[g.reg.i_fps] * MM_PER_FRAME[g.reg.i_mm_per_frame];
  if (g.telescope)
    return SPEED_SCALE_TEL * x;
  else
    return SPEED_SCALE * x;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


float Msteps_per_frame ()
/* Computing the "microsteps per frame" parameter - redo this every time g.reg.i_mm_per_frame changes.
*/
{
  if (g.telescope)
    return (MM_PER_FRAME[g.reg.i_mm_per_frame] / MM_PER_ROTATION_TEL) * MICROSTEPS_PER_ROTATION;
  else
    return (MM_PER_FRAME[g.reg.i_mm_per_frame] / MM_PER_ROTATION) * MICROSTEPS_PER_ROTATION;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


short Nframes ()
/* Computing the "Nframes" parameter (only for 2-point stacking) - redo this every time either of
   g.msteps_per_frame, g.reg.point[0], or g.reg.point[3] changes.
*/
{
  return short(((float)(g.reg.point[3] - g.reg.point[0])) / g.msteps_per_frame) + 1;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char *ftoa(char *a, float f, int precision)
// Converting float to string (up to 9 decimals)
{
  long p[] = {0, 10, 100, 1000};

  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  // Filling up with leading zeros if needed:
  for (byte i = snprintf(0, 0, "%+d", desimal) - 1; i < precision; i++)
    *a++ = '0';
  itoa(desimal, a, 10);
  return ret;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

COORD_TYPE nintMy(float x)
/*
  My version of nint. Float -> COORD_TYPE conversion. Valid for positive/negative/zero.
*/
{
  // Rounding x towards 0:
  COORD_TYPE x_short = (COORD_TYPE)x;
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
      return x_short + (COORD_TYPE)1;
    else
      return x_short - (COORD_TYPE)1;
  }
  else
    return x_short;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


COORD_TYPE floorMy(float x)
/* A limited implementation of C function floor - only to convert from float to COORD_TYPE.
   Works with positive, negative numbers and 0.
*/
{
  COORD_TYPE m = x;
  if (x >= 0.0)
    return m;
  else
    return m - (COORD_TYPE)1;
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
void change_speed(float speed1_loc, byte moving_mode1, char accel)
/* Run the function every time you want to change speed. It will figure out required acceleration based on current speed and speed1,
   and will update t0, speed0, pos0, if accel changed here. The parameter "accel" is the suggested acceleration (0, 1, or 2).
   Inputs:
    - speed1_loc: new target speed.
    When moving_mode1=1, global moving_mode=1 is  enabled (to be used in go_to).
*/
{
  char new_accel;

  // Ignore any speed change requests during emergency breaking  (after hitting a limiter)
  if (g.breaking || g.calibrate_flag == 2)
    return;

  g.moving_mode = moving_mode1;

  if (speed1_loc >= g.speed)
    // We have to accelerate
    new_accel = accel;
  else
    // Have to decelerate:
    new_accel = -accel;

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
    if (g.reg.save_energy)
    {
#ifndef DISABLE_MOTOR
      digitalWrite(PIN_ENABLE, LOW);
#endif
      delay(ENABLE_DELAY_MS);
    }
  }

  // Updating the target speed:
  g.speed1 = speed1_loc;
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void go_to(float pos1, float speed)
/* Initiating a travel to pos1 at given target speed (positive number) and maximum acceleration.
   With non-zero g.backlash constant, all go_to moves result in fully backlash-compensated moves. The
   backlash model used is the simplest possible: when starting from a good initial position
   (g.BL_counter=0), meaning the physical coordinate = program coordinate, and
   moving in the bad (negative pos) direction, the rail will not start moving until g.backlash
   steps are done (and g.BL_counter gets the largest possible value, g.backlash), and then it starts
   moving instantly. If switching direction, again the rail doesn't move until g.backlash steps
   are carried out, and g.BL_counter becomes 0 (smallest possible value), and then it starts moving
   instantly. The current physical coordinate of the rail is always connected to the program
   coordinate via equation:

   pos_phys = pos_prog + g.BL_counter

*/
{
  float speed1_loc, dx_stop;
  byte speed_changes;

  if (g.breaking || g.backlashing)
    return;

  // Ultimate physical coordinate to achieve:
  COORD_TYPE pos1_short = floorMy(pos1);

  // Current physical coordinate:
  COORD_TYPE pos_short_phys = g.pos_short_old + g.BL_counter;
  float pos_phys = g.pos + (float)g.BL_counter;

  // We are already there, and no need for backlash compensation, so just returning:
  if (g.moving == 0 && pos1_short == g.pos_short_old && g.BL_counter == (COORD_TYPE)0)
    return;

  // The "shortcut" direction - if there was no acceleration limit and no need for backlash compensation:
  // If we are here and pos1_short = pos_short_phys, that could only happen if g.BL_counter>0, so what we need
  // to accomplish is to compensate the backlash (move g.BL_counter steps in the positive direction)
  if (pos1_short >= pos_short_phys)
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
      // Overshooting by g.backlash microsteps (this will be compensated in backlash() function after we stop):
      pos1 = pos1 - (float)g.backlash;
      speed1_loc = -speed;
    }
  }

  else
    // We are currently moving
  {
    // Stopping distance in the current direction:
    // Breaking is always done with the maximum deceleration:
    dx_stop = g.speed * g.speed / (2.0 * g.accel_limit);
    // Travel vector:
    float dx_vec = pos1 - g.pos;
    float dx = fabs(dx_vec);
    // Number of whole steps to take if going straight to the target:
    COORD_TYPE dx_steps = pos1_short - g.pos_short_old;

    // All the cases when speed sign will change while traveling to the target:
    // When we move in the correct direction, but cannot stop in time because of the acceleration limit
    if (dx < dx_stop && (g.direction > 0 && g.speed > 0.0 || g.direction < 0 && g.speed <= 0.0) ||
        // or when we are moving in the wrong direction
        g.direction > 0 && g.speed <= 0.0 || g.direction < 0 && g.speed > 0.0)
      speed_changes = 1;
    else
      // In all other cases speed sign will be constant:
      speed_changes = 0;

    // Identifying all the cases when to achieve a full backlash compensation we need to use goto twice: first goto pos1-g.backlash, then goto pos1
    // (The second goto is initiated in backlash() )
    if (
      // Case 1: Moving towards the target, in the bad (negative) direction:
      g.speed <= 0.0 && !speed_changes ||
      // Case 2: Moving in the bad direction, will have to reverse the direction to the good one, but at the end not enough to compensate for BL:
      g.speed <= 0.0 && speed_changes && floorMy(dx_stop - dx) < g.backlash ||
      // Case 3: Initially moving in the good direction, but reverse at the end, so BL compensation is needed:
      g.speed > 0.0 && speed_changes)
    {
      // Current target position (to be achieved in the current go_to call):
      pos1 = pos1 - (float)g.backlash;
      // In all of these cases, speed1<0.0 in the first go_to
      speed1_loc = -speed;
    }
    else
      // In all other cases we will approach the target with the good (positive) speed sign
    {
      speed1_loc = speed;
    }
  }  // End of else (if we are currently moving)

  // Global parameter to be used in motor_control():
  g.pos_goto = pos1;

  // Setting the target speed and moving_mode=1; in go_to, acceleration is always maximum possible (accel=2):
  change_speed(speed1_loc, 1, 2);

  g.pos_stop_flag = 0;

  return;

}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void stop_now()
/*
  Things to do when we decide to stop inside motor_control().
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

  if (g.telescope == 0)
    if (g.error == 1)
    {
      unsigned char limit_on = digitalRead(PIN_LIMITERS);
      // If we fixed the error 1 (limiter on initially) by rewinding to a safe area, set error code to 0:
      if (limit_on == LOW)
        g.error = 0;
    }

  if (g.reg.save_energy)
  {
#ifndef DISABLE_MOTOR
    digitalWrite(PIN_ENABLE, HIGH);
#endif
    delay(ENABLE_DELAY_MS);
  }

  // Saving the current position to EEPROM:
  if (!g.telescope)
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
    // To clear garbage in the status line:
    display_status_line();
  }

  if (g.stacker_mode >= 2 && g.backlashing == 0 && g.continuous_mode == 1)
  {
    // Ending focus stacking
    g.stacker_mode = 0;
  }

  // We can lower the breaking flag now, as we already stopped:
  g.breaking = 0;
  g.backlashing = 0;
  g.speed = 0.0;
  // Refresh the whole display:
  display_all();
  if (g.noncont_flag > 0)
  {
    letter_status("S");
  }
  g.t_display = g.t;

  if (g.calibrate_flag == 0 && g.coords_change != 0)
    // We apply the coordinate change after doing calibration:
  {
    coordinate_recalibration();
    g.coords_change = 0;
  }

  // Used in continuous_mode=0; we are here right after the travel to the next frame position
  if (g.noncont_flag == 4)
    g.noncont_flag = 1;

#ifdef PRECISE_STEPPING
  g.dt_backlash = 0;
#endif
#ifdef EXTENDED_REWIND
  g.no_extended_rewind = 0;
#endif

  // Any act of moving (followed by stopping) changes the coordinate, so we have to clear the current point index
  // (because we're no longer at any specific memory point):
  g.current_point = -1;

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void set_backlight()
// Setting the LCD backlight. Up to 32 levels.
{
  unsigned char level;
  //  level = g.backlight * (255 / (N_BACKLIGHT - 1));
  switch (g.backlight)
  {
    case 0:
      level = 0;
      break;
    case 1:
      // Very low value for complete darkness:
      level = 10;
      break;
    case 2:
      level = 255;
  }
  analogWrite(PIN_LCD_LED, level);

  EEPROM.put( ADDR_BACKLIGHT, g.backlight);

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
  // Saving the current position to EEPROM:
  if (!g.telescope)
    EEPROM.put( ADDR_POS, g.pos );
  display_all();

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
  d_pos = g.limit1 + g.limit2 + g.backlash;
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
  EEPROM.put( ADDR_POS, g.pos );
  g.pos0 = g.pos;
  g.pos_old = g.pos;
  g.pos_short_old = floorMy(g.pos);
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
  return g.starting_point + nintMy(((float)g.frame_counter) * g.msteps_per_frame);
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++




void read_params(byte n)
{
  byte straight_old = g.reg.straight;
  EEPROM.get( g.addr_reg[n], g.reg);
  // Memorizing as default environment:
  EEPROM.put( g.addr_reg[0], g.reg);
  g.msteps_per_frame = Msteps_per_frame();
  g.Nframes = Nframes();
  if (g.telescope)
  {
    g.displayed_register = n;
#ifdef TEMPERATURE
    for (byte i = 0; i < 4; i++)
      g.Temp0[i] = compute_temperature(g.reg.raw_T[i]);
    // This computes delta_pos[]:
    measure_temperature();
#endif
  }
  display_all();
  display_comment_line("Read from Reg");
  lcd.print(n);
  lcd.clearRestOfLine();
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
{
  if (g.telescope)
  {
    g.displayed_register = n;
    display_all();
  }
  EEPROM.put( g.addr_reg[n], g.reg);
  display_comment_line("Saved to Reg");
  lcd.print(n);
  lcd.clearRestOfLine();
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
    g.backlash = (COORD_TYPE)1;
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
    digitalWrite(PIN_ENABLE, HIGH); // Not using the holding torque feature (to save batteries)
  else
    digitalWrite(PIN_ENABLE, LOW); // Using the holding torque feature (bad for batteries; good for holding torque and accuracy)
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
  for (unsigned short i = 0; i < N_TEMP; i++)
    raw = raw + (float)analogRead(PIN_AF);
  raw = raw / N_TEMP;
  g.raw_T = (int)(raw + 0.5);

  // Current temperature (Celsius):
  g.Temp = compute_temperature(raw);
  // Temperature-induced shift in the focal plane of the telescope caused by the thermal expansion of the telescope tube, in microsteps:
  // +0.5 is for proper round-off
  if (g.reg.mirror_lock)
    for (byte i = 0; i < 4; i++)
    {
      g.delta_pos[i] = (COORD_TYPE)(CTE * (g.Temp - g.Temp0[i]) / g.mm_per_microstep + 0.5);
    }

  return;
}
#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



void make_step(COORD_TYPE * pos_target, short * frame_counter0)
/*
   Make a step to pos_target, update frame counter
*/
{
  // This 100 steps padding is just a hack, to fix the occasional bug when a combination of single frame steps and rewind can
  // move the rail beyond g.limit1
  if (*pos_target < g.limit1 + (COORD_TYPE)100 || *pos_target > g.limit2 - (COORD_TYPE)100 || g.paused && (g.frame_counter < 0 || g.frame_counter >= g.Nframes))
  {
    // Recovering the original frame counter if aborting:
    g.frame_counter = *frame_counter0;
    return;
  }
  go_to(*pos_target + 0.5, g.speed_limit);
  display_frame_counter();
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



void clear_calibrate_state()
// Clear calibrate state
{
  g.calibrate = 0;
  g.calibrate_flag = 0;
  g.calibrate_warning = 0;
  g.calibrate_init = g.calibrate;
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void set_memory_point(byte n)
// Setting one of the two (macro mode) or four (telescope mode) memory points - current coordinate and (telescope mode only)
// current raw temperature
{
  if (g.paused || g.moving)
    return;
  if (g.telescope)
  {
    // Removing the Register # line at the top:
    g.displayed_register = 0;
  }
  g.current_point = n - 1;
  g.reg.point[g.current_point] = g.pos_short_old;
#ifdef TEMPERATURE
  if (g.telescope)
  {
    // Measuring current temperature
    measure_temperature();
    g.reg.raw_T[g.current_point] = g.raw_T;
    g.Temp0[g.current_point] = compute_temperature(g.raw_T);
    g.delta_pos[g.current_point] = 0;
  }
#endif
  EEPROM.put( g.addr_reg[0], g.reg);
  g.msteps_per_frame = Msteps_per_frame();
  g.Nframes = Nframes();
  display_all();
  sprintf(g.buffer, "  P%1d was set  ", n);
  display_comment_line(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void goto_memory_point(byte n)
// Go to memory point # n
{
  if (g.paused)
    return;
#ifdef TEMPERATURE
  if (g.telescope)
    // Computing delta_pos based on the current temperature
    measure_temperature();
#endif
  g.current_point = n - 1;
  // Travelling to the memory point position corrected for the current temperature:
  go_to((float)(g.reg.point[g.current_point] + g.delta_pos[g.current_point]) + 0.5, g.speed_limit);
  sprintf(g.buffer, " Going to P%1d  ", n);
  display_comment_line(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


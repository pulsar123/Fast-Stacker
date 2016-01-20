float target_speed ()
// Estimating the required speed in microsteps per microsecond
{
  return SPEED_SCALE * FPS[g.i_fps] * MM_PER_FRAME[g.i_mm_per_frame];
}


float Msteps_per_frame ()
/* Computing the "microsteps per frame" parameter - redo this every time g.i_mm_per_frame changes.
 */
{
  return (MM_PER_FRAME[g.i_mm_per_frame] / MM_PER_ROTATION) * MICROSTEPS_PER_ROTATION;
}


short Nframes ()
/* Computing the "Nframes" parameter (only for 2-point stacking) - redo this every time either of
   g.msteps_per_frame, g.point1, or g.point2 changes.
 */
{
  return short(((float)(g.point2 - g.point1)) / g.msteps_per_frame) + 1;
}


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
/* Initiating a travel to pos1 at given target speed (positive number) and maximum acceleration.
   With non-zero BACKLASH constant, all go_to moves result in fully backlash-compensated moves. The
   backlash model used is the simplest possible: when starting from a good initial position
   (g.BL_counter=0), meaning the physical coordinate = program coordinate, and
   moving in the bad (negative pos) direction, the rail will not start moving until BACKLASH
   steps are done (and g.BL_counter gets the largest possible value, BACKLASH), and then it starts
   moving instantly. If switching direction, again the rail doesn't move until BACKLASH steps
   are carried out, and g.BL_counter becomes 0 (smallest possible value), and then it starts moving
   instantly. The current physical coordinate of the rail is always connected to the program
   coordinate via equation:

   pos_phys = pos_prog + g.BL_counter

 */
{
  float speed1_loc;
  byte speed_changes;

  if (g.breaking || g.backlashing)
    return;

  // Ultimate physical coordinate to achieve:
  short pos1_short = floorMy(pos1);

  // Current physical coordinate:
  short pos_short_phys = g.pos_short_old + g.BL_counter;
  float pos_phys = g.pos + (float)g.BL_counter;

  // We are already there, and no need for backlash compensation, so just returning:
  if (g.moving == 0 && pos1_short == g.pos_short_old && g.BL_counter == 0)
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
      // Overshooting by BACKLASH microsteps (this will be compensated in backlash() function after we stop):
      pos1 = pos1 - (float)BACKLASH;
      speed1_loc = -speed;
    }
  }

  else
    // We are currently moving
  {
    // Stopping distance in the current direction:
    // Breaking is always done with the maximum deceleration:
    float dx_stop = g.speed * g.speed / (2.0 * ACCEL_LIMIT);
    // Travel vector:
    float dx_vec = pos1 - g.pos;
    float dx = fabs(dx_vec);
    // Number of whole steps to take if going straight to the target:
    short dx_steps = pos1_short - g.pos_short_old;

    // All the cases when speed sign will change while traveling to the target:
    // When we move in the correct direction, but cannot stop in time because of the acceleration limit
    if (dx < dx_stop && (g.direction > 0 && g.speed > 0.0 || g.direction < 0 && g.speed <= 0.0) ||
        // or when we are moving in the wrong direction
        g.direction > 0 && g.speed <= 0.0 || g.direction < 0 && g.speed > 0.0)
      speed_changes = 1;
    else
      // In all other cases speed sign will be constant:
      speed_changes = 0;

    // Identifying all the cases when to achieve a full backlash compensation we need to use goto twice: first goto pos1-BACKLASH, then goto pos1
    // (The second goto is initiated in backlash() )
    if (
      // Case 1: Moving towards the target, in the bad (negative) direction:
      g.speed <= 0.0 && !speed_changes ||
      // Case 2: Moving in the bad direction, will have to reverse the direction to the good one, but at the end not enough to compensate for BL:
      g.speed <= 0.0 && speed_changes && floorMy(dx_stop - dx) < BACKLASH ||
      // Case 3: Initially moving in the good direction, but reverse at the end, so BL compensation is needed:
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


void set_backlight()
// Setting the LCD backlight. 2 levels for now.
{
  switch (g.backlight)
  {
    case 0:
      analogWrite(PIN_LCD_LED, 0);
      break;

    case 1:
      // For some reason intermediate backlight (127) started crashing the LCD (since I borrowed some pins)
      // Switching to two level for now
      analogWrite(PIN_LCD_LED, 255);
      break;

      //    case 2:
      //      analogWrite(PIN_LCD_LED, 255);
      //      break;
  }

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
  display_all();

  return;
}


void set_accel_v()
{
  // Five possible floating point values for acceleration
  g.accel_v[0] = -ACCEL_LIMIT;
  g.accel_v[1] = -ACCEL_LIMIT / (float)ACCEL_FACTOR[g.i_accel_factor];
  g.accel_v[2] = 0.0;
  g.accel_v[3] =  ACCEL_LIMIT / (float)ACCEL_FACTOR[g.i_accel_factor];
  g.accel_v[4] =  ACCEL_LIMIT;
  return;
}


void to_reg()
// Parameters -> to reg structure
{
  g.reg = {g.i_n_shots, g.i_mm_per_frame, g.i_fps, g.i_first_delay, g.i_second_delay, g.i_accel_factor, g.mirror_lock, g.straight, g.point1, g.point2};
  return;
}


void from_reg()
// reg structure -> parameters
{
  g.i_n_shots = g.reg.i_n_shots;
  g.i_mm_per_frame = g.reg.i_mm_per_frame;
  g.i_fps = g.reg.i_fps;
  g.point1 = g.reg.point1;
  g.point2 = g.reg.point2;
  g.i_first_delay = g.reg.i_first_delay;
  g.i_second_delay = g.reg.i_second_delay;
  g.mirror_lock = g.reg.mirror_lock;
  g.straight = g.reg.straight;
  g.i_accel_factor = g.reg.i_accel_factor;
  return;
}


void put_reg()
// Putting all parameters which are part of reg structure to EEPROM
{
  EEPROM.put( ADDR_I_N_SHOTS, g.i_n_shots);
  EEPROM.put( ADDR_I_MM_PER_FRAME, g.i_mm_per_frame);
  EEPROM.put( ADDR_I_FPS, g.i_fps);
  EEPROM.put( ADDR_I_FIRST_DELAY, g.i_first_delay);
  EEPROM.put( ADDR_I_SECOND_DELAY, g.i_second_delay);
  EEPROM.put( ADDR_I_ACCEL_FACTOR, g.i_accel_factor);
  EEPROM.put( ADDR_MIRROR_LOCK, g.mirror_lock);
  EEPROM.put( ADDR_STRAIGHT, g.straight);
  EEPROM.put( ADDR_POINT1, g.point1);
  EEPROM.put( ADDR_POINT2, g.point2);
  return;
}


void get_reg()
// Getting all parameters which are part of reg structure from EEPROM
{
  EEPROM.get( ADDR_I_N_SHOTS, g.i_n_shots);
  EEPROM.get( ADDR_I_MM_PER_FRAME, g.i_mm_per_frame);
  EEPROM.get( ADDR_I_FPS, g.i_fps);
  EEPROM.get( ADDR_I_FIRST_DELAY, g.i_first_delay);
  EEPROM.get( ADDR_I_SECOND_DELAY, g.i_second_delay);
  EEPROM.get( ADDR_I_ACCEL_FACTOR, g.i_accel_factor);
  EEPROM.get( ADDR_MIRROR_LOCK, g.mirror_lock);
  EEPROM.get( ADDR_STRAIGHT, g.straight);
  EEPROM.get( ADDR_POINT1, g.point1);
  EEPROM.get( ADDR_POINT2, g.point2);
  return;
}


void rail_reverse(byte fix_points)
/* Reversing the rail operation - either manually (*1 function), or automatically, when loading one of the memory registers
   If fix_points=1, update the current point1,2 accordingly.
 */
{
  short d_pos, pos_target;

  // We need to do a full backlash compensation loop when reversing the rail operation:
  g.BL_counter = BACKLASH;
  // This will instruct the backlash module to do BACKLASH_2 travel at the end, to compensate for BL in reveresed coordinates
  g.backlash_init = 2;
  d_pos = g.limit1 + g.limit2 + BACKLASH - BACKLASH_2;
  // Updating the current coordinate in the new (reversed) frame of reference:
  g.pos = d_pos - g.pos;
  g.pos0 = g.pos;
  g.pos_old = g.pos;
  g.pos_short_old = floorMy(g.pos);
  if (fix_points)
  {
    // Updating the current two points positions:
    pos_target = d_pos - g.point2;
    g.point2 = d_pos - g.point1;
    g.point1 = pos_target;
  }

  return;
}


short frame_coordinate()
// Coordinate (short type) of a frame given by g.frame_number, in 2-point stacking
{
  return g.starting_point + nintMy(((float)g.frame_counter) * g.msteps_per_frame);
}


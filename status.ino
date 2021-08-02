/*
  All LCD related functions
*/

void display_all()
/*
  Refreshing the whole screen
*/
{
  float p;
  byte row, col;

#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif

  tft.fillScreen(TFT_BLACK);
  my_setCursor(0, 0, 1);

#ifdef TEST_SWITCH
  byte i;
  float xcount;
  // The switch on section
  i = 0;
  my_setCursor(0, 0, 1);
  xcount = ((float)g.count[i]) / ((float)g.test_N);
  // Averge number of triggers, average coordinate of the first trigger:
  sprintf(g.buffer, "%4s %9s", ftoa(g.buf6, xcount, 2), ftoa(g.buf9, g.test_avr[i], 2));
  //  sprintf(g.buffer, "%5d", g.limit1);
  tft.print(g.buffer);
  my_setCursor(0, 1, 1);
  // Half of total deviation and std for the first trigger:
  sprintf(g.buffer, "%6s %6s", ftoa(g.buf7, g.test_dev[i], 2), ftoa(g.buf6, g.test_std[i], 2));
  tft.print(g.buffer);
  // The switch off section
  i = 1;
  my_setCursor(0, 2, 1);
  if (g.test_N > 0)
    xcount = ((float)g.count[i]) / ((float)g.test_N - 1);
  else
    xcount = 0.0;
  sprintf(g.buffer, "%4s %9s", ftoa(g.buf6, xcount, 2), ftoa(g.buf9, g.test_avr[i], 2));
  tft.print(g.buffer);
  my_setCursor(0, 3, 1);
  sprintf(g.buffer, "%6s %6s", ftoa(g.buf7, g.test_dev[i], 2), ftoa(g.buf6, g.test_std[i], 2));
  tft.print(g.buffer);
  my_setCursor(0, 4, 1);
  sprintf(g.buffer, "%2d %9s", g.test_N, ftoa(g.buf9, g.test_limit, 2));
  tft.print(g.buffer);
  display_status_line();
  return;
#endif


  if (g.alt_flag)
  {
    if (g.alt_kind == 1)
      // Screen "*"
    {
      // Line 1:
      my_setCursor(7, 0, 1);
      sprintf(g.buffer, "Acc=%1d", ACCEL_FACTOR[g.reg.i_accel_factor]);
      tft.print(g.buffer);
      sprintf(g.buf6, "Rev=%1d", 1 - g.reg.straight);
      my_setCursor(0, 0, 1);
      tft.print(g.buf6);

      // Line 2:
      sprintf(g.buffer, "N=%-3d", N_TIMELAPSE[g.reg.i_n_timelapse]);
      my_setCursor(0, 1, 1);
      tft.print(g.buffer);
      sprintf(g.buffer, "BL=%1d", g.reg.backlash_on);
      my_setCursor(7, 1, 1);
      tft.print(g.buffer);
      // Line 3:
      my_setCursor(0, 2, 1);
      sprintf(g.buf6, "dt=%ds", DT_TIMELAPSE[g.reg.i_dt_timelapse]);
      tft.print(g.buf6);
      sprintf(g.buf6, "Mir");
      sprintf(g.buffer, "%3s=%1d", g.buf6, g.reg.mirror_lock);
      my_setCursor(7, 2, 1);
      tft.print(g.buffer);
      // Line 4:
      sprintf(g.buffer, "Save=%1d        ", g.reg.save_energy);
      my_setCursor(0, 3, 1);
      tft.print(g.buffer);
      // Line 5:
      //    tft.print("              ");
      my_setCursor(0, 5, 1);
      // Line 6:
      sprintf(g.buffer, "         s%s", VERSION);

#ifdef TIMING
      // Average loop length for the last motion, in shortest miscrostep length units *100:
      int avr = (100.0 * (float)g.total_dt_timing / (float)(g.i_timing - 1) * SPEED_LIMIT);
      // Maximum/minimum loop lenght in the above units:
      int max1 = (100.0 * (float)(g.dt_max) * SPEED_LIMIT);
      int min1 = (100.0 * (float)(g.dt_min) * SPEED_LIMIT);
      sprintf(g.buffer, "%4d %4d %4d", min1, avr, max1);
      my_setCursor(0, 4, 1);
      tft.print(g.buffer);

      // Maximum number of steps requested in one loop (should be 1), number of loops with d>1, total number of loops, number of times when PRECISE_STEPPING sanity check failed:
      sprintf(g.buffer, "%3d %4d %6d %4d", g.d_max, g.d_Nbad, g.d_N, g.N_insanity);
      my_setCursor(0, 6, 1);
      tft.print(g.buffer);

      // How many times arduino loop was longer than the shortest microstep time interval; total number of arduino loops:
      sprintf(g.buffer, "%4d %8d   ", g.bad_timing_counter, g.i_timing);
      my_setCursor(0, 5, 1);
      tft.print(g.buffer);
#ifdef MOTOR_DEBUG
      //  sprintf(g.buffer, "%4d %4d %4d", cplus2, cmax, imax);
      //  my_setCursor(0, 3);
      //  tft.print(g.buffer);
#endif
#endif

#ifdef SHOW_EEPROM
      //  Showing amount of EEPROM used:
      sprintf(g.buffer, "%4d s%s", ADDR_END, VERSION);
#else
#endif  // EEPROM
      tft.print(g.buffer);
    }

  }

  else
  {
    // Error code displaying:
    switch (g.error)
    {
      case 0:  // No error
        display_u_per_f();  display_fps();
        display_one_point_params();
        display_two_point_params();
        display_two_points();
        display_current_position();
        display_status_line();
        break;

      case 1:
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.print("Cable disconnected, or limiter is on!");
        tft.print("Only if cable is connected, rewind");
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        break;

      case 2: // Critically low battery level; not used when debugging
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.print("Critically low battery\n level!");
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        break;

      case 3:  // Factory reset initiated
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.print("Factory reset?\nPress \"1\" to start");
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        break;

      case 4:  // Calibration initiated
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        tft.print("  Calibration\n   required!\n");
        tft.print("\n Press any key\n to start calibration");
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        break;

    } // case
  }  // if alt_flag
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void letter_status(char const * l)
/*
  Display a letter code "l" at the beginning of the status line
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;
  my_setCursor(0, 5, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (g.paused)
    tft.print("P");
  else if (g.timelapse_mode)
    tft.print("T");
  else
    tft.print(*l);
  tft.print(' ');
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void motion_status()
/*
  Motion status: 2-char status showing the direction and speed (rewind vs. focus stacking)
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;
  uint8_t i;
  
  byte motion_status_new = STATUS_NONE;

  if (g.moving == 0 && g.model_init == 0)
  {
    motion_status_new = STATUS_NONE;
  }
  else
  {
    if (g.direction == -1)
    {
      if (g.stacker_mode < 2)
        motion_status_new = STATUS_REWIND;
      else
        motion_status_new = STATUS_REVERSE;
    }
    else
    {
      if (g.stacker_mode < 2)
        motion_status_new = STATUS_FORWARD;
      else
        motion_status_new = STATUS_STRAIGHT;
    }
  }

  // Only if the motion status changed since the last check, we do the expensive operation - updating th display:
  if (motion_status_new != g.motion_status_code)
  {
    my_setCursor(2, 5, 1);
    g.motion_status_code = motion_status_new;

    tft.print("   ");

    switch (g.motion_status_code)
    {
//      case STATUS_NONE:
//        tft.print("   ");
//        break;

      case STATUS_REWIND:
        tft.drawBitmap(g.x0, g.y0 + DEL_BITMAP, rewind_char, 3 * FONT_WIDTH, FONT_HEIGHT, TFT_WHITE);
        break;

      case STATUS_REVERSE:
        tft.drawBitmap(g.x0, g.y0 + DEL_BITMAP, reverse_char, 3 * FONT_WIDTH, FONT_HEIGHT, TFT_WHITE);
        break;
        
      case STATUS_FORWARD:
        tft.drawBitmap(g.x0, g.y0 + DEL_BITMAP, forward_char, 3 * FONT_WIDTH, FONT_HEIGHT, TFT_WHITE);
        break;
        
      case STATUS_STRAIGHT:
        tft.drawBitmap(g.x0, g.y0 + DEL_BITMAP, straight_char, 3 * FONT_WIDTH, FONT_HEIGHT, TFT_WHITE);
        break;
    }
  }



  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_frame_counter()
/*
  Printing the current stacking frame number in the status line.
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;
  // Printing frame counter:
  if (g.stacker_mode == 0 && g.paused == 0 || g.paused > 1)
    sprintf (g.buffer, "   0 ");
  else
    sprintf (g.buffer, "%4d ",  g.frame_counter + 1);
  my_setCursor(5, 5, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //  tft.print (g.buffer);
  tft.drawString(g.buffer, g.x0, g.y0);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  return;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void points_status()
/* Displays F or B if the current coordinate is exactly the foreground (g.reg.point[FOREGROUND]) or background (g.reg.point[BACKGROUND]) point.
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;

  my_setCursor(10, 5, 1);

  if (g.current_point < 0)
  {
    tft.print("  ");
    return;
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  if (g.current_point == 0)
    tft.print("F ");
  else
    tft.print("B ");

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void battery_status()
/*
  Measuring the battery voltage and displaying it.
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.moving == 1 || g.model_init == 1 || g.alt_flag)
    return;

  // Battery voltage (per AA battery; assuming 8 batteries) measured via a two-resistor voltage devider
  // (to reduce voltage from 12V -> 5V)
  // Slow operation (100 us), so should be done infrequently
  float V = (float)analogRead(PIN_BATTERY) * VOLTAGE_SCALER;

  if (g.error)
    return;

#ifdef BATTERY_DEBUG
  // Printing actual voltage per AA battery (times 100)
  my_setCursor(11, 5, 1);
  sprintf(g.buffer, "%3d", analogRead(PIN_BATTERY));
  tft.print(g.buffer);
#else
  my_setCursor(12, 5, 0);  // 12

  // A 5-level bitmap indication (between V_LOW and V_HIGH):
  short level = (short)((V - V_LOW) / (V_HIGH - V_LOW) * 5.0);
  if (level < 0)
    level = 0;
  if (level > 4)
    level = 4;
  tft.drawBitmap(g.x0, g.y0 + DEL_BITMAP, battery_char[level], 2 * FONT_WIDTH, FONT_HEIGHT, TFT_WHITE);
  //tft.print(V);

#endif // BATTERY_DEBUG

  // Disabling the rail once V goes below the critical V_LOW voltage
#ifndef MOTOR_DEBUG
#ifndef NO_CRITICAL_VOLTAGE
  if (V < V_LOW)
    g.error = 2;
#endif
#endif

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_status_line()
/*
  Display the whole status line
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;
  letter_status(" ");
  motion_status();
  display_frame_counter();
  points_status();
  battery_status();
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_u_per_f()
/*
  Display the input parameter u per frame (1000*MM_PER_FRAME)
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;

  float um_per_frame = 1e3 * MM_PER_MICROSTEP * MSTEP_PER_FRAME[g.reg.i_mm_per_frame];

  if (um_per_frame >= 10.0)
    // +0.5 is for proper round-off:
    sprintf(g.buffer, "%4duf ", (int)(um_per_frame + 0.5));
  else
    // +0.05 is for proper round-off:
    sprintf(g.buffer, "%4suf ", ftoa(g.buf7, um_per_frame + 0.05, 1));

  my_setCursor(0, 2, 1);
  tft.print(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_fps()
/*
  Display the input parameter fps (frames per second)
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;
  if (FPS[g.reg.i_fps] >= 1.0)
    sprintf(g.buffer, " %3sfps ", ftoa(g.buf7, FPS[g.reg.i_fps], 1));
  else
    sprintf(g.buffer, "%4sfps ", ftoa(g.buf7, FPS[g.reg.i_fps], 2));

  my_setCursor(7, 2, 1);
  tft.print(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_one_point_params()
/*
  Display the three parameters for one-point shooting:
  - input parameter N_SHOTS,
  - travel distance, mm,
  - travel time, s.
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;

  my_setCursor(0, 0, 1);

  // +0.05 for proper round off:
  float dx = (float)(N_SHOTS[g.reg.i_n_shots] - 1) * MM_PER_MICROSTEP * MSTEP_PER_FRAME[g.reg.i_mm_per_frame] + 0.05;
  short dt = (short)roundMy((float)(N_SHOTS[g.reg.i_n_shots] - 1) / FPS[g.reg.i_fps]);
  if (dt < 1000.0 && dt >= 0.0)
    sprintf(g.buf6, "%3ds", dt);
  else if (dt < 10000.0 && dt >= 0.0)
    sprintf(g.buf6, "%4d", dt);
  else
    sprintf(g.buf6, "****");

  if (dx < 100.0)
    ftoa(g.buf7, dx, 1);
  else
    sprintf(g.buf7, "****");

  sprintf(g.buffer, "%4d %4s    %4s   ", N_SHOTS[g.reg.i_n_shots], g.buf7 , g.buf6);

  tft.print(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_two_point_params()
/*
  Display the three parameters for two-point shooting:
  - number of shots,
  - travel distance, mm,
  - travel time, s.
*/
{
  float dx;
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;

  my_setCursor(0, 1, 1);

  // +0.05 for proper round off:
  dx = MM_PER_MICROSTEP * (float)(g.reg.point[BACKGROUND] - g.reg.point[FOREGROUND]) + 0.05;
  // +0.5 for proper round off:
  short dt = (short)((float)(g.Nframes - 1) / FPS[g.reg.i_fps] + 0.5);
  if (dt < 1000.0 && dt >= 0.0)
    sprintf(g.buf6, "%3ds", dt);
  else if (dt < 10000.0 && dt >= 0.0)
    sprintf(g.buf6, "%4d", dt);
  else
    sprintf(g.buf6, "****");

  if (g.reg.point[BACKGROUND] >= g.reg.point[FOREGROUND])
    sprintf(g.buffer, "%4d  %4s   %4s   ", g.Nframes, ftoa(g.buf7, dx, 1), g.buf6);
  else
    sprintf(g.buffer, "****  ****   ****   ");
  tft.print(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_two_points()
/*
  Display the positions (in mm) of two points: foreground, F, and background, B.
*/
{
  float p;
  COORD_TYPE p_int;

#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;

#ifdef TEST_SWITCH
  return;
#endif

  my_setCursor(0, 3, 1);
  tft.print(g.empty_buffer);  //erasing the line

#ifdef SHOW_RAW
  sprintf(g.buffer, "F%5d", g.reg.point[FOREGROUND]);
#else
  p = MM_PER_MICROSTEP * (float)(g.reg.point[FOREGROUND]);
  if (p >= 0.0)
    sprintf(g.buffer, "F%s", ftoa(g.buf7, p, 2));
  else
    sprintf(g.buffer, "F*****");
#endif
  my_setCursor(0, 3, 1);
  tft.print(g.buffer);

#ifdef SHOW_RAW
  sprintf(g.buffer, "B%5d", g.reg.point[BACKGROUND]);
#else
  p = MM_PER_MICROSTEP * (float)(g.reg.point[BACKGROUND]);
  if (p >= 0.0)
    sprintf(g.buffer, "B%s", ftoa(g.buf7, p, 2));
  else
    sprintf(g.buffer, "B*****");
#endif
  my_setCursor(8, 3, 1);
  tft.print(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_current_position()
/*
  Display the current position on the transient line
*/
{
  float p;
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
#ifdef CAMERA_DEBUG
  //  return;
#endif
#ifdef TEST_SWITCH
  return;
#endif

  //!!! added  g.moving==1 || g.model_init==1
  //  if (g.error || g.moving == 0 && g.BL_counter > 0 || g.alt_flag || g.moving==1 || g.model_init==1)
  if (g.error || g.moving == 0 && g.BL_counter > 0 || g.alt_flag)
    return;

  if (g.reg.straight)
    g.tmp_char = ' ';
  else
    g.tmp_char = 'R';

  if (g.timelapse_mode)
    sprintf(g.buf6, "%3d", g.timelapse_counter + 1);
  else
    sprintf(g.buf6, "   ");

#if defined(BL_DEBUG)
  // When debugging backlash, displays the current backlash value in microsteps
  sprintf(g.buf6, "%3d", g.backlash);
#elif defined(BL2_DEBUG)
  // When debugging BACKLASH_2, displays the current BACKLAS_2 value in microsteps
  sprintf(g.buf6, "%3d", BACKLASH_2);
#elif defined(DELAY_DEBUG)
  // Delay used in mirror_lock=2 mode (electronic shutter), in 10ms units:
  sprintf(g.buf6, "%3d", SHUTTER_ON_DELAY2 / 10000);
#elif defined(BUZZER_DEBUG)
  // When debugging buzzer, displays the current g.dt1_buzz_us value in microseconds
  sprintf(g.buf6, "%3d", g.dt1_buzz_us);
#endif

#ifdef SHOW_RAW
  sprintf(g.buffer, "  %c  %6d    %3s  ", g.tmp_char, g.ipos, g.buf6);
#else
  p = MM_PER_MICROSTEP * g.ipos;
  sprintf(g.buffer, "  %c  %6smm  %3s  ", g.tmp_char, ftoa(g.buf7, p, 3), g.buf6);
#endif

#ifdef TEST_LIMITER
  sprintf(g.buffer, "%d", g.limiter_i);
#endif

  my_setCursor(0, 4, 1);
  tft.print(g.buffer);

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_comment_line(char const * l)
/*
  Display a comment line briefly (then it should be replaced with display_current_position() output).
  The char array *l should always have the length of the line + 1 (21), to ensure the older conent is erased
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
#ifdef CAMERA_DEBUG
  return;
#endif
  if (g.error)
    return;
  my_setCursor(0, 4, 1);
  tft.print(l);
  g.t_comment = g.t;
  g.comment_flag = 1;
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void delay_buffer()
// Fill g.buffer with non-continuous stacking parameters, to be displayed with display_comment_line:
{
  float y;
  y = (float)MSTEP_PER_FRAME[g.reg.i_mm_per_frame] / ACCEL_LIMIT;
  // Time to travel one frame (s), with fixed acceleration:
  float dt_goto = 2e-6 * sqrt(y);
  float delay1 = FIRST_DELAY[g.reg.i_first_delay];
  float delay2 = SECOND_DELAY[g.reg.i_second_delay];
  // +0.5 for roundoff:
  short dt = (short)((float)(g.Nframes) * (FIRST_DELAY[g.reg.i_first_delay] + SECOND_DELAY[g.reg.i_second_delay]) + (float)(g.Nframes - 1) * dt_goto + 0.5);
  sprintf(g.buffer, " %4s   %4s   %4d ", ftoa(g.buf7, delay1, 1), ftoa(g.buf6, delay2, 1), dt);  // 20 chars long

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#ifdef CAMERA_DEBUG
void AF_status(short s)
{
  my_setCursor(12, 4, 1);
  tft.print(s);
  return;
}
void shutter_status(short s)
{
  my_setCursor(13, 4, 1);
  tft.print(s);
  return;
}
#endif

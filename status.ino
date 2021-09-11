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

  if (g.help_mode)
    return;

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
  sprintf(g.buffer, "%6s %6s", ftoa(g.buf10, g.test_dev[i], 2), ftoa(g.buf6, g.test_std[i], 2));
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
  sprintf(g.buffer, "%6s %6s", ftoa(g.buf10, g.test_dev[i], 2), ftoa(g.buf6, g.test_std[i], 2));
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
      const byte shift = 10;
      const byte del = 2;
      byte line;
      //---------------------------------------------
      line = 0;
      my_setCursor(0, line, 1);
      tft.setTextColor(TFT_BLACK, TFT_ORANGE);
      tft.print("1");
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      if (g.reg.straight == 1)
        sprintf(g.buf10, "Rev=Off");
      else
        sprintf(g.buf10, "Rev=On");
      my_setCursor(del, line, 1);
      tft.print(g.buf10);

      my_setCursor(shift, line, 1);
      tft.setTextColor(TFT_BLACK, TFT_ORANGE);
      tft.print("A");
      my_setCursor(shift + del, line, 1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      sprintf(g.buffer, "Acc=%1d", ACCEL_FACTOR[g.reg.i_accel_factor]);
      tft.print(g.buffer);

      //---------------------------------------------
      line = 1;
      my_setCursor(0, line, 1);
      tft.setTextColor(TFT_BLACK, TFT_ORANGE);
      tft.print("4");
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      sprintf(g.buffer, "N=%-3d", g.reg.n_timelapse);
      my_setCursor(del, line, 1);
      tft.print(g.buffer);

      my_setCursor(shift, line, 1);
      tft.setTextColor(TFT_BLACK, TFT_ORANGE);
      tft.print("B");
      my_setCursor(shift + del, line, 1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      if (g.reg.backlash_on == -1)
        sprintf(g.buffer, "BL=Down");
      else if (g.reg.backlash_on == 0)
        sprintf(g.buffer, "BL=Off ");
      else
        sprintf(g.buffer, "BL=Up  ");
      tft.print(g.buffer);

      //---------------------------------------------
      line = 2;
      my_setCursor(0, line, 1);
      tft.setTextColor(TFT_BLACK, TFT_ORANGE);
      tft.print("7");
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      my_setCursor(del, line, 1);
      sprintf(g.buf6, "dt=%ds", (int)g.reg.dt_timelapse);
      tft.print(g.buf6);

      my_setCursor(shift, line, 1);
      tft.setTextColor(TFT_BLACK, TFT_ORANGE);
      tft.print("C");
      my_setCursor(shift + del, line, 1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      if (g.reg.mirror_lock == MIRROR_OFF)
        sprintf(g.buffer, "Mir=Off");
      else if (g.reg.mirror_lock == MIRROR_ON)
        sprintf(g.buffer, "Mir=On");
      else if (g.reg.mirror_lock == MIRROR_FRSP)
        sprintf(g.buffer, "Mir=FRSP");
      else if (g.reg.mirror_lock == MIRROR_BURST)
        sprintf(g.buffer, "Mir=Brst");
      tft.print(g.buffer);

      //---------------------------------------------
      line = 3;
      my_setCursor(0, line, 1);
      tft.setTextColor(TFT_BLACK, TFT_ORANGE);
      tft.print("0");
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      my_setCursor(del, line, 1);
      if (g.reg.save_energy == 0)
        sprintf(g.buffer, "Save=Off");
      else
        sprintf(g.buffer, "Save=On ");
      tft.print(g.buffer);

      my_setCursor(shift, line, 1);
      tft.setTextColor(TFT_BLACK, TFT_ORANGE);
      tft.print("D");
      my_setCursor(shift + del, line, 1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      if (g.reg.buzzer == 0)
        sprintf(g.buffer, "Buzz=Off");
      else
        sprintf(g.buffer, "Buzz=On");
      tft.print(g.buffer);

      //---------------------------------------------
      line = 4;

      my_setCursor(shift, line, 1);
      tft.setTextColor(TFT_BLACK, TFT_ORANGE);
      tft.print("9");
      my_setCursor(shift + del, line, 1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      sprintf(g.buffer, "Acc2=%1d", ACCEL_FACTOR[g.reg.i_accel_factor2]);
      tft.print(g.buffer);

      //---------------------------------------------
      line = 6;
      my_setCursor(3, line, 1);
      sprintf(g.buffer, "Fast Stacker s%s", VERSION);

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
    if (g.error > 0)
      tft.setTextColor(TFT_RED, TFT_BLACK);

    switch (g.error)
    {
      case 0:  // No error, normal display
        g.refresh = 1;
        display_mode();
        display_step();
        display_third_line();
        display_derivatives();
        display_two_points();
        display_current_position(0);
        display_status_line();
        g.refresh = 0;
        break;

      case 1:
        tft.print("Cable disconnected, or limiter is on!");
        tft.print("Only if cable is connected, rewind");
        break;

      case 2: // Critically low battery level; not used when debugging
        tft.print("Critically low battery\n level!");
        break;

      case 3:  // Factory reset initiated
        tft.print("Factory reset?\nPress \"1\" to start");
        break;

      case 4:  // Calibration initiated
        tft.print("  Calibration\n   required!\n");
        tft.print("\n Press any key\n to start calibration");
        break;

    } // case
  }  // if alt_flag
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


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void display_mode()
/*
  Display the mode (top line)
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;

  my_setCursor(0, 0, 1);
  //  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  if (g.reg.i_mode == ONE_SHOT_MODE)
  {
    tft.fillRect(0, 0, 160, 16, 0xFA08);
    tft.setTextColor(TFT_BLACK, 0xFA08);
    tft.print("    One point cont     ");
  }
  else if (g.reg.i_mode == CONT_MODE)
  {
    tft.fillRect(0, 0, 160, 16, 0x47E8);
    tft.setTextColor(TFT_BLACK, 0x47E8);
    tft.print("   Two points cont     ");
  }
  else if (g.reg.i_mode == NONCONT_MODE)
  {
    tft.fillRect(0, 0, 160, 16, 0x421F);
    tft.setTextColor(TFT_BLACK, 0x421F);
    tft.print("  Two points non-cont  ");
  }

  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void display_step()
/*
  Display step size (second line)
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;

  my_setCursor(0, 1, 1);
  tft.setTextColor(TFT_BLACK, TFT_ORANGE);
  tft.print("5");

  my_setCursor(1, 1, 1);
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);

  float step_um = 1e3 * MM_PER_MICROSTEP * g.reg.mstep;

  if (step_um >= 10.0)
    // +0.5 is for proper round-off:
    sprintf(g.buffer, " Step=%4dum       ", (int)(step_um + 0.5));
  else
    // +0.05 is for proper round-off:
    sprintf(g.buffer, " Step=%4sum       ", ftoa(g.buf10, step_um + 0.05, 1));

  tft.print(g.buffer);

  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void display_third_line()
/*
  Display third line
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;

  if (g.reg.i_mode == ONE_SHOT_MODE)
  {
    my_setCursor(0, 2, 1);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.print("8");
    my_setCursor(1, 2, 1);
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    if (g.reg.fps >= 1.0)
      sprintf(g.buffer, " FPS=%4s", ftoa(g.buf10, g.reg.fps, 1));
    else
      sprintf(g.buffer, " FPS=%4s", ftoa(g.buf10, g.reg.fps, 2));
    tft.print(g.buffer);

    my_setCursor(10, 2, 1);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.print("9");
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    sprintf(g.buffer, " N=%4d           ", g.reg.n_shots);
    tft.print(g.buffer);
  }
  else if (g.reg.i_mode == CONT_MODE)
  {
    my_setCursor(0, 2, 1);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.print("8");
    my_setCursor(1, 2, 1);
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    if (g.reg.fps >= 1.0)
      sprintf(g.buffer, " FPS=%4s          ", ftoa(g.buf10, g.reg.fps, 1));
    else
      sprintf(g.buffer, " FPS=%4s          ", ftoa(g.buf10, g.reg.fps, 2));
    tft.print(g.buffer);
  }
  else if (g.reg.i_mode == NONCONT_MODE)
  {
    my_setCursor(0, 2, 1);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.print("8");
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    sprintf(g.buffer, " d1=%3ss ", ftoa(g.buf10, g.reg.first_delay + 0.05, 1));
    tft.print(g.buffer);

    my_setCursor(10, 2, 1);
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.print("9");
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    sprintf(g.buffer, " d2=%3ss ", ftoa(g.buf10, g.reg.second_delay + 0.05, 1));
    tft.print(g.buffer);
  }

  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void display_derivatives()
/*
  Display derivatives - number of shots, total time (line #4)
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.error || g.alt_flag)
    return;

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  my_setCursor(0, 3, 1);

  if (g.reg.i_mode == ONE_SHOT_MODE)
  {
    // +0.05 for proper round off:
    float dx = (float)(g.reg.n_shots - 1) * MM_PER_MICROSTEP * g.reg.mstep + 0.05;
    if (dx < 1000.0)
      ftoa(g.buf10, dx, 2);
    else
      sprintf(g.buf10, "******");

    int dt = roundMy((float)(g.reg.n_shots - 1) / g.reg.fps);
    if (dt < 10000 && dt >= 0)
      sprintf(g.buf6, "%4d", dt);
    else
      sprintf(g.buf6, "****");

    sprintf(g.buffer, "d=%6s  t=%4ss   ", g.buf10 , g.buf6);
  }
  else
  {
    TIME_STYPE dt;
    if (g.reg.i_mode == CONT_MODE)
    {
      // +0.5 for proper round off:
      dt = (float)(g.Nframes - 1) / g.reg.fps + 0.5;
    }
    else
    {
      // Time to travel one frame (s), with fixed acceleration:
      float dt_goto = 1e-6 * sqrt((float)g.reg.mstep / g.accel_v2);
      // +0.5 for roundoff:
      dt = (float)(g.Nframes) * (g.reg.first_delay + g.reg.second_delay) + (float)(g.Nframes - 1) * dt_goto + 0.5;
    }

    if (dt < 100000 && dt >= 0)
      sprintf(g.buf6, "%5d", dt);
    else
      sprintf(g.buf6, "*****");

    if (g.reg.point[BACKGROUND] >= g.reg.point[FOREGROUND])
      sprintf(g.buffer, "N=%4d    t=%5ss   ", g.Nframes, g.buf6);
    else
      sprintf(g.buffer, "N=****    t=*****   ");
  }




  tft.print(g.buffer);

  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_two_points()
/*
  Display the positions (in mm) of two points: foreground, F, and background, B. Line #5.
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

  byte line = 4;
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  my_setCursor(0, line, 1);
  tft.print(g.empty_buffer);  //erasing the line

#ifdef SHOW_RAW
  sprintf(g.buffer, "F%5d", g.reg.point[FOREGROUND]);
#else
  p = MM_PER_MICROSTEP * (float)(g.reg.point[FOREGROUND]);
  if (p >= 0.0)
    sprintf(g.buffer, "F%s", ftoa(g.buf10, p, 3));
  else
    sprintf(g.buffer, "F*****");
#endif
  my_setCursor(0, line, 1);
  tft.print(g.buffer);

#ifdef SHOW_RAW
  sprintf(g.buffer, "B%5d", g.reg.point[BACKGROUND]);
#else
  p = MM_PER_MICROSTEP * (float)(g.reg.point[BACKGROUND]);
  if (p >= 0.0)
    sprintf(g.buffer, "B%s", ftoa(g.buf10, p, 3));
  else
    sprintf(g.buffer, "B*****");
#endif
  my_setCursor(10, line, 1);
  tft.print(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_current_position(byte only_position)
/*
  Display the current position on the transient line (#6)
  If only_position !=0, printing only the position.
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

  //  if (g.error || g.moving == 0 && g.reg.backlash_on * g.BL_counter > 0 || g.alt_flag)
  //    return;

  if (g.error || g.alt_flag || g.editing || g.Backlashing)
    return;

  if (only_position)
    // Update only the position, and only if it changed
  {
    if (g.ipos != g.ipos_printed)
    {
      float speed = fabs(current_speed());
      if (speed < SPEED_LIMIT * POS_SPEED_FRACTION)
        // A hack; only if the current speed is small enough, update the current displayed position while moving (to minimize noise)
      {
        p = MM_PER_MICROSTEP * g.ipos;
        my_setCursor(2, 5, 0);
        if (g.reg.straight)
          g.x0 += 2;
        else
          g.x0 += 4;
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        sprintf(g.buffer, "%8smm   ", ftoa(g.buf10, p, 4));
        tft.drawString(g.buffer, g.x0, g.y0);
      }
      g.ipos_printed = g.ipos;
      return;
    }
  }

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
  // Delay used in mirror_lock=2 mode (electronic shutter), in ms units:
  sprintf(g.buf6, "%4d", SHUTTER_ON_DELAY2 / 1000);
#endif

#ifdef SHOW_RAW
  sprintf(g.buffer, "  %c  %6d    %3s  ", g.tmp_char, g.ipos, g.buf6);
#else
  p = MM_PER_MICROSTEP * g.ipos;
  sprintf(g.buffer, "%c  %8smm  %3s  ", g.tmp_char, ftoa(g.buf10, p, 4), g.buf6);
#endif

#ifdef TEST_LIMITER
  sprintf(g.buffer, "%d", g.limiter_i);
#endif

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  my_setCursor(0, 5, 1);
  if (g.comment_flag == 1)
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print(g.buf_comment);
  }
  else
    tft.print(g.buffer);

  g.ipos_printed = g.ipos;

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
  if (g.error || g.alt_flag)
    return;

  tft.setTextColor(TFT_RED, TFT_BLACK);
  my_setCursor(0, 5, 1);
  tft.print(g.empty_buffer);
  my_setCursor(0, 5, 1);
  tft.print(l);
  strcpy(g.buf_comment, l);
  g.t_comment = g.t;
  g.comment_flag = 1;
  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_status_line()
/*
  Display the whole status line (line #7)
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
  points_status(0);
  battery_status(0);
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
  my_setCursor(0, 6, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (g.paused)
    tft.print("P");
  else if (g.timelapse_mode)
    tft.print("T");
  else
    tft.print(*l);
  tft.print(' ');
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
    my_setCursor(2, 6, 1);
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
  my_setCursor(8, 6, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //  tft.print (g.buffer);
  tft.drawString(g.buffer, g.x0, g.y0);

  return;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void points_status(byte clear)
/* Displays F or B if the current coordinate is exactly the foreground (g.reg.point[FOREGROUND]) or background (g.reg.point[BACKGROUND]) point.
   Clear the area if clear=1
*/
{
#ifdef NO_DISP
  if (g.model_init)
    return;
#endif
  if (g.error || g.alt_flag || g.editing || g.moving)
    return;

  my_setCursor(14, 6, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  if (clear)
  {
    tft.print("  ");
  }
  else
  {
    if (g.ipos == g.reg.point[FOREGROUND])
    {
      tft.print("F ");
    }
    else if (g.ipos == g.reg.point[BACKGROUND])
    {
      tft.print("B ");
    }
    else
    {
      tft.print("  ");
    }
  }

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void battery_status(byte only_if_changed)
/*
  Measuring the battery voltage and displaying it (only if changed, if only_if_changed=1).
*/
{
#ifdef NO_DISP
  if (g.moving || g.model_init)
    return;
#endif
  if (g.moving == 1 || g.model_init == 1 || g.alt_flag)
    return;

  // Battery voltage (per AA battery; assuming 8 batteries) measured via a two-resistor voltage devider
  // (to reduce voltage from 12V -> <3.3V)
  // Slow operation (100 us), so should be done infrequently
  float V = (float)analogRead(PIN_BATTERY) * VOLTAGE_SCALER;

  if (g.error)
    return;

  // Disabling the rail once V goes below the critical V_LOW voltage
#ifndef MOTOR_DEBUG
#ifndef NO_CRITICAL_VOLTAGE
  if (V < V_LOW)
    g.error = 2;
#endif
#endif

#ifdef BATTERY_DEBUG
  // Printing raw voltage measurement
  my_setCursor(15, 6, 1);
  sprintf(g.buffer, "%3d", analogRead(PIN_BATTERY));
  tft.print(g.buffer);
#else
  my_setCursor(17, 6, 0);  // 12


  // A 5-level bitmap indication (between V_LOW and V_HIGH):
  int level = (int)((V - V_LOW) / (V_HIGH - V_LOW) * 5.0);
  if (level < 0)
    level = 0;
  if (level > 4)
    level = 4;

  // Detecting if we are using AC:
  if (V > V_THRESHOLD)
    level = 5;  // Special value of level for AC power

  if (only_if_changed && level == g.level_old)
    return;

  g.level_old = level;

  int color;
  if (level == 0)
    color = TFT_RED;
  else if (level == 1)
    color = TFT_ORANGE;
  else if (level == 2)
    color = TFT_YELLOW;
  else
    color = TFT_WHITE;

  tft.fillRect(g.x0, g.y0 + DEL_BITMAP, 2 * FONT_WIDTH, FONT_HEIGHT, TFT_BLACK);
  if (level < 5)
    tft.drawBitmap(g.x0, g.y0 + DEL_BITMAP, battery_char[level], 2 * FONT_WIDTH, FONT_HEIGHT, color);
  else
    tft.drawBitmap(g.x0, g.y0 + DEL_BITMAP, AC_char, 2 * FONT_WIDTH, FONT_HEIGHT, color);


#endif // BATTERY_DEBUG


  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


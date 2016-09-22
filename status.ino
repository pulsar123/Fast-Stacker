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
#ifdef TIMING
  return;
#endif

  lcd.clear();
  lcd.setCursor(0, 0);

  if (g.alt_flag)
  {
    if (g.alt_kind == 1)
      // Screen "*"
    {
      // Line 1:
      if (g.telescope)
        sprintf(g.buffer, "         Acc=%1d", ACCEL_FACTOR[g.reg.i_accel_factor]);
      else
        sprintf(g.buffer, "Rev=%1d    Acc=%1d", 1 - g.reg.straight, ACCEL_FACTOR[g.reg.i_accel_factor]);
      lcd.print(g.buffer);

      // Line 2:
      if (g.telescope)
        sprintf(g.buffer, "Lock=%1d    BL=%1d", g.locked[g.ireg - 1], g.reg.backlash_on);
      else
        sprintf(g.buffer, "N=%-3d     BL=%1d", N_TIMELAPSE[g.reg.i_n_timelapse], g.reg.backlash_on);
      lcd.print(g.buffer);
      // Line 3:
      if (!g.telescope)
      {
        sprintf(g.buf6, "dt=%ds", DT_TIMELAPSE[g.reg.i_dt_timelapse]);
        lcd.print(g.buf6);
      }
      if (g.telescope)
        sprintf(g.buf6, "TC");
      else
        sprintf(g.buf6, "Mir");
      sprintf(g.buffer, "%3s=%1d", g.buf6, g.reg.mirror_lock);
      lcd.setCursor(9, 2);
      lcd.print(g.buffer);
      // Line 4:
      sprintf(g.buffer, "Save=%1d   Deb=%1d", g.reg.save_energy, g.disable_limiters);
      lcd.print(g.buffer);
      // Line 5:
      //    lcd.print("              ");
      lcd.setCursor(0, 5);
      // Line 6:
#ifdef SHOW_EEPROM
      //  Showing amount of EEPROM used:
      sprintf(g.buffer, "%4d s%s", ADDR_END, VERSION);
#else
#ifdef TEMPERATURE
      // Printing the temperature (Celcius), and version:
      sprintf(g.buffer, "%4sC  s%s", ftoa(g.buf6, g.Temp, 1), VERSION);
      //      sprintf(g.buffer, "%3d %4s", g.raw_T, ftoa(g.buf6, g.Temp, 1));
#else
      sprintf(g.buffer, "         s%s", VERSION);
#endif // TEMPERATURE
#endif  // EEPROM
      lcd.print(g.buffer);
    }

    else
      // Screen "D" (telescope mode only)
    {
      byte i = 0;
      for (row = 0; row < 4; row = row + 3)
        for (col = 0; col < 8; col = col + 7)
        {
          if (i == g.current_point)
            // Marking the current point section of the screen with a "*":
          {
            lcd.setCursor(col, row);
            lcd.print("*");
          }
          lcd.setCursor(col + 1, row);
#ifdef SHOW_RAW
          sprintf(g.buffer, "%1d)%4d", i + 1, g.delta_pos[i]);
#else
          short p_int = g.mm_per_microstep * 1000 * (float)(g.delta_pos[i]) + 0.5;
          // Showing delta_pos in microns:
          sprintf(g.buffer, "%1d)%4d", i + 1, p_int);
#endif
          lcd.print(g.buffer);
          lcd.setCursor(col + 1, row + 1);
#ifdef TEMPERATURE
#ifdef SHOW_RAW
          sprintf(g.buffer, "%5d", g.reg.raw_T[i]);
#else
          sprintf(g.buffer, "%5sC", ftoa(g.buf6, g.Temp0[i], 1));
#endif
#endif
          lcd.print(g.buffer);
          lcd.setCursor(col + 1, row + 2);
#ifdef SHOW_RAW
          sprintf(g.buffer, "%6d", g.reg.point[i]);
#else
          p = g.mm_per_microstep * (float)(g.reg.point[i]);
          sprintf(g.buffer, "%6s", ftoa(g.buf7, p, 3));
#endif
          lcd.print(g.buffer);
          i++;
        }
    }  // if alt_kind
  }

  else
  {
    if (g.error == 0)
    {
      display_u_per_f();  display_fps();
      display_one_point_params();
      display_two_point_params();
      display_two_points();
      display_current_position();
      display_status_line();
    }

    else
      // Error code displaying:
    {
      switch (g.error)
      {
        case 1:
#ifdef SHORT_ERRORS
          lcd.print("No cable");
#else
          lcd.print("Cable discon- nected, or    limiter is on!");
          lcd.print("Only if cable is connected, rewind");
          /*
                    lcd.print("Cable discon- ");
                    lcd.print("nected, or    ");
                    lcd.print("limiter is on!");
                    lcd.print("Only if cable ");
                    lcd.print("is connected, ");
                    lcd.print("rewind."); lcd.clearRestOfLine();
          */
#endif
          break;

        case 2: // Critically low battery level; not used when debugging
#ifdef SHORT_ERRORS
          lcd.print("Low battery");
#else
          //          lcd.print("Critically lowbattery level!\n");
          //          lcd.print("Replace the   batteries");

          lcd.print("Critically low");
          lcd.print("battery level!");
          //          lcd.print("              ");
          //            lcd.clearRestOfLine();
          //            lcd.print("Replace the   ");
          //            lcd.print("batteries."); lcd.clearRestOfLine();
          //            lcd.clearRestOfLine();
          //          lcd.print("              ");
#endif
          break;

        case 3:  // Factory reset initiated
          lcd.print("Factory reset?");
          break;

        case 4:  // Calibration initiated
#ifdef SHORT_ERRORS
          lcd.print("Calibration");
#else
          lcd.print("  Calibration\n  required!\n");
          lcd.print("\nPress any key\nto start\n");
          lcd.print("calibration");
          /*
            lcd.print("  Calibration ");
            lcd.print("  required!"); lcd.clearRestOfLine();
            //        lcd.print("              ");
            lcd.clearRestOfLine();
            lcd.print("Press any key ");
            lcd.print("to start"); lcd.clearRestOfLine();
            lcd.print("calibration.  ");
          */
#endif
          break;

      } // case
    }
  }  // if alt_flag
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void letter_status(char const * l)
/*
  Display a letter code "l" at the beginning of the status line
*/
{
  if (g.error || g.alt_flag)
    return;
  lcd.setCursor(0, 5);
  if (g.paused)
    lcd.print("P");
  else if (g.timelapse_mode)
    lcd.print("T");
  else
    lcd.print(*l);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void motion_status()
/*
  Motion status: 2-char status showing the direction and speed (rewind vs. focus stacking)
*/
{
  if (g.error || g.alt_flag)
    return;
#ifdef REWIND_BITMAPS
  uint8_t i;
#endif
  lcd.setCursor(2, 5);

  if (g.moving == 0 && g.started_moving == 0)
    lcd.print("   ");
  else
  {
    if (g.direction == -1)
    {
      if (g.stacker_mode < 2)
#ifdef REWIND_BITMAPS
        for (i = 0; i < 12; i++)
          lcd.data(rewind_char[i]);
#else
        lcd.print("<- ");
#endif
      else
        lcd.print("<  ");
    }
    else
    {
      if (g.stacker_mode < 2)
#ifdef REWIND_BITMAPS
        for (i = 0; i < 12; i++)
          lcd.data(forward_char[i]);
#else
        lcd.print("-> ");
#endif
      else
        lcd.print(" > ");
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
  if (g.error || g.alt_flag || g.telescope)
    return;
  // Printing frame counter:
  if (g.stacker_mode == 0 && g.paused == 0 || g.paused > 1)
    sprintf (g.buffer, "   0 ");
  else
    sprintf (g.buffer, "%4d ",  g.frame_counter + 1);
  lcd.setCursor(5, 5);
  lcd.print (g.buffer);

  return;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void points_status()
/* Displays F or B if the current coordinate is exactly the foreground (g.reg.point[0]) or background (g.reg.point[3]) point.
*/
{
  if (g.error || g.alt_flag)
    return;

  lcd.setCursor(10, 5);

  if (g.status_flag == 2 && g.t > g.t_status + FLASHING_DELAY)
  {
    g.status_flag = 0;
  }

  if (g.current_point < 0 || abs(g.delta_pos_curr - g.delta_pos[g.current_point]) > DELTA_POS_MAX && g.status_flag < 2)
  {
    if (g.current_point >= 0)
    {
      if (g.status_flag == 0)
      {
        g.status_flag = 1;
        g.t_status = g.t;
      }
      if (g.status_flag == 1 && g.t > g.t_status + FLASHING_DELAY)
      {
        g.status_flag = 2;
        g.t_status = g.t;
      }
    }
    lcd.print("  ");
    return;
  }
  else
    g.status_flag = 0;

  if (g.telescope)
  {
    sprintf (g.buffer, "%1d ", g.current_point + 1);
    lcd.print (g.buffer);
  }
  else
  {
    if (g.current_point == 0)
      lcd.print("F ");
    else
      lcd.print("B ");
  }

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void battery_status()
/*
  Measuring the battery voltage and displaying it.
*/
{
  if (g.moving == 1 || g.alt_flag)
    return;

  // Battery voltage (per AA battery; assuming 8 batteries) measured via a two-resistor voltage devider
  // (to reduce voltage from 12V -> 5V)
  // Slow operation (100 us), so should be done infrequently
  float V = (float)analogRead(PIN_BATTERY) * VOLTAGE_SCALER;

  if (g.error)
    return;

#ifdef BATTERY_DEBUG
  // Printing actual voltage per AA battery (times 100)
  lcd.setCursor(11, 5);
  int Vint = (int)(100.0 * V);
  sprintf(g.buffer, "%3d", Vint);
  lcd.print(g.buffer);
#else
  lcd.setCursor(12, 5);

#ifdef BATTERY_BITMAPS
  // A 4-level bitmap indication (between V_LOW and V_HIGH):
  byte level = (short)((V - V_LOW) / (V_HIGH - V_LOW) * 4.0);
  if (level < 0)
    level = 0;
  if (level > 3)
    level = 3;
  uint8_t i;
  for (i = 0; i < 12; i++)
    lcd.data(battery_char[level][i]);
#else
  if (V > 1.25)
    // >50% charge:
    lcd.print("##");
  else if (V > V_LOW)
    // Less than 50% charge:
    lcd.print("#.");
  else
    // Critically low charge:
    lcd.print("..");
#endif

#endif // BATTERY_DEBUG

  // Disabling the rail once V goes below the critical V_LOW voltage
#ifndef MOTOR_DEBUG
  if (V < V_LOW)
    g.error = 2;
#endif

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_status_line()
/*
  Display the whole status line
*/
{
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
  if (g.error || g.alt_flag)
    return;

  // +0.05 is for proper round-off:
  float um_per_frame = 1e3 * g.mm_per_microstep * MSTEP_PER_FRAME[g.reg.i_mm_per_frame] + 0.05;

  if (um_per_frame >= 10.0)
    sprintf(g.buffer, "%4duf ", (int)um_per_frame);
  else
    sprintf(g.buffer, "%4suf ", ftoa(g.buf7, um_per_frame + 0.05, 1));

  lcd.setCursor(0, 2);
  lcd.print(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_fps()
/*
  Display the input parameter fps (frames per second)
*/
{
  if (g.error || g.alt_flag || g.telescope)
    return;
  if (FPS[g.reg.i_fps] >= 1.0)
    sprintf(g.buffer, " %3sfps", ftoa(g.buf7, FPS[g.reg.i_fps], 1));
  else
    sprintf(g.buffer, "%4sfps", ftoa(g.buf7, FPS[g.reg.i_fps], 2));

  lcd.setCursor(7, 2);
  lcd.print(g.buffer);
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
  if (g.error || g.alt_flag)
    return;

  lcd.setCursor(0, 0);

  if (g.telescope)
  {
    if (g.locked[g.ireg - 1])
      sprintf(g.tmp_char, "L");
    else
      sprintf(g.tmp_char, " ");
    sprintf(g.buffer, " Register %2d%1s ", g.ireg, g.tmp_char);
  }
  else
  {
    // +0.05 for proper round off:
    float dx = (float)(N_SHOTS[g.reg.i_n_shots] - 1) * g.mm_per_microstep * MSTEP_PER_FRAME[g.reg.i_mm_per_frame] + 0.05;
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

    sprintf(g.buffer, "%4d %4s %4s", N_SHOTS[g.reg.i_n_shots], g.buf7 , g.buf6);
  }

  lcd.print(g.buffer);
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
  if (g.error || g.alt_flag)
    return;

  lcd.setCursor(0, 1);

  if (g.telescope)
  {
    sprintf(g.buffer, "%s", Name[g.ireg - 1]);
  }
  else
  {
    // +0.05 for proper round off:
    dx = g.mm_per_microstep * (float)(g.reg.point[3] - g.reg.point[0]) + 0.05;
    // +0.5 for proper round off:
    short dt = (short)((float)(g.Nframes - 1) / FPS[g.reg.i_fps] + 0.5);
    if (dt < 1000.0 && dt >= 0.0)
      sprintf(g.buf6, "%3ds", dt);
    else if (dt < 10000.0 && dt >= 0.0)
      sprintf(g.buf6, "%4d", dt);
    else
      sprintf(g.buf6, "****");

    if (g.reg.point[3] >= g.reg.point[0])
      sprintf(g.buffer, "%4d %4s %4s", g.Nframes, ftoa(g.buf7, dx, 1), g.buf6);
    else
      sprintf(g.buffer, "**** **** ****");
  }
  lcd.print(g.buffer);
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

  if (g.error || g.alt_flag || g.telescope)
    return;

#ifdef SHOW_RAW
  sprintf(g.buffer, "F%5d", g.reg.point[0]);
#else
  p = g.mm_per_microstep * (float)(g.reg.point[0]);
  if (p >= 0.0)
    sprintf(g.buffer, "F%s", ftoa(g.buf7, p, 2));
  else
    sprintf(g.buffer, "F*****");
#endif
  lcd.setCursor(0, 3);
  lcd.print(g.buffer);

#ifdef SHOW_RAW
  sprintf(g.buffer, "B%5d", g.reg.point[3]);
#else
  p = g.mm_per_microstep * (float)(g.reg.point[3]);
  if (p >= 0.0)
    sprintf(g.buffer, "B%s", ftoa(g.buf7, p, 2));
  else
    sprintf(g.buffer, "B*****");
#endif
  lcd.setCursor(8, 3);
  lcd.print(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_current_position()
/*
  Display the current position on the transient line
*/
{
  float p;
#ifdef CAMERA_DEBUG
  //  return;
#endif
#ifdef TEST_SWITCH
  lcd.setCursor(0, 4);
  sprintf(g.buffer, "%2d %5s %5s", g.test_N, ftoa(g.buf7, g.test_dev, 2), ftoa(g.buf6, g.test_std, 2));
  //  sprintf(g.buffer, "%2d %5d %5d", g.test_N, g.limit1, g.limit_tmp);
  lcd.print(g.buffer);
  return;
#endif

#ifdef TIMING
  // Average loop length for the last motion, in shortest miscrostep length units *100:
  short avr = (short)(100.0 * (float)(g.t - g.t0_timing) / (float)(g.i_timing - 1) * SPEED_LIMIT);
  // Maximum/minimum loop lenght in the above units:
  short max1 = (short)(100.0 * (float)(g.dt_max) * SPEED_LIMIT);
  short min1 = (short)(100.0 * (float)(g.dt_min) * SPEED_LIMIT);
  sprintf(g.buffer, "%4d %4d %4d", min1, avr, max1);
  lcd.setCursor(0, 4);
  lcd.print(g.buffer);
  // How many times arduino loop was longer than the shortest microstep time interval; total number of arduino loops:
  sprintf(g.buffer, "%4d %6ld   ", g.bad_timing_counter, g.i_timing);
  lcd.setCursor(0, 5);
  lcd.print(g.buffer);
#ifdef MOTOR_DEBUG
  //  sprintf(g.buffer, "%4d %4d %4d", cplus2, cmax, imax);
  //  lcd.setCursor(0, 3);
  //  lcd.print(g.buffer);
#endif
  return;
#endif

  if (g.error || g.moving == 0 && g.BL_counter > 0 || g.alt_flag)
    return;

  if (g.reg.straight)
    sprintf(g.tmp_char, " ");
  else
    sprintf(g.tmp_char, "R");

  if (g.timelapse_mode)
    sprintf(g.buf6, "%3d", g.timelapse_counter + 1);
  else
    sprintf(g.buf6, "   ");

#ifdef BL_DEBUG
  // When debugging backlash, displays the current backlash value in microsteps
  sprintf(g.buf6, "%3d", g.backlash);
#endif
#ifdef BL2_DEBUG
  // When debugging BACKLASH_2, displays the current BACKLAS_2 value in microsteps
  sprintf(g.buf6, "%3d", BACKLASH_2);
#endif
#ifdef DELAY_DEBUG
  // Delay used in mirror_lock=2 mode (electronic shutter), in 10ms units:
  sprintf(g.buf6, "%3d", SHUTTER_ON_DELAY2 / 10000);
#endif


#ifdef SHOW_RAW
  sprintf(g.buffer, "%1s %6d   %3s", g.tmp_char, g.pos_short_old, g.buf6);
#else
  p = g.mm_per_microstep * g.pos;
  sprintf(g.buffer, "%1s %6smm %3s", g.tmp_char, ftoa(g.buf7, p, 3), g.buf6);
#endif

  lcd.setCursor(0, 4);
  lcd.print(g.buffer);

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_comment_line(char const * l)
/*
  Display a comment line briefly (then it should be replaced with display_current_position() output).
*/
{
#ifdef TIMING
  return;
#endif
#ifdef CAMERA_DEBUG
  return;
#endif
  if (g.error)
    return;
  lcd.setCursor(0, 4);
  lcd.print(l);
  g.t_comment = g.t;
  g.comment_flag = 1;
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void delay_buffer()
// Fill g.buffer with non-continuous stacking parameters, to be displayed with display_comment_line:
{
  float y;
  if (g.telescope)
    return;
  y = (float)MSTEP_PER_FRAME[g.reg.i_mm_per_frame] / g.accel_limit;
  // Time to travel one frame (s), with fixed acceleration:
  float dt_goto = 2e-6 * sqrt(y);
  float delay1 = FIRST_DELAY[g.reg.i_first_delay];
  float delay2 = SECOND_DELAY[g.reg.i_second_delay];
  // +0.5 for roundoff:
  short dt = (short)((float)(g.Nframes) * (FIRST_DELAY[g.reg.i_first_delay] + SECOND_DELAY[g.reg.i_second_delay]) + (float)(g.Nframes - 1) * dt_goto + 0.5);
  sprintf(g.buffer, "%4s %4s %4d", ftoa(g.buf7, delay1, 1), ftoa(g.buf6, delay2, 1), dt);

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#ifdef CAMERA_DEBUG
void AF_status(short s)
{
  lcd.setCursor(12, 4);
  lcd.print(s);
  return;
}
void shutter_status(short s)
{
  lcd.setCursor(13, 4);
  lcd.print(s);
  return;
}
#endif

/*
 All LCD related functions
*/

void display_all()
/*
 Refreshing the whole screen
 */
{
#ifdef TIMING
  return;
#endif

  lcd.clear();
  lcd.setCursor(0, 0);

  if (g.alt_flag)
  {
    // Line 1:
    sprintf(g.buffer, "Rev=%1d    Acc=%1d", 1 - g.straight, ACCEL_FACTOR[g.i_accel_factor]);
    lcd.print(g.buffer);
    // Line 2:
    sprintf(g.buffer, "N=%-3d     BL=%1d", N_TIMELAPSE[g.i_n_timelapse], g.backlash_on);
    lcd.print(g.buffer);
    // Line 3:
    sprintf(g.buf6, "dt=%ds", DT_TIMELAPSE[g.i_dt_timelapse]);
    lcd.print(g.buf6);
    sprintf(g.buffer, "Mir=%1d", g.mirror_lock);
    lcd.setCursor(9, 2);
    lcd.print(g.buffer);
    // Line 4:
    sprintf(g.buffer, "         Deb=%1d", g.disable_limiters);
    lcd.print(g.buffer);
    // Line 5:
    //    lcd.print("              ");
    lcd.setCursor(0, 5);
    // Line 6:
    sprintf(g.buffer, "         s%s", VERSION);
    lcd.print(g.buffer);
  }
  else
  {

    if (g.error == 0)
    {
      if (g.calibrate_warning == 0)
      {
        display_u_per_f();  display_fps();
        display_one_point_params();
        display_two_point_params();
        display_two_points();
        display_current_position();
        display_status_line();
      }
      else
      {
        lcd.print("  Calibration ");
        lcd.print("  required!   ");
        lcd.print("              ");
        lcd.print("Press any key ");
        lcd.print("to start      ");
        lcd.print("calibration.  ");
      }
    }

    else
      // Error code displaying:
    {
      switch (g.error)
      {
        case 1:
          lcd.print("Cable discon- ");
          lcd.print("nected, or    ");
          lcd.print("limiter is on!");
          lcd.print("Only if cable ");
          lcd.print("is connected, ");
          lcd.print("rewind.       ");
          break;

        case 2: // Critically low battery level; not used when debugging
          lcd.print("Critically low");
          lcd.print("battery level!");
          lcd.print("              ");
          lcd.print("Replace the   ");
          lcd.print("batteries.    ");
          lcd.print("              ");
          break;

      } // case
    }
  }  // if alt_flag
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void letter_status(char* l)
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
  uint8_t i;
  lcd.setCursor(2, 5);

  if (g.moving == 0 && g.started_moving == 0)
    lcd.print("   ");
  else
  {
    if (g.direction == -1)
    {
      if (g.stacker_mode < 2)
        for (i = 0; i < 12; i++)
          lcd.data(rewind_char[i]);
      else
        lcd.print("<  ");
    }
    else
    {
      if (g.stacker_mode < 2)
        for (i = 0; i < 12; i++)
          lcd.data(forward_char[i]);
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
  if (g.error || g.alt_flag)
    return;
  // Printing frame counter:
  if (g.stacker_mode == 0 && g.paused == 0)
    sprintf (g.buffer, "   0 ");
  else
    sprintf (g.buffer, "%4d ",  g.frame_counter + 1);
  lcd.setCursor(5, 5);
  lcd.print (g.buffer);

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void points_status()
/* Displays F or B if the current coordinate is exactly the foreground (g.point1) or background (g.point2) point.
 */
{
  if (g.error || g.alt_flag)
    return;
  lcd.setCursor(10, 5);

  if (g.pos_short_old == g.point1)
    lcd.print("F ");
  else if (g.pos_short_old == g.point2)
    lcd.print("B ");
  else
    lcd.print("  ");

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

  // This is done only once, when the decice is powerd up:
  if (g.setup_flag == 1)
  {
    // Deciding which seep limit to use: using the larger value if powered from AC, smaller value if powered from batteries:
    if (V > SPEED_VOLTAGE)
      g.speed_limit = SPEED_LIMIT;
    else
      g.speed_limit = SPEED_LIMIT2;
  }

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
  // A 4-level bitmap indication (between V_LOW and V_HIGH):
  byte level = (short)((V - V_LOW) / (V_HIGH - V_LOW) * 4.0);
  if (level < 0)
    level = 0;
  if (level > 3)
    level = 3;
  uint8_t i;
  for (i = 0; i < 12; i++)
    lcd.data(battery_char[level][i]);

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
    
  if (MM_PER_FRAME[g.i_mm_per_frame] >= 0.00995)
    sprintf(g.buffer, "%4duf ", nintMy(1000.0 * MM_PER_FRAME[g.i_mm_per_frame]));
  else
    // +0.05 is for proper round-off:
    sprintf(g.buffer, "%4suf ", ftoa(g.buf7, 1000.0 * MM_PER_FRAME[g.i_mm_per_frame] + 0.05, 1));

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
  if (g.error || g.alt_flag)
    return;
  if (FPS[g.i_fps] >= 1.0)
    sprintf(g.buffer, " %3sfps", ftoa(g.buf7, FPS[g.i_fps], 1));
  else
    sprintf(g.buffer, "%4sfps", ftoa(g.buf7, FPS[g.i_fps], 2));

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
    
  // +0.05 for proper round off:
  float dx = (float)(N_SHOTS[g.i_n_shots] - 1) * MM_PER_FRAME[g.i_mm_per_frame] + 0.05;
  short dt = roundMy((float)(N_SHOTS[g.i_n_shots] - 1) / FPS[g.i_fps]);
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

  sprintf(g.buffer, "%4d %4s %4s", N_SHOTS[g.i_n_shots], g.buf7 , g.buf6);
  lcd.setCursor(0, 0);
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
  if (g.error || g.alt_flag)
    return;
    
  // +0.05 for proper round off:
  float dx = MM_PER_MICROSTEP * (float)(g.point2 - g.point1) + 0.05;
  short dt = nintMy((float)(g.Nframes - 1) / FPS[g.i_fps]);
  if (dt < 1000.0)
    sprintf(g.buf6, "%3ds", dt);
  else if (dt < 10000.0)
    sprintf(g.buf6, "%4d", dt);
  else
    sprintf(g.buf6, "****");

  if (g.point2 >= g.point1)
    sprintf(g.buffer, "%4d %4s %4s", g.Nframes, ftoa(g.buf7, dx, 1), g.buf6);
  else
    sprintf(g.buffer, "**** **** ****");
  lcd.setCursor(0, 1);
  lcd.print(g.buffer);
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_two_points()
/*
 Display the positions (in mm) of two points: foreground, F, and background, B.
 */
{
  if (g.error || g.alt_flag)
    return;
    
  float p = MM_PER_MICROSTEP * (float)g.point1;
  if (p >= 0.0)
    sprintf(g.buffer, "F%s", ftoa(g.buf7, p, 2));
  else
    sprintf(g.buffer, "F*****");
  lcd.setCursor(0, 3);
  lcd.print(g.buffer);
  p = MM_PER_MICROSTEP * (float)g.point2;
  if (p >= 0.0)
    sprintf(g.buffer, "B%s", ftoa(g.buf7, p, 2));
  else
    sprintf(g.buffer, "B*****");
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
#ifdef CAMERA_DEBUG
  //  return;
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
  sprintf(g.buffer, "%4d %4d %4d", cplus2, cmax, imax);
  lcd.setCursor(0, 3);
  lcd.print(g.buffer);
#endif
  return;
#endif

  if (g.error || g.calibrate_warning || g.moving == 0 && g.BL_counter > 0 || g.alt_flag)
    return;

  if (g.straight)
    g.rev_char = " ";
  else
    g.rev_char = "R";

  if (g.timelapse_mode)
    sprintf(g.buf6, "%4d", g.timelapse_counter + 1);
  else
    sprintf(g.buf6, "    ");

  float p = MM_PER_MICROSTEP * (float)g.pos;
  sprintf(g.buffer, "%1s %6smm %4s", g.rev_char, ftoa(g.buf7, p, 3), g.buf6);

#ifdef BL_DEBUG
  sprintf(g.buffer, "%1s %6smm %4d", ftoa(g.buf7, p, 3), roundMy(10000.0 * MM_PER_MICROSTEP * BACKLASH_2));
#endif

  lcd.setCursor(0, 4);
  lcd.print(g.buffer);

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_comment_line(char *l)
/*
 Display a comment line briefly (then it should be replaced with display_current_positio() output)
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
  float y = MM_PER_FRAME[g.i_mm_per_frame] / MM_PER_MICROSTEP / ACCEL_LIMIT;
  // Time to travel one frame (s), with fixed acceleration:
  float dt_goto = 2e-6 * sqrt(y);
  float delay1 = FIRST_DELAY[g.i_first_delay];
  float delay2 = SECOND_DELAY[g.i_second_delay];
  short dt = nintMy((float)(g.Nframes) * (FIRST_DELAY[g.i_first_delay] + SECOND_DELAY[g.i_second_delay]) + (float)(g.Nframes - 1) * dt_goto);
  sprintf(g.buffer, "%4s %4s %4d", ftoa(g.buf7, delay1, 1), (int)delay2, ftoa(g.buf7, delay2, 1), dt);

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

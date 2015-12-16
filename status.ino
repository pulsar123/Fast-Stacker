/*
 All LCD related functions
*/

void display_all(char* l)
/*
 Refreshing the whole screen
 */
{
#ifdef TIMING
  return;
#endif
  if (g.error == 0)
  {
    if (g.calibrate_warning == 0)
    {
      display_u_per_f();  display_fps();
      display_one_point_params();
      display_two_point_params();
      display_two_points();
      display_current_position();
      display_status_line(l);
    }
    else
    {
#ifdef DEBUG
      Serial.println("Press any key to start calibration");
#endif
#ifdef LCD
      lcd.setCursor(0, 0);  lcd.print("  Calibration ");
      lcd.setCursor(0, 1);  lcd.print("  required!   ");
      lcd.setCursor(0, 2);  lcd.print("              ");
      lcd.setCursor(0, 3);  lcd.print("Press any key ");
      lcd.setCursor(0, 4);  lcd.print("to start      ");
      lcd.setCursor(0, 5);  lcd.print("calibration.   ");
#endif
    }
  }

  else
    // Error code displaying:
  {
    switch (g.error)
    {
      case 1:
#ifdef DEBUG
        Serial.println("Cable disconnected or limiter is on!");
        Serial.println("Connect the cable if it is disconnected.");
        Serial.println("If cable is connected, rewind to safe area");
#endif
#ifdef LCD
        lcd.setCursor(0, 0);  lcd.print("Cable discon- ");
        lcd.setCursor(0, 1);  lcd.print("nected, or    ");
        lcd.setCursor(0, 2);  lcd.print("limiter is on!");
        lcd.setCursor(0, 3);  lcd.print("Only if cable ");
        lcd.setCursor(0, 4);  lcd.print("is connected, ");
        lcd.setCursor(0, 5);  lcd.print("rewind.       ");
#endif
        break;

      case 2: // Critically low battery level; not used when debugging
#ifdef LCD
        lcd.setCursor(0, 0);  lcd.print("Critically low");
        lcd.setCursor(0, 1);  lcd.print("battery level!");
        lcd.setCursor(0, 2);  lcd.print("              ");
        lcd.setCursor(0, 3);  lcd.print("Replace the   ");
        lcd.setCursor(0, 4);  lcd.print("batteries.    ");
        lcd.setCursor(0, 5);  lcd.print("              ");
#endif
        break;

    } // case
  }

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void letter_status(char* l)
/*
 Display a letter code "l" at the beginning of the status line
 */
{
  if (g.error || g.paused)
    return;
#ifdef LCD
  lcd.setCursor(0, 5);
  lcd.print(l);
#endif
#ifdef DEBUG
  Serial.println(l);
#endif
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void motion_status()
/*
 Motion status: 2-char status showing the direction and speed (rewind vs. focus stacking)
 */
{
  if (g.error)
    return;
#ifdef LCD
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
#endif

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_frame_counter()
/*
 Printing the current stacking frame number in the status line. Allowing for negative (>-100) and
 positive <1000 numbers.
  */
{
  if (g.error)
    return;
  // Printing frame counter:
  if (g.stacker_mode == 0 && g.paused == 0)
    sprintf (g.buffer, "  0 ");
  else if (g.frame_counter > -100 && g.frame_counter + 1 < 1000)
    sprintf (g.buffer, "%3d ",  g.frame_counter + 1);
  else
    sprintf (g.buffer, "*** ");
#ifdef LCD
  lcd.setCursor(5, 5);
  lcd.print (g.buffer); // display line on buffer
#endif
#ifdef DEBUG
  Serial.println(g.buffer);
#endif

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void points_status()
/* Display status of two points in two-point stacking mode. Empty space
  means the point wasn't set yet since bootup. Capital letter means
  we are exactly at that position now. f/b stans for foreground/background
  points (point1/point2).
 */
{
  if (g.error)
    return;
#ifdef LCD
  lcd.setCursor(9, 5);

  switch (g.points_byte)
  {
    case 0:
      lcd.print("   ");
      break;

    case 1:
      if (g.pos_short_old == g.point1)
        lcd.print("F  ");
      else
        lcd.print("f  ");
      break;

    case 2:
      if (g.pos_short_old == g.point2)
        lcd.print(" B ");
      else
        lcd.print("b  ");
      break;

    case 3:
      if (g.pos_short_old == g.point1)
        lcd.print("Fb ");
      else if (g.pos_short_old == g.point2)
        lcd.print("fB ");
      else
        lcd.print("fb ");
      break;
  }
#endif
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void battery_status()
/*
 Measuring the battery voltage and displaying it.
 */
{
  if (g.error || g.moving == 1)
    return;
  // Battery voltage (per AA battery; assuming 8 batteries) measured via a two-resistor voltage devider
  // (to reduce voltage from 12V -> 5V)
  // Slow operation (100 us), so should be done infrequently
  float V = (float)analogRead(PIN_BATTERY) * VOLTAGE_SCALER;
#ifdef DEBUG
  Serial.print("Voltage=");
  sprintf(g.buffer, "%5d.%03d V", (int)V, (int)(1000.0 * (V - (int)V)));
  Serial.println(g.buffer);
#endif

#ifdef LCD
#ifdef BATTERY_DEBUG
  // Printing actual voltage per AA battery (times 100)
  lcd.setCursor(11, 5);
  int Vint = (int)(100.0 * V);
  sprintf(g.buffer, "%3d", Vint);
  lcd.print(g.buffer);
#else
  lcd.setCursor(12, 5);
  // A 4-level bitmao indication (between V_LOW and V_HIGH):
  short level = (short)((V - V_LOW) / (V_HIGH - V_LOW) * 4.0);
  if (level < 0)
    level = 0;
  if (level > 3)
    level = 3;
  uint8_t i;
  for (i = 0; i < 12; i++)
    lcd.data(battery_char[level][i]);

#endif // BATTERY_DEBUG
#endif  // LCD

  // Disabling the rail once V goes below the critical V_LOW voltage
#ifndef DEBUG
#ifndef MOTOR_DEBUG
  if (V < V_LOW)
    g.error = 2;
#endif
#endif

  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_status_line(char* l)
/*
 Display the whole status line
 */
{
  if (g.error)
    return;
  letter_status(l);
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
  if (g.error)
    return;
  sprintf(g.buffer, "%4duf ", (short)(1000.0 * MM_PER_FRAME[g.i_mm_per_frame]));
#ifdef LCD
  lcd.setCursor(0, 2);
  lcd.print(g.buffer);
#endif
#ifdef DEBUG
  Serial.print(g.buffer);
#endif
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_fps()
/*
 Display the input parameter fps (frames per second)
 */
{
  if (g.error)
    return;
  //  sprintf(g.buffer, "%5.3ffs", FPS[g.i_fps]);
  sprintf(g.buffer, "%1d.%03dfs", (int)FPS[g.i_fps], (int)(1000.0 * (FPS[g.i_fps] - (int)FPS[g.i_fps])));
#ifdef LCD
  lcd.setCursor(7, 2);
  lcd.print(g.buffer);
#endif
#ifdef DEBUG
  Serial.println(g.buffer);
#endif
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
  if (g.error)
    return;
  float dx = (float)(N_SHOTS[g.i_n_shots] - 1) * MM_PER_FRAME[g.i_mm_per_frame];
  short dt = nintMy((float)(N_SHOTS[g.i_n_shots] - 1) / FPS[g.i_fps]);
  //  sprintf(g.buffer, "%3d %4du %3ds", N_SHOTS[g.i_n_shots], (int)dx, dt);
  sprintf(g.buffer, "%3d %2d.%01dm %3ds", N_SHOTS[g.i_n_shots], (int)dx, (int)((dx - (int)dx) * 10.0), dt);
#ifdef LCD
  lcd.setCursor(0, 0);
  lcd.print(g.buffer);
#endif
#ifdef DEBUG
  Serial.println(g.buffer);
#endif
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
  if (g.error)
    return;
  float dx = MM_PER_MICROSTEP * (float)(g.point2 - g.point1);
  short dt = nintMy((float)(g.Nframes - 1) / FPS[g.i_fps]);
  if (g.Nframes < 1000)
    sprintf(g.buffer, "%3d %2d.%01dm %3ds", g.Nframes, (int)dx, (int)((dx - (int)dx) * 10.0), dt);
  else
    sprintf(g.buffer, "*** %2d.%01dm %3ds", (int)dx, (int)((dx - (int)dx) * 10.0), dt);
#ifdef LCD
  lcd.setCursor(0, 1);
  lcd.print(g.buffer);
#endif
#ifdef DEBUG
  Serial.println(g.buffer);
#endif
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_two_points()
/*
 Display the positions (in mm) of two points: foreground, F, and background, B.
 */
{
  if (g.error)
    return;
  float p1 = MM_PER_MICROSTEP * (float)g.point1;
  float p2 = MM_PER_MICROSTEP * (float)g.point2;
  sprintf(g.buffer, "F%2d.%02d  B%2d.%02d", (int)p1, (int)(100.0 * (p1 - (int)p1)), (int)p2, (int)(100.0 * (p2 - (int)p2)));
#ifdef LCD
  lcd.setCursor(0, 3);
  lcd.print(g.buffer);
#endif
#ifdef DEBUG
  Serial.println(g.buffer);
#endif
  return;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void display_current_position()
/*
 Display the current position on the transient line
 */
{
#ifdef TIMING
  // Average loop length for the last motion, in shortest miscrostep length units *100:
  short avr = (short)(100.0 * (float)(g.t - g.t0_timing) / (float)(g.i_timing - 1) * SPEED_LIMIT);
  // Maximum/minimum loop lenght in the above units:
  short max1 = (short)(100.0 * (float)(g.dt_max) * SPEED_LIMIT);
  short min1 = (short)(100.0 * (float)(g.dt_min) * SPEED_LIMIT);
  sprintf(g.buffer, "%4d %4d %4d", min1, avr, max1);
#ifdef LCD
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
#endif
  return;
#endif

  if (g.error || g.calibrate_warning || g.moving == 0 && g.BL_counter > 0)
    return;

  float p = MM_PER_MICROSTEP * (float)g.pos;
#ifdef MOTOR_DEBUG
#ifdef PRECISE_STEPPING
  short backlash1 = (short)(100.0 * (float)(dt_backlash) * SPEED_LIMIT);
  //  sprintf(g.buffer, "%2d.%03d %3d %3d", (int)p, (int)(1000.0 * (p - (int)p)), backlash1, skipped_total);
  //  sprintf(g.buffer, "%2d.%03d %3d %3d", (int)p, (int)(1000.0 * (p - (int)p)), n_fixed, n_failed);
  //!!!
  sprintf(g.buffer, "%5d %5d   ", g.pos_short_old, g.pos_to_shoot);
#else
  sprintf(g.buffer, "%2d.%03d %3d %3d", (int)p, (int)(1000.0 * (p - (int)p)), skipped_current, skipped_total);
#endif
#else
  sprintf(g.buffer, "   P=%2d.%02dmm  ", (int)p, (int)(100.0 * (p - (int)p)));
#endif
#ifdef LCD
  lcd.setCursor(0, 4);
  lcd.print(g.buffer);
#endif

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
  if (g.error)
    return;
#ifdef LCD
  lcd.setCursor(0, 4);
  lcd.print(l);
#endif
#ifdef DEBUG
  Serial.println(l);
#endif
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
  sprintf(g.buffer, "%2d.%01d %2d.%01d %4d", (int)delay1, (int)(10.0 * (delay1 - (int)delay1)), (int)delay2, (int)(10.0 * (delay2 - (int)delay2)), dt);

  return;
}


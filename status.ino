/*
 Functions used to print the bottom (status) line on LCD
*/

void display_all(char* l)
/*
 Refreshing the whole screen
 */
{
  display_u_per_f();  display_fps();
  display_one_point_params();
  display_two_point_params();
  display_two_points();
  display_current_position();
  display_status_line(l);
  return;
}

void letter_status(char* l)
/*
 Display a letter code "l" at the beginning of the status line
 */
{
  lcd.setCursor(0, 5);
  lcd.print(*l);
  return;
}

void motion_status()
/*
 Motion status: 2-char status showing the direction and speed (rewind vs. focus stacking)
 */
{
  lcd.setCursor(2, 5);

  if (g.moving == 0)
    lcd.print("  ");
  else
  {
    if (g.direction == -1)
    {
      if (g.stacker_mode < 2)
        lcd.print("<-");
      else
        lcd.print("< ");
    }
    else
    {
      if (g.stacker_mode < 2)
        lcd.print("->");
      else
        lcd.print(" >");
    }
  }

  return;
}


void display_frame_counter()
{
  // Printing frame counter:
  lcd.setCursor(5, 5);
  sprintf (g.buffer, "%4u",  g.frame_counter);
  lcd.print (g.buffer); // display line on buffer

  return;
}


void points_status()
/* Display status of two points in two-point stacking mode. Empty space
  means the point wasn't set yet since bootup. Capital letter means
  we are exactly at that position now. f/b stans for foreground/background
  points (point1/point2).
 */
{
  lcd.setCursor(9, 5);

  switch (g.points_byte)
  {
    case 0:
      lcd.print("  ");
      break;

    case 1:
      if (g.pos_short_old == g.point1)
        lcd.print("F ");
      else
        lcd.print("f ");
      break;

    case 2:
      if (g.pos_short_old == g.point2)
        lcd.print(" B");
      else
        lcd.print("b ");
      break;

    case 3:
      if (g.pos_short_old == g.point1)
        lcd.print("Fb");
      else if (g.pos_short_old == g.point2)
        lcd.print("fB");
      else
        lcd.print("fb");
      break;
  }
  return;
}


void battery_status()
/*
 Just a placeholder for now.
 */
{
  lcd.setCursor(12, 5);
  // For now:
  lcd.print("##");

  return;
}


void display_status_line(char* l)
/*
 Display the whole status line
 */
{
  letter_status(l);
  motion_status();
  display_frame_counter();
  points_status();
  battery_status();
  return;
}


void display_u_per_f()
/*
 Display the input parameter u per frame (1000*MM_PER_FRAME)
 */
{
  lcd.setCursor(0, 0);
  sprintf(g.buffer, "%4dus ", (short)(1000.0 * MM_PER_FRAME[g.i_mm_per_frame]));
  lcd.print(g.buffer);
  return;
}


void display_fps()
/*
 Display the input parameter fps (frames per second)
 */
{
  lcd.setCursor(7, 0);
  sprintf(g.buffer, "%5.3ffs", FPS[g.i_fps]);
  lcd.print(g.buffer);
  return;
}


void display_one_point_params()
/*
 Display the three parameters for one-point shooting:
  - input parameter N_SHOTS,
  - travel distance, mm,
  - travel time, s.
 */
{
  lcd.setCursor(0, 1);
  float dx = (N_SHOTS[g.i_n_shots] - 1) * MM_PER_FRAME[g.i_mm_per_frame];
  short dt = nintMy((float)(N_SHOTS[g.i_n_shots] - 1) / FPS[g.i_fps]);
  sprintf(g.buffer, "%3d %4.1fm %3ds", N_SHOTS[g.i_n_shots], dx, dt);
  lcd.print(g.buffer);
  return;
}


void display_two_point_params()
/*
 Display the three parameters for two-point shooting:
  - number of shots,
  - travel distance, mm,
  - travel time, s.
 */
{
  lcd.setCursor(0, 2);
  float dx = MM_PER_MICROSTEP * (float)(g.point2 - g.point1);
  short dt = nintMy((float)(g.Nframes - 1) / FPS[g.i_fps]);
  sprintf(g.buffer, "%3d %4.1fm %3ds", g.Nframes, dx, dt);
  lcd.print(g.buffer);
  return;
}


void display_two_points()
/*
 Display the positions (in mm) of two points: foreground, F, and background, B.
 */
{
  lcd.setCursor(0, 3);
  sprintf(g.buffer, "F%5.2f  B%5.2f", MM_PER_MICROSTEP * (float)g.point1, MM_PER_MICROSTEP * (float)g.point2);
  lcd.print(g.buffer);
  return;
}


void display_current_position()
/*
 Display the current position on the transient line
 */
{
  lcd.setCursor(0, 4);
  sprintf(g.buffer, "   P=%5.2fmm", MM_PER_MICROSTEP * g.pos);
  // Do the slow display operation only if the number changed:
  if (strcmp(g.buffer, g.p_buffer) != 0)
    lcd.print(g.buffer);
  return;

}


void display_comment_line(char *l)
/*
 Display a comment line briefly (then it should be replaced with display_current_positio() output)
 */
 {
  lcd.setCursor(0, 4);
  lcd.print(*l);  
  g.t_comment = g.t;
  g.comment_flag = 1;
  return;
 }


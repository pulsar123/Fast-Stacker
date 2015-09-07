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
#ifdef LCD
  lcd.setCursor(0, 5);
  lcd.print(l);
#endif
#ifdef DEBUG
  Serial.println(l);
#endif  
  return;
}

void motion_status()
/*
 Motion status: 2-char status showing the direction and speed (rewind vs. focus stacking)
 */
{
#ifdef LCD
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
#endif

  return;
}


void display_frame_counter()
{
  // Printing frame counter:
  sprintf (g.buffer, "%4u",  g.frame_counter);
#ifdef LCD  
  lcd.setCursor(5, 5);
  lcd.print (g.buffer); // display line on buffer
#endif  
#ifdef DEBUG  
  Serial.println(g.buffer);
#endif

  return;
}


void points_status()
/* Display status of two points in two-point stacking mode. Empty space
  means the point wasn't set yet since bootup. Capital letter means
  we are exactly at that position now. f/b stans for foreground/background
  points (point1/point2).
 */
{
#ifdef LCD  
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
#endif  
  return;
}


void battery_status()
/*
 Just a placeholder for now.
 */
{
#ifdef LCD
 lcd.setCursor(12, 5);
  // For now:
  lcd.print("##");
#endif
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
  sprintf(g.buffer, "%4dus ", (short)(1000.0 * MM_PER_FRAME[g.i_mm_per_frame]));
#ifdef LCD  
  lcd.setCursor(0, 0);
  lcd.print(g.buffer);
#endif  
#ifdef DEBUG  
  Serial.print(g.buffer);
#endif
  return;
}


void display_fps()
/*
 Display the input parameter fps (frames per second)
 */
{
//  sprintf(g.buffer, "%5.3ffs", FPS[g.i_fps]);
  sprintf(g.buffer, "%5d.%3dfs", (int)FPS[g.i_fps], (int)(1000.0*(FPS[g.i_fps]-(int)FPS[g.i_fps])));
#ifdef LCD
  lcd.setCursor(7, 0);
  lcd.print(g.buffer);
#endif  
#ifdef DEBUG  
  Serial.println(g.buffer);
#endif
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
  float dx = (float)(N_SHOTS[g.i_n_shots] - 1) * MM_PER_FRAME[g.i_mm_per_frame];
  short dt = nintMy((float)(N_SHOTS[g.i_n_shots] - 1) / FPS[g.i_fps]);
//  sprintf(g.buffer, "%3d %4du %3ds", N_SHOTS[g.i_n_shots], (int)dx, dt);
  sprintf(g.buffer, "%3d %2d.%1dm %3ds", N_SHOTS[g.i_n_shots], (int)dx, (int)((dx-(int)dx)*10.0), dt);
#ifdef LCD
  lcd.setCursor(0, 1);
  lcd.print(g.buffer);
#endif
#ifdef DEBUG  
  Serial.println(g.buffer);
#endif
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
  float dx = MM_PER_MICROSTEP * (float)(g.point2 - g.point1);
  short dt = nintMy((float)(g.Nframes - 1) / FPS[g.i_fps]);
//  sprintf(g.buffer, "%3d %4.1fm %3ds", g.Nframes, dx, dt);
  sprintf(g.buffer, "%3d %2d.%1dm %3ds", g.Nframes, (int)dx, (int)((dx-(int)dx)*10.0), dt);
#ifdef LCD  
  lcd.setCursor(0, 2);
  lcd.print(g.buffer);
#endif  
#ifdef DEBUG  
  Serial.println(g.buffer);
#endif
  return;
}


void display_two_points()
/*
 Display the positions (in mm) of two points: foreground, F, and background, B.
 */
{
//  sprintf(g.buffer, "F%5.2f  B%5.2f", MM_PER_MICROSTEP * (float)g.point1, MM_PER_MICROSTEP * (float)g.point2);
float p1 = MM_PER_MICROSTEP * (float)g.point1;
float p2 = MM_PER_MICROSTEP * (float)g.point2;
  sprintf(g.buffer, "F%2d.%2df  B%2d.%2ds", (int)p1, (int)(100.0*(p1-(int)p1)), (int)p2, (int)(100.0*(p2-(int)p2)));
#ifdef LCD  
  lcd.setCursor(0, 3);
  lcd.print(g.buffer);
#endif  
#ifdef DEBUG  
  Serial.println(g.buffer);
#endif
  return;
}


void display_current_position()
/*
 Display the current position on the transient line
 */
{
//  sprintf(g.buffer, "   P=%5.2fmm", MM_PER_MICROSTEP * g.pos);
  float p = MM_PER_MICROSTEP * g.pos;
  sprintf(g.buffer, "   P=%2d.%2dmm", (int)p, (int)(100.0*(p-(int)p)));
  // Do the slow display operation only if the number changed:
  if (strcmp(g.buffer, g.p_buffer) != 0)
  {
#ifdef LCD    
    lcd.setCursor(0, 4);
    lcd.print(g.buffer);
#endif    
#ifdef DEBUG  
//  Serial.println(g.buffer);
Serial.print("pos=");
Serial.print(g.pos_short_old);
Serial.print("; g.point1=");
Serial.print(g.point1);
Serial.print("; g.point2=");
Serial.println(g.point2);
#endif
  }
  return;

}


void display_comment_line(char *l)
/*
 Display a comment line briefly (then it should be replaced with display_current_positio() output)
 */
 {
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


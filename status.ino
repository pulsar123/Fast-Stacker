/*
 Functions used to print the bottom (status) line on LCD
*/

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


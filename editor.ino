void editor(char key)
/*
   Handles all the editing stuff. Parameter to edit is provided via g.edited_param .
   Input: a char, with either a pressed key value, or a command:

  I: initialize editor

*/
{

#define WIDTH 16
#define Y_VALUE 51
#define TFT_VERYDARK 0x0020

  signed char digit = -1;
  if (key == '0' || key == '1' || key == '2' || key == '3' || key == '4' || key == '5' || key == '6' || key == '7' || key == '8' || key == '9')
  {
    digit = (key - '0');
  }

  if (key == '#')
  {
    if (g.dot_pos >= 0)
      // No more than one dot in the value
      return;
    key = '.';
    g.dot_pos = g.cursor_pos;  // Memorizing the dot position
  }

  if (key == 'I')
    // Initializing the editor
  {
    // Initial cursor position:
    g.cursor_pos = 0;
    g.dot_pos = -1;
    sprintf(g.value, "          ");  // 10 spaces = MAX_POS

    tft.fillScreen(TFT_BLACK);

    tft.setCursor(0, 0, 1);
    tft.setTextColor(TFT_SILVER, TFT_BLACK);

    switch (g.edited_param)
      /// Descriptions for each parameter (top of the screen)
      // 26 chars per line, up to 5 lines ok
    {
      case PARAM_MSTEP:
        tft.println("Distance between adjacent");
        tft.println("frames in micrometers.");
        tft.println("Will be rounded up to the");
        tft.println("nearest whole microstep.");
        break;

      case PARAM_FPS:
        tft.println("Frames per second (only");
        tft.println("in continuous shooting");
        tft.println("mode)");
        break;

      case PARAM_N_SHOTS:
        tft.println("Number of shots (only for");
        tft.println("one-point continuous");
        tft.println("shooting)");
        break;

      case PARAM_FIRST_DELAY:
        tft.println("The first delay in non-");
        tft.println("continuous mode (seconds)");
        break;

      case PARAM_SECOND_DELAY:
        tft.println("The second delay in non-");
        tft.println("continuous mode (seconds)");
        break;

      case PARAM_GOTO:
        if (g.paused)
        {
          tft.println("Destination frame for");
          tft.println("for the GoTo command.");
          tft.println("Range: 1...N");
        }
        else
        {
          tft.println("Destination coordinate");
          tft.println("(in mm) for the GoTo");
          tft.println("command.");
        }
        break;
    }

    // Descriptions for the keys (bottom of the screen)
    int x0 = 4;
    int y0 = 84;
    int delta = 26;
    int shift = 14;
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);

    tft.setCursor(x0 + shift, y0, 2);
    tft.print("Accept");

    tft.setCursor(80 + shift, y0, 2);
    tft.print("Backspace");

    tft.setCursor(x0 + shift, y0 + delta, 2);
    tft.print("Cancel");

    tft.setCursor(80 + shift, y0 + delta, 2);
    tft.print("Dot");

    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.setTextPadding(12);

    tft.setCursor(x0, y0, 2);
    tft.print('A');

    tft.setCursor(80, y0, 2);
    tft.print('B');

    tft.setCursor(x0, y0 + delta, 2);
    tft.print('C');

    tft.setCursor(80, y0 + delta, 2);
    tft.print('#');

    tft.setTextFont(2);
  }

  else if (key == 'C')
  {
    g.editing = 0;
    tft.setTextFont(2);
    display_all();
  }

  else if (key == 'B')
  {
    if (g.cursor_pos > 0)
    {
      g.cursor_pos--;
      g.value[g.cursor_pos] = ' ';
      tft.setTextColor(TFT_VERYDARK, TFT_BLACK);
      int x = g.cursor_pos * WIDTH;
      tft.setCursor(x, Y_VALUE, 4);
      tft.print("   ");
      if (g.cursor_pos == g.dot_pos)
        g.dot_pos = -1;
    }
  }

  else if (digit >= 0 || key == '.')
  {
    if (g.cursor_pos < MAX_POS)
    {
      g.value[g.cursor_pos] = key;
      tft.setTextColor(TFT_YELLOW, TFT_NAVY);
      int x = g.cursor_pos * WIDTH;
      tft.setCursor(x, Y_VALUE, 4);
      tft.print(key);
      if (key == '.')
        tft.print(" ");
      g.cursor_pos++;

    }
  }

  else if (key == 'A')
    // Accepting the value, and doing all associated actions
  {
    if (g.cursor_pos > 0)
    {
      // First - converting to float
      float fvalue = atof(g.value);
      if (g.edited_param == PARAM_MSTEP)
      {
        float n_steps = fvalue / 1000.0 / MM_PER_MICROSTEP;
        COORD_TYPE in_steps = (COORD_TYPE)(n_steps + 0.5); // Rounding up to the nearest integer value
        // Enforcing limits:
        if (in_steps < MSTEP_MIN)
          in_steps = MSTEP_MIN;
        if (in_steps > MSTEP_MAX)
          in_steps = MSTEP_MAX;
        g.reg.mstep = in_steps; // Updating the value
        EEPROM.put( g.addr_reg[0], g.reg);
      }

      else if (g.edited_param == PARAM_FPS)
      {
        // Enforcing limits:
        if (fvalue < FPS_MIN)
          fvalue = FPS_MIN;
        if (fvalue > FPS_MAX)
          fvalue = FPS_MAX;
        // Maximum fps from speed limit:
        float max_fps1 = max_fps();
        if (fvalue > max_fps1)
          fvalue = max_fps1;
        g.reg.fps = fvalue; // Updating the value
        EEPROM.put( g.addr_reg[0], g.reg);
      }

      else if (g.edited_param == PARAM_N_SHOTS)
      {
        int n_shots = (int)(fvalue + 0.5);
        // Enforcing limits:
        if (n_shots < N_SHOTS_MIN)
          n_shots = N_SHOTS_MIN;
        if (n_shots > N_SHOTS_MAX)
          n_shots = N_SHOTS_MAX;
        g.reg.n_shots = n_shots; // Updating the value
        EEPROM.put( g.addr_reg[0], g.reg);
      }

      else if (g.edited_param == PARAM_FIRST_DELAY)
      {
        // Enforcing limits:
        if (fvalue < FIRST_DELAY_MIN)
          fvalue = FIRST_DELAY_MIN;
        if (fvalue > FIRST_DELAY_MAX)
          fvalue = FIRST_DELAY_MAX;
        g.reg.first_delay = fvalue; // Updating the value
        EEPROM.put( g.addr_reg[0], g.reg);
      }

      else if (g.edited_param == PARAM_SECOND_DELAY)
      {
        // Enforcing limits:
        if (fvalue < SECOND_DELAY_MIN)
          fvalue = SECOND_DELAY_MIN;
        if (fvalue > SECOND_DELAY_MAX)
          fvalue = SECOND_DELAY_MAX;
        g.reg.second_delay = fvalue; // Updating the value
        EEPROM.put( g.addr_reg[0], g.reg);
      }

      else if (g.edited_param == PARAM_GOTO)
      {
        if (g.paused)
        {
          short int frame_counter0 = g.frame_counter;
          g.frame_counter = (int)(fvalue + 0.5) - 1;
          if (g.frame_counter < 0)
            g.frame_counter = 0;
          if (g.frame_counter > g.Nframes-1)
            g.frame_counter = g.Nframes-1;
          COORD_TYPE ipos_target = frame_coordinate();
          move_to_next_frame(&ipos_target, &frame_counter0);
        }
        else
        {
          // Destination coordinate (in microsteps):
          COORD_TYPE ipos1 = (fvalue / MM_PER_MICROSTEP) + 0.5;
          // Enforcing limits:
          if (ipos1 < 0)
            ipos1 = 0;
          if (ipos1 > g.limit2)
            ipos1 = g.limit2;
          go_to(ipos1, SPEED_LIMIT); // Moving to the target position with maximum speed/acceleration
        }
      }

    }

    g.Nframes = Nframes();
    g.editing = 0;
    tft.setTextFont(2);
    display_all();
  }


  return;
}


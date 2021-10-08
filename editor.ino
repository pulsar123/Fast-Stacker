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
    g.alt_flag = 0;
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
        #if defined(BL_DEBUG)
        tft.println("BACKLASH value in");
        tft.println("microsteps");
        #elif defined (BL2_DEBUG)
        tft.println("BACKLASH_2 value in");
        tft.println("microsteps");
        #elif defined (DELAY_DEBUG)
        tft.println("SHUTTER_ON_DELAY2");
        tft.println("value in ms");
        #else
        tft.println("Distance between adjacent");
        tft.println("frames in micrometers.");
        tft.println("Will be rounded up to the");
        tft.println("nearest whole microstep.");
        #endif
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

      case PARAM_N_TIMELAPSE:
        tft.println("Number of passes in a");
        tft.println("timelapse sequence.");
        tft.println("(Set to 1 to disable");
        tft.println("timelapsing.)");
        break;

      case PARAM_DT_TIMELAPSE:
        tft.println("Time interval");
        tft.println("(seconds) between");
        tft.println("timelapse passes.");
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
    byte comment_code = 0; // If it becomes non-zero, a comment line will be displayed after editing is done

    if (g.cursor_pos > 0)
    {
      // First - converting to float
      FLOAT_TYPE fvalue = atof(g.value);
      if (g.edited_param == PARAM_MSTEP)
      {
        #if defined(BL_DEBUG)
        g.backlash = (int)(fvalue + 0.5); // Rounding up to the nearest integer value
        // Enforcing limits:
        if (g.backlash < 0)
          g.backlash = 0;
        if (g.backlash > 10000) // A sane upper limit
          g.backlash = 10000;

        #elif defined (BL2_DEBUG)
        BACKLASH_2 = (int)(fvalue + 0.5); // Rounding up to the nearest integer value
        // Enforcing limits:
        if (BACKLASH_2 < 0)
          BACKLASH_2 = 0;
        if (BACKLASH_2 > 10000) // A sane upper limit
          BACKLASH_2 = 10000;

        #elif defined (DELAY_DEBUG)
        SHUTTER_ON_DELAY2 = (int)(fvalue + 0.5); // Rounding up to the nearest integer value
        // Enforcing limits:
        if (SHUTTER_ON_DELAY2 < 1)
          SHUTTER_ON_DELAY2 = 1;
        if (SHUTTER_ON_DELAY2 > 10000) // A sane upper limit
          SHUTTER_ON_DELAY2 = 10000;
        SHUTTER_ON_DELAY2 = 1000 * SHUTTER_ON_DELAY2;  // Converting ms -> us

        #else
        FLOAT_TYPE n_steps = fvalue / 1000.0 / MM_PER_MICROSTEP;
        COORD_TYPE in_steps = (COORD_TYPE)(n_steps + 0.5); // Rounding up to the nearest integer value
        // Enforcing limits:
        if (in_steps < MSTEP_MIN)
        {
          comment_code = 3;
          in_steps = MSTEP_MIN;
        }
        if (in_steps > MSTEP_MAX)
        {
          comment_code = 4;
          in_steps = MSTEP_MAX;
        }
        // Step is limited by the maximum speed and FPS:
        int max_step = (int)(1e6 * SPEED_LIMIT / g.reg.fps); 
        if (in_steps > max_step)
        {
          in_steps = max_step;
          comment_code = 1;
        }
        g.reg.mstep = in_steps; // Updating the value
        EEPROM.put( g.addr_reg[0], g.reg);
        #endif
      }

      else if (g.edited_param == PARAM_FPS)
      {
        // Enforcing limits:
        if (fvalue < FPS_MIN)
        {
          comment_code = 3;
          fvalue = FPS_MIN;
        }
        if (fvalue > FPS_MAX)
        {
          comment_code = 4;
          fvalue = FPS_MAX;
        }
        // Maximum fps from speed limit:
        FLOAT_TYPE max_fps1 = max_fps();
        if (fvalue > max_fps1)
        {
          fvalue = max_fps1;
          comment_code = 2;
        }
        g.reg.fps = fvalue; // Updating the value
        EEPROM.put( g.addr_reg[0], g.reg);
      }

      else if (g.edited_param == PARAM_N_SHOTS)
      {
        int n_shots = (int)(fvalue + 0.5);
        // Enforcing limits:
        if (n_shots < N_SHOTS_MIN)
        {
          comment_code = 3;
          n_shots = N_SHOTS_MIN;
        }
        if (n_shots > N_SHOTS_MAX)
        {
          comment_code = 4;
          n_shots = N_SHOTS_MAX;
        }
        g.reg.n_shots = n_shots; // Updating the value
        EEPROM.put( g.addr_reg[0], g.reg);
      }

      else if (g.edited_param == PARAM_FIRST_DELAY)
      {
        // Enforcing limits:
        if (fvalue < FIRST_DELAY_MIN)
        {
          comment_code = 3;
          fvalue = FIRST_DELAY_MIN;
        }
        if (fvalue > FIRST_DELAY_MAX)
        {
          comment_code = 4;
          fvalue = FIRST_DELAY_MAX;
        }
        g.reg.first_delay = fvalue; // Updating the value
        EEPROM.put( g.addr_reg[0], g.reg);
      }

      else if (g.edited_param == PARAM_SECOND_DELAY)
      {
        // Enforcing limits:
        if (fvalue < SECOND_DELAY_MIN)
        {
          comment_code = 3;
          fvalue = SECOND_DELAY_MIN;
        }
        if (fvalue > SECOND_DELAY_MAX)
        {
          comment_code = 4;
          fvalue = SECOND_DELAY_MAX;
        }
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
          {
            comment_code = 3;
            g.frame_counter = 0;
          }
          if (g.frame_counter > g.Nframes - 1)
          {
            comment_code = 4;
            g.frame_counter = g.Nframes - 1;
          }
          COORD_TYPE ipos_target = frame_coordinate();
          move_to_next_frame(&ipos_target, &frame_counter0);
        }
        else
        {
          // Destination coordinate (in microsteps):
          COORD_TYPE ipos1 = (fvalue / MM_PER_MICROSTEP) + 0.5;
          // Enforcing limits:
          if (ipos1 < 0)
          {
            comment_code = 3;
            ipos1 = 0;
          }
          if (ipos1 > g.limit2)
          {
            comment_code = 4;
            ipos1 = g.limit2;
          }
          go_to(ipos1, SPEED_LIMIT); // Moving to the target position with maximum speed/acceleration
        }
      }

      else if (g.edited_param == PARAM_N_TIMELAPSE)
      {
        int n_timelapse = (int)(fvalue + 0.5);
        // Enforcing limits:
        if (n_timelapse < N_TIMELAPSE_MIN)
        {
          comment_code = 3;
          n_timelapse = N_TIMELAPSE_MIN;
        }
        if (n_timelapse > N_TIMELAPSE_MAX)
        {
          comment_code = 4;
          n_timelapse = N_TIMELAPSE_MAX;
        }
        g.reg.n_timelapse = n_timelapse; // Updating the value
        EEPROM.put( g.addr_reg[0], g.reg);
      }

      else if (g.edited_param == PARAM_DT_TIMELAPSE)
      {
        // Enforcing limits:
        if (fvalue < DT_TIMELAPSE_MIN)
        {
          comment_code = 3;
          fvalue = DT_TIMELAPSE_MIN;
        }
        if (fvalue > DT_TIMELAPSE_MAX)
        {
          comment_code = 4;
          fvalue = DT_TIMELAPSE_MAX;
        }
        g.reg.dt_timelapse = fvalue; // Updating the value
        EEPROM.put( g.addr_reg[0], g.reg);
      }
    }

    g.Nframes = Nframes();
    g.editing = 0;
    tft.setTextFont(2);
    display_all();

    if      (comment_code == 1)
      display_comment_line("   Reduce FPS!  ");
    else if (comment_code == 2)
      display_comment_line("   Reduce Step! ");
    else if (comment_code == 3)
      display_comment_line("    Too small!  ");
    else if (comment_code == 4)
      display_comment_line("    Too large!  ");
      
  }

  return;
}


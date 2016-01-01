void process_keypad()
/*
 All the keypad runtime stuff goes here
 */
{
  float speed, pos_target;
  short frame_counter0;


  // Ignore keypad during emergency breaking
  if (g.breaking == 1 || (g.calibrate == 3 && g.calibrate_warning == 0) || g.error > 1)
    return;

  if (g.stacker_mode == 1 && g.moving == 0 && g.backlashing == 0)
  {
    if (g.continuous_mode)
    {
      // The flag means we just initiated stacking:
      g.start_stacking = 1;
      letter_status("  ");
    }
    else
    {
      g.start_stacking = 0;
      g.noncont_flag = 1;
      letter_status("S ");
    }
    // Time when stacking was initiated:
    g.t0_stacking = g.t;
    g.frame_counter = 0;
    display_frame_counter();
    g.pos_to_shoot = g.pos_short_old;
    g.stacker_mode = 2;
#ifdef H1.2
    // Initiating AF now:
    digitalWrite(PIN_AF, HIGH);
    g.AF_on = 1;
#ifdef CAMERA_DEBUG
    AF_status(1);
#endif
#endif
  }


  // Reading a keypad key if any:
  char key = keypad.getKey();
  KeyState state = keypad.getState();
  KeyState state1 = keypad.key[1].kstate;

  // This is to keep the non-continuous parameters displayed as long as the key "#" is pressed:
  if (g.comment_flag == 1 && keypad.key[0].kchar == '#')
    // -COMMENT_LINE+10000 is a hack, to reduce to almost zero the time the parameters are displayed:
    // (So basically the parameters are only displayed as long as the "#" key is pressed)
    g.t_comment = g.t - COMMENT_DELAY + 10000;

  if ((keypad.key[0].kstate == PRESSED) && (keypad.key[0].kchar == '#') && (state1 != g.state1_old) && (state1 == PRESSED || state1 == RELEASED))
    // Two-key commands (they all start with "#" key)
  {
    if (state1 == PRESSED)
    {
      switch (keypad.key[1].kchar)
      {
        case 'C': // #C: Initiate a full calibration
          // Ignore if moving:
          if (g.moving == 1 || g.paused)
            break;
          g.calibrate = 3;
          g.calibrate_flag = 0;
          g.calibrate_warning = 1;
          g.calibrate_init = g.calibrate;
          // Displaying the calibrate warning:
          display_all("  ");
          break;

        case 'B':  // #B: Initiate emergency breaking, or abort paused stacking
          if (g.paused && g.moving == 0)
          {
            g.paused = 0;
            g.just_paused = 0;
            g.frame_counter = 0;
            display_frame_counter();
            letter_status("  ");
            g.noncont_flag = 0;
            g.stacker_mode = 0;
          }
          else
          {
            change_speed(0.0, 0, 2);
            // This should be done after change_speed(0.0):
            g.breaking = 1;
            letter_status("B ");
            g.calibrate = 0;
            g.calibrate_flag = 0;
            g.calibrate_warning = 0;
            g.calibrate_init = g.calibrate;
          }
          break;

        case '2': // #2: Save parameters to first memory bank
          if (g.paused)
            break;
          g.reg1 = {g.i_n_shots, g.i_mm_per_frame, g.i_fps, g.i_first_delay, g.i_second_delay, g.point1, g.point2};
          EEPROM.put( ADDR_REG1, g.reg1);
          display_comment_line("Saved to Reg1 ");
          break;

        case '3': // #3: Read parameters from first memory bank
          if (g.paused)
            break;
          EEPROM.get( ADDR_REG1, g.reg1);
          g.i_n_shots = g.reg1.i_n_shots;
          g.i_mm_per_frame = g.reg1.i_mm_per_frame;
          g.i_fps = g.reg1.i_fps;
          g.point1 = g.reg1.point1;
          g.point2 = g.reg1.point2;
          g.i_first_delay = g.reg1.i_first_delay;
          g.i_second_delay = g.reg1.i_second_delay;
          g.msteps_per_frame = Msteps_per_frame();
          g.Nframes = Nframes();
          EEPROM.put( ADDR_I_N_SHOTS, g.i_n_shots);
          EEPROM.put( ADDR_I_MM_PER_FRAME, g.i_mm_per_frame);
          EEPROM.put( ADDR_I_FPS, g.i_fps);
          EEPROM.put( ADDR_POINT1, g.point1);
          EEPROM.put( ADDR_POINT2, g.point2);
          EEPROM.put( ADDR_I_FIRST_DELAY, g.i_first_delay);
          EEPROM.put( ADDR_I_SECOND_DELAY, g.i_second_delay);
          display_all("  ");
          display_comment_line("Read from Reg1");
          break;

        case '5': // #5: Save parameters to second memory bank
          if (g.paused)
            break;
          g.reg2 = {g.i_n_shots, g.i_mm_per_frame, g.i_fps, g.i_first_delay, g.i_second_delay, g.point1, g.point2};
          EEPROM.put( ADDR_REG2, g.reg2);
          display_comment_line("Saved to Reg2 ");
          break;

        case '6': // #6: Read parameters from second memory bank
          if (g.paused)
            break;
          EEPROM.get( ADDR_REG2, g.reg2);
          g.i_n_shots = g.reg2.i_n_shots;
          g.i_mm_per_frame = g.reg2.i_mm_per_frame;
          g.i_fps = g.reg2.i_fps;
          g.point1 = g.reg2.point1;
          g.point2 = g.reg2.point2;
          g.i_first_delay = g.reg2.i_first_delay;
          g.i_second_delay = g.reg2.i_second_delay;
          g.msteps_per_frame = Msteps_per_frame();
          g.Nframes = Nframes();
          EEPROM.put( ADDR_I_N_SHOTS, g.i_n_shots);
          EEPROM.put( ADDR_I_MM_PER_FRAME, g.i_mm_per_frame);
          EEPROM.put( ADDR_I_FPS, g.i_fps);
          EEPROM.put( ADDR_POINT1, g.point1);
          EEPROM.put( ADDR_POINT2, g.point2);
          EEPROM.put( ADDR_I_FIRST_DELAY, g.i_first_delay);
          EEPROM.put( ADDR_I_SECOND_DELAY, g.i_second_delay);
          display_all("  ");
          display_comment_line("Read from Reg2");
          break;

        case '8': // #8: Cycle through the table for FIRST_DELAY parameter
          if (g.paused)
            break;
          if (g.i_first_delay < N_FIRST_DELAY - 1)
            g.i_first_delay++;
          else
            g.i_first_delay = 0;
          EEPROM.put( ADDR_I_FIRST_DELAY, g.i_first_delay);
          // Fill g.buffer with non-continuous stacking parameters, to be displayed with display_comment_line:
          delay_buffer();
          display_comment_line(g.buffer);
          break;

        case '9': // #9: Cycle through the table for SECOND_DELAY parameter
          if (g.paused)
            break;
          if (g.i_second_delay < N_SECOND_DELAY - 1)
            g.i_second_delay++;
          else
            g.i_second_delay = 0;
          EEPROM.put( ADDR_I_SECOND_DELAY, g.i_second_delay);
          // Fill g.buffer with non-continuous stacking parameters, to be displayed with display_comment_line:
          delay_buffer();
          display_comment_line(g.buffer);
          break;

        case '4': // #4: Backlighting control
          g.backlight++;
          // LCD unstable for medium backlight; using only two levels:
          if (g.backlight > 1)
            g.backlight = 0;
          set_backlight();
          break;

        case '*': // #*: Factory reset
          if (g.paused)
            break;
          factory_reset();
          g.calibrate_warning = 1;
          g.calibrate_init = g.calibrate;
          display_all("  ");
          break;

        case '7': // #7: Manual camera shutter triggering
          // Setting the shutter on:
#ifdef H1.2
          digitalWrite(PIN_AF, HIGH);
#ifdef CAMERA_DEBUG
          AF_status(1);
#endif
          g.AF_on = 1;
          g.single_shot = 1;
#endif
          digitalWrite(PIN_SHUTTER, HIGH);
#ifdef CAMERA_DEBUG
          shutter_status(1);
#endif
          g.shutter_on = 1;
          g.t_shutter = g.t;
          break;

        case '1': // #1: Rewind a single frame step (no shooting)
          if (g.moving)
            break;
          frame_counter0 = g.frame_counter;
          if (g.paused)
          {
            // This "if" condition is to ensure that the very first frame back is to the currently displayed frame (as the pause happens between the current and the next frame)
            if (!g.just_paused || g.stacking_direction < 0)
              g.frame_counter = g.frame_counter - g.stacking_direction;
            pos_target = g.starting_point + g.stacking_direction * nintMy(((float)g.frame_counter) * g.msteps_per_frame);
          }
          else
          {
            g.frame_counter--;
            pos_target = g.pos - g.msteps_per_frame;
          }
          // This 100 steps padding is just a hack, to fix the occasional bug when a combination of single frame steps and rewind can
          // move the rail beyond g.limit1
          if (pos_target < (float)g.limit1 + 100.0 || pos_target > (float)g.limit2 - 100.0 || g.paused && (g.frame_counter < 0 || g.frame_counter >= g.Nframes))
          {
            // Recovering the original frame counter if aborting:
            g.frame_counter = frame_counter0;
            break;
          }
          go_to(pos_target, g.speed_limit);
          display_frame_counter();
          g.just_paused = 0;
          break;

        case 'A': // #A: Fast-forward a single frame step (no shooting)
          if (g.moving)
            break;
          // Required microsteps per frame:
          frame_counter0 = g.frame_counter;
          if (g.paused)
          {
            // This if condition is to ensure that the very first frame back is to the currently displayed frame (as the pause happens between the current and the next frame)
            if (!g.just_paused || g.stacking_direction > 0)
              g.frame_counter = g.frame_counter + g.stacking_direction;
            pos_target = g.starting_point + g.stacking_direction * nintMy(((float)g.frame_counter) * g.msteps_per_frame);
          }
          else
          {
            g.frame_counter++;
            pos_target = g.pos + g.msteps_per_frame;
          }
          if (pos_target < (float)g.limit1 + 100.0 || pos_target > (float)g.limit2 - 100.0 || g.paused && (g.frame_counter < 0 || g.frame_counter >= g.Nframes))
          {
            g.frame_counter = frame_counter0;
            break;
          }
          go_to(pos_target, g.speed_limit);
          display_frame_counter();
          g.just_paused = 0;
          break;

        case 'D':  // #D: Go to the last starting point (for both 1- and 2-point shooting); not memorized in EEPROM
          if (g.paused)
            break;
          go_to((float)g.starting_point + 0.5, g.speed_limit);
          display_comment_line(" Going to P0  ");
          break;

        case '0': // #0: Start 2-point focus stacking from the foreground point in a non-continuous mode
          if (g.paused)
            break;
          // Checking the correctness of point1/2
          if (g.point2 > g.point1 && g.point1 >= g.limit1 && g.point2 <= g.limit2)
          {
            // Using the simplest approach which will result the last shot to always slightly undershoot
            g.Nframes = Nframes();
            // Always starting from the foreground point, for full backlash compensation:
            go_to((float)g.point1 + 0.5, g.speed_limit);
            g.starting_point = g.point1;
            g.destination_point = g.point2;
            g.stacking_direction = 1;
            g.stacker_mode = 1;
            // This is a non-continuous mode:
            g.continuous_mode = 0;
            display_comment_line("2-points stack");
          }
          else
          {
            // Should print error message
            display_comment_line("Bad 2 points! ");
          }
          break;

      } // switch
    }
    g.state1_old = state1;
  }

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  else
    // Single-key commands
  {
    // Action is only needed if the kepad state changed since the last time,
    // or if one of the parameter change keys was pressed longer than T_KEY_LAG microseconds:
    if (state != g.state_old && (state == PRESSED || state == RELEASED) ||
        state == PRESSED && g.state_old == PRESSED && (g.key_old == '2' || g.key_old == '3' || g.key_old == '5'
            || g.key_old == '6' || g.key_old == '8' || g.key_old == '9') && g.t - g.t_key_pressed > T_KEY_LAG)
    {

      if (state == PRESSED)
      {
        if (g.state_old != PRESSED)
        {
          g.key_old = key;
          // Memorizing the time when a key was pressed:
          g.t_key_pressed = g.t;
          if (g.calibrate_warning == 1)
            // Any key pressed when calibrate_warning=1 will initiate calibration:
          {
            g.calibrate_warning = 0;
            display_all("  ");
            return;
          }
        }

        else
          // We are here when a change parameter key was pressed longer than T_KEY_LAG
        {
          if (g.N_repeats == 0)
            // Will be used for keys with repetition (parameter change keys):
          {
            g.t_last_repeat = g.t;
            g.N_repeats = 1;
          }
          // We repeat a paramet change key once the time since the last repeat is larger than T_KEY_REPEat:
          if (g.t - g.t_last_repeat > T_KEY_REPEAT)
          {
            g.N_repeats++;
            g.t_last_repeat = g.t;
            // A trick, as "key" has only proper value when just pressed
            key = g.key_old;
          }
          else
            // Too early for a repeated key, so returning now:
            return;
        }

        // Keys interpretation depends on the stacker_mode:
        if (g.stacker_mode == 0)
          // Mode 0: default; rewinding etc.
        {
          // When error 1 (limiter on initially), the only commands accepted are rewind and fast forward:
          if (g.error == 1 && key != '1' && key != 'A')
            return;

          switch (key)
          {
            case '1':  // 1: Rewinding, or moving 10 frames back for the current stacking direction (if paused)
              if (g.pos_short_old <= g.limit1)
                break;
#ifdef MOTOR_DEBUG
              cplus1 = cminus1 = cplus2 = cminus2 = skipped_current = 0;
              cmax = 0;  istep = 0;
#endif
              if (g.paused)
              {
                if (g.moving)
                  break;
                frame_counter0 = g.frame_counter;
                g.frame_counter = g.frame_counter - 10 * g.stacking_direction;
                pos_target = g.starting_point + g.stacking_direction * nintMy(((float)g.frame_counter) * g.msteps_per_frame);
                if (pos_target < (float)g.limit1 + 100.0 || pos_target > (float)g.limit2 - 100.0 || g.paused && (g.frame_counter < 0 || g.frame_counter >= g.Nframes))
                {
                  g.frame_counter = frame_counter0;
                  break;
                }
                go_to(pos_target, g.speed_limit);
                display_frame_counter();
              }
              else
              {
                g.direction = -1;
                motion_status();
                // Rewinding is done with small acceleration:
                change_speed(-g.speed_limit, 0, 1);
              }
              g.just_paused = 0;
              break;

            case 'A':  // A: Fast forwarding, or moving 10 frames forward for the current stacking direction (if paused)
              if (g.pos_short_old >= g.limit2)
                break;
#ifdef MOTOR_DEBUG
              cplus1 = cminus1 = cplus2 = cminus2 = skipped_current = 0;
              cmax = 0;  istep = 0;
#endif
              if (g.paused)
              {
                if (g.moving)
                  break;
                frame_counter0 = g.frame_counter;
                g.frame_counter = g.frame_counter + 10 * g.stacking_direction;
                pos_target = g.starting_point + g.stacking_direction * nintMy(((float)g.frame_counter) * g.msteps_per_frame);
                if (pos_target < (float)g.limit1 + 100.0 || pos_target > (float)g.limit2 - 100.0 || g.paused && (g.frame_counter < 0 || g.frame_counter >= g.Nframes))
                {
                  g.frame_counter = frame_counter0;
                  break;
                }
                go_to(pos_target, g.speed_limit);
                display_frame_counter();
              }
              else
              {
                g.direction = 1;
                motion_status();
                // Rewinding is done with small acceleration:
                change_speed(g.speed_limit, 0, 1);
              }
              g.just_paused = 0;
              break;

            case '4':  // 4: Set foreground point
              if (g.paused || g.moving)
                break;
              g.point1 = g.pos_short_old;
              if (g.points_byte == 0 || g.points_byte == 2)
                g.points_byte = g.points_byte + 1;
              g.msteps_per_frame = Msteps_per_frame();
              g.Nframes = Nframes();
              points_status();
              display_two_point_params();
              display_two_points();
              display_comment_line("  P1 was set  ");
              EEPROM.put( ADDR_POINT1, g.point1);
              EEPROM.put( ADDR_POINTS_BYTE, g.points_byte);
              break;

            case 'B':  // B: Set background point
              if (g.paused || g.moving)
                break;
              g.point2 = g.pos_short_old;
              if (g.points_byte == 0 || g.points_byte == 1)
                g.points_byte = g.points_byte + 2;
              g.msteps_per_frame = Msteps_per_frame();
              g.Nframes = Nframes();
              points_status();
              display_two_point_params();
              display_two_points();
              display_comment_line("  P2 was set  ");
              EEPROM.put( ADDR_POINT2, g.point2);
              EEPROM.put( ADDR_POINTS_BYTE, g.points_byte);
              break;

            case '7':  // 7: Go to the foreground point
              if (g.paused)
                break;
#ifdef MOTOR_DEBUG
              cplus1 = cminus1 = cplus2 = cminus2 = skipped_current = 0;
              cmax = 0;  istep = 0;
#endif
              go_to((float)g.point1 + 0.5, g.speed_limit);
              display_comment_line(" Going to P1  ");
              break;

            case 'C':  // C: Go to the background point
              if (g.paused)
                break;
#ifdef MOTOR_DEBUG
              cplus1 = cminus1 = cplus2 = cminus2 = skipped_current = 0;
              cmax = 0;  istep = 0;
#endif
              go_to((float)g.point2 + 0.5, g.speed_limit);
              display_comment_line(" Going to P2  ");
              break;

            case '0': // 0: Start shooting (2-point focus stacking) from the foreground point (backlash compensated)
              // Checking the correctness of point1/2
              if (g.point2 > g.point1 && g.point1 >= g.limit1 && g.point2 <= g.limit2)
              {
                if (g.paused)
                  // Resuming 2-point stacking from a paused state
                {
                  if (g.moving)
                    break;
                  g.paused = 0;
                  if (g.continuous_mode)
                  {
                    // The flag means we just initiated stacking:
                    g.start_stacking = 1;
                    letter_status("  ");
                  }
                  else
                  {
                    g.start_stacking = 0;
                    g.noncont_flag = 1;
                    letter_status("S ");
                  }
                  // Time when stacking was initiated:
                  g.t0_stacking = g.t;
                  g.pos_to_shoot = g.pos_short_old;
                  g.stacker_mode = 2;
                }
                else
                {
                  // Using the simplest approach which will result the last shot to always slightly undershoot
                  g.Nframes = Nframes();
                  go_to((float)g.point1 + 0.5, g.speed_limit);
                  g.starting_point = g.point1;
                  g.destination_point = g.point2;
                  g.stacking_direction = 1;
                  g.stacker_mode = 1;
                  g.continuous_mode = 1;
                  display_comment_line("2-points stack");
                }
              }
              else
              {
                // Should print error message
                display_comment_line("Bad 2 points! ");
              }
              g.just_paused = 0;
              break;

            case '*':  // *: Initiate one-point focus stacking backwards (not backlash compensated)
              if (g.paused)
                break;
              // Simplest workaround: ignore the command if currently moving
              // (better solution would be to stop first)
              if (!g.moving)
              {
                // The flag means we just initiated stacking:
                g.start_stacking = 1;
                // Time when stacking was initiated:
                g.t0_stacking = g.t;
                g.frame_counter = 0;
                display_frame_counter();
                g.pos_to_shoot = g.pos_short_old;
                g.starting_point = g.pos_short_old;
                g.stacking_direction = -1;
                g.stacker_mode = 3;
                g.continuous_mode = 1;
#ifdef H1.2
                // Initiating AF now:
                digitalWrite(PIN_AF, HIGH);
                g.AF_on = 1;
#endif                
#ifdef CAMERA_DEBUG
                AF_status(1);
#endif
                display_comment_line("1-point stack ");
              }
              break;

            case 'D':  // D: Initiate one-point focus stacking forward
              if (g.paused)
                break;
              if (!g.moving)
              {
                // The flag means we just initiated stacking:
                g.start_stacking = 1;
                // Time when stacking was initiated:
                g.t0_stacking = g.t;
                g.frame_counter = 0;
                display_frame_counter();
                g.pos_to_shoot = g.pos_short_old;
                g.starting_point = g.pos_short_old;
                g.stacking_direction = 1;
                g.stacker_mode = 3;
                g.continuous_mode = 1;
#ifdef H1.2
                // Initiating AF now:
                digitalWrite(PIN_AF, HIGH);
                g.AF_on = 1;
#endif                
#ifdef CAMERA_DEBUG
                AF_status(1);
#endif
                display_comment_line("1-point stack ");
              }
              break;

            case '2':  // 2: Decrease parameter n_shots (for 1-point sstacking)
              if (g.paused)
                break;
              if (g.i_n_shots > 0)
                g.i_n_shots--;
              else
                break;
              EEPROM.put( ADDR_I_N_SHOTS, g.i_n_shots);
              display_all("  ");
              break;

            case '3':  // 3: Increase parameter n_shots (for 1-point sstacking)
              if (g.paused)
                break;
              if (g.i_n_shots < N_PARAMS - 1)
                g.i_n_shots++;
              else
                break;
              EEPROM.put( ADDR_I_N_SHOTS, g.i_n_shots);
              display_one_point_params();
              break;

            case '5':  // 5: Decrease parameter mm_per_frame
              if (g.paused)
                break;
              if (g.i_mm_per_frame > 0)
                g.i_mm_per_frame--;
              else
                break;
              // Required microsteps per frame:
              g.msteps_per_frame = Msteps_per_frame();
              g.Nframes = Nframes();
              display_all("  ");
              EEPROM.put( ADDR_I_MM_PER_FRAME, g.i_mm_per_frame);
              break;

            case '6':  // 6: Increase parameter mm_per_frame
              if (g.paused)
                break;
              if (g.i_mm_per_frame < N_PARAMS - 1)
              {
                g.i_mm_per_frame++;
                // Estimating the required speed in microsteps per microsecond
                speed = target_speed();
                // Reverting back if required speed > maximum allowed:
                if (speed > g.speed_limit)
                {
                  g.i_mm_per_frame--;
                  break;
                }
              }
              else
                break;
              // Required microsteps per frame:
              g.msteps_per_frame = Msteps_per_frame();
              g.Nframes = Nframes();
              EEPROM.put( ADDR_I_MM_PER_FRAME, g.i_mm_per_frame);
              display_all("  ");
              break;

            case '8':  // 8: Decrease parameter fps
              if (g.paused)
                break;
              if (g.i_fps > 0)
                g.i_fps--;
              else
                break;
              EEPROM.put( ADDR_I_FPS, g.i_fps);
              display_all("  ");
              break;

            case '9':  // 9: Increase parameter fps
              if (g.paused)
                break;
              if (g.i_fps < N_PARAMS - 1)
              {
                g.i_fps++;
                // Estimating the required speed in microsteps per microsecond
                speed = target_speed();
                // Reverting back if required speed > maximum allowed:
                if (speed > g.speed_limit)
                {
                  g.i_fps--;
                  break;
                }
              }
              else
                break;
              EEPROM.put( ADDR_I_FPS, g.i_fps);
              display_all("  ");
              break;

            case '#': // #: Show the non-continuous parameters in the 5th line of the LCD
              if (g.moving || g.paused)
                break;
              delay_buffer();
              display_comment_line(g.buffer);
              break;

          } // End of case
        }  // if (g.stacker_mode == 0)

        else if (g.stacker_mode > 0)
          // Mode 1/2: focus stacking
        {
          // Any key press in stacking mode interrupts stacking
          if (g.moving)
            change_speed(0.0, 0, 2);
          if (g.stacker_mode == 2)
            // In 2-point stacking, we pause
          {
            display_comment_line("    Paused    ");
            letter_status("P ");
            g.paused = 1;
            g.just_paused = 1;
            // This seems to have fixed the bug with the need to double click keys in non-continuous paused mode:
            g.state1_old = (KeyState)0;
            // Switches the frame counter back to the last accomplished frame
            g.frame_counter--;
          }
          else
            // In 1-point stacking, we abort
          {
            g.frame_counter = 0;
            display_comment_line("Stacking abort");
          }
          g.stacker_mode = 0;
        }

      }  // if (PRESSED)

      else
        // if a key was just released
      {
        // Resetting the counter of key repeats:
        g.N_repeats = 0;
        // Breaking / stopping if no keys pressed (only after rewind/fastforward)
        if ((g.key_old == '1' || g.key_old == 'A') && g.moving == 1 && state == RELEASED && g.paused == 0)
        {
#ifdef EXTENDED_REWIND
          if (g.key_old == '1' && g.speed < 0.0)
            // Moving in the bad (negative) direction
          {
            // Estimating how much rail would travel if the maximum breaking started now (that's how much
            // rail would actually travel if moving in the good direction, or if backlash was zero):
            // Stopping distance in the current direction:
            float dx_stop = g.speed * g.speed / (2.0 * ACCEL_LIMIT);
            // The physical coordinate where we have to stop:
            float pos1 = g.pos - dx_stop;
            // To mimick the good direction (key "A") behaviour, we replace emergency breaking with a go_to call:
            // (All technicalities - backlash compensation, limit of decceleration - will be handled by go_to)
            go_to(pos1, g.speed_limit);
          }
          else
#endif
            change_speed(0.0, 0, 2);
        }
      }

      g.state_old = state;
    }  // End of if(keyStateChanged)
  } // End of two-key / one-key if


  return;
}


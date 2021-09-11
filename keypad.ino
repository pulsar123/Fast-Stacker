void process_keypad()
/*
  All the keypad runtime stuff goes here

  For each key command, use the following comment format (after "case 'x':")
    //@ KEY Key description

  Then one can easily list all the key commands by executing the following code (bash - under cygwin or Linux):

  grep @ keypad.ino | cut -d@ -f2 | sort

*/
{
  float speed, dx_stop;
  short frame_counter0;
  COORD_TYPE ipos_target;


  // Ignore keypad during emergency breaking
  if (g.uninterrupted == 1)
    return;

  // Treating delayed keyes here:
  if (g.init_delayed_key == 1 && micros() - g.t_delayed_key > KEY_DELAY_US)
  {
    g.init_delayed_key = 0;

    switch (keypad.key[0].kchar)
    {
      case '4':
        // Set foreground point
        set_memory_point(1);
        break;

      case 'B':
        // Set background point
        set_memory_point(2);
        break;
    }
  }

  // The previous value of the key 0:
  g.key_old = keypad.key[0].kchar;

  // This is a trick to generate multiple actions when you press certain keys long enough
  // (longer than T_KEY_LAG). The multiple actions are separated by delays T_KEY_REPEAT.
  // The trick is to generate fake key press events for the currently pressed key. Flag fake_key
  // is used to differentiate bwetween a real key press (then it is '0') and fake key press (it is '1').
  char fake_key = 0;
  // This is the list of the all keys (only one-key bindings are allowed) with multiple actions:
  if (g.help_mode == 1 && (g.key_old == '1' || g.key_old == 'A') && g.editing == 0 && g.moving == 0
      && g.t - g.t_key_pressed > T_KEY_LAG)
    // We are here when a change parameter key was pressed longer than T_KEY_LAG
  {
    if (g.N_repeats == 0)
      // Generating the first fake key event:
    {
      g.t_last_repeat = g.t;
      g.N_repeats = 1;
      fake_key = 1;
    }
    if (g.t - g.t_last_repeat > T_KEY_REPEAT)
      // Generating subsequent fake key events:
    {
      g.N_repeats++;
      g.t_last_repeat = g.t;
      fake_key = 1;
    }
  }

  // Rescanning the keys. Most of the calls return false (no scan performed), exiting immediately if so
  if (!keypad.getKeys() && !fake_key)
    return;

  KeyState state0, state1;
  char key0;
  bool state0_changed;
  if (fake_key)
    // Simulating a fake key (for repetitive key actions when certain keys are pressed long enough)
  {
    state0 = PRESSED;
    key0 = g.key_old;
    state0_changed = 1;
  }
  else
    // Processing real (not fake) key press
  {
    state0 = keypad.key[0].kstate;
    state1 = keypad.key[1].kstate;
    key0 = keypad.key[0].kchar;
    state0_changed = keypad.key[0].stateChanged;
  }

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Two-key #X commands (no fake key events allowed)
  if (state0 == PRESSED && keypad.key[0].kchar == '#' && keypad.key[1].stateChanged && state1 == PRESSED && !fake_key && g.editing == 0 && g.help_mode == 0)
  {
    switch (keypad.key[1].kchar)
    {
      case 'C': //@ #C: Initiate a full calibration
        // Ignore if moving:
        if (g.moving == 1 || g.paused || g.limit_on)
          break;
        g.limit1 = -HUGE;
        g.limit2 = HUGE;
        g.calibrate_flag = 1;
        g.error = 4;
        // Displaying the calibrate warning:
        display_all();
        break;

      case 'B': //@ #B: Initiate emergency breaking, or abort paused stacking
        if (g.paused && g.moving == 0)
          // Aborting stacking:
        {
          g.paused = 0;
          g.frame_counter = 0;
          display_frame_counter();
          g.noncont_flag = 0;
          g.stacker_mode = 0;
          g.timelapse_mode = 0;
          g.end_of_stacking = 0;
          display_all();
        }
        else if (g.moving == 1)
          // Emergency breaking:
        {
          start_breaking();
          g.calibrate_flag = 0;
        }
#ifdef TEST_SWITCH
        g.test_flag = 10;
#endif
        break;

      case '2': //@ #2: Save parameters to first memory bank
        if (!g.paused)
          save_params(1);
        break;

      case '3': //@ #3: Read parameters from first memory bank
        if (!g.paused)
          read_params(1);
        break;

      case '5': //@ #5: Save parameters to second memory bank
        if (!g.paused)
          save_params(2);
        break;

      case '6': //@ #6: Read parameters from second memory bank
        if (!g.paused)
          read_params(2);
        break;

      case '8': //@ #8: Save parameters to third memory bank
        if (!g.paused)
          save_params(3);
        break;

      case '9': //@ #9: Read parameters from third memory bank
        if (!g.paused)
          read_params(3);
        break;

      case '*': //@ #*: Factory reset
        if (g.paused || g.limit_on)
          break;
        g.error = 3;
        display_all();
        break;

      case '7': //@ #7: Manual camera shutter triggering
        if (g.moving)
          break;
        g.make_shot = 1;
        g.single_shot = 1;
        g.start_stacking = 0;
        g.stacker_mode = 0;
        break;

      case '1': //@ #1: Rewind a single frame step (no shooting)
        if (g.moving || g.paused > 1)
          break;
        frame_counter0 = g.frame_counter;
        g.frame_counter--;
        if (g.paused)
          ipos_target = frame_coordinate();
        else
          ipos_target = g.ipos - g.reg.mstep;
        move_to_next_frame(&ipos_target, &frame_counter0);
        break;

      case 'A': //@ #A: Fast-forward a single frame step (no shooting)
        if (g.moving || g.paused > 1)
          break;
        frame_counter0 = g.frame_counter;
        g.frame_counter++;
        if (g.paused)
          ipos_target = frame_coordinate();
        else
          ipos_target = g.ipos + g.reg.mstep;
        move_to_next_frame(&ipos_target, &frame_counter0);
        break;

      case 'D': //@ #D: Go to the last starting point (for both 1- and 2-point shooting); not memorized in EEPROM
        if (g.paused)
          break;
        go_to(g.starting_point, SPEED_LIMIT);
        display_comment_line("    Going to P0     ");
        break;

    } // switch
  }


  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Two-key *X commands (don't work for paused and moving states; no fake key events allowed)
  else if (state0 == PRESSED && keypad.key[0].kchar == '*' && keypad.key[1].stateChanged && state1 == PRESSED && !fake_key)

  {
    if (g.moving == 0 && g.paused == 0 && g.editing == 0 && g.help_mode == 0)
    {
      switch (keypad.key[1].kchar)
      {
        case '1': //@ *1: Rail reverse
          if (g.ipos - g.limit1 <= BACKLASH + BACKLASH_2 || g.limit2 - g.ipos <= BACKLASH + BACKLASH_2)
            // Being too close to limiters seems to be causing issues. Aborting
          {
            g.alt_flag = 0;
            display_all();
            display_comment_line("Too close to limiters");
            break;
          }
          g.reg.straight = 1 - g.reg.straight;
          EEPROM.put( g.addr_reg[0], g.reg);
          // Reversing the rail and updating the point1,2 parameters:
          rail_reverse(1);
          display_all();
          break;

        case '2': //@ *2: Save parameters to 4th memory bank
          save_params(4);
          break;

        case '3': //@ *3: Read parameters from 4th memory bank
          read_params(4);
          break;

        case '5': //@ *5: Save parameters to 5th memory bank
          save_params(5);
          break;

        case '6': //@ *6: Read parameters from 5th memory bank
          read_params(5);
          break;

        case '8': //@ *8:
          //          save_params(6);
          break;

        case '9': //@ *9: Set acceleration_factor2 (for stopping)
          //          read_params(6);
          if (g.reg.i_accel_factor2 < N_ACCEL_FACTOR - 1)
            g.reg.i_accel_factor2++;
          else
            g.reg.i_accel_factor2 = 0;
          EEPROM.put( g.addr_reg[0], g.reg);
          display_all();
          // Five possible floating point values for acceleration
          set_accel_v();
          break;

        case 'A': //@ *A: Change accel_factor
          if (g.reg.i_accel_factor < N_ACCEL_FACTOR - 1)
            g.reg.i_accel_factor++;
          else
            g.reg.i_accel_factor = 0;
          EEPROM.put( g.addr_reg[0], g.reg);
          display_all();
          // Five possible floating point values for acceleration
          set_accel_v();
          break;

        case 'B': //@ *B: Backlash compensation: -1, 0, 1 (camera looking down; no backlash; camera looking up)
          if (g.reg.backlash_on < 1)
            g.reg.backlash_on++;
          else
            g.reg.backlash_on = -1;
          update_backlash();
          display_all();
          EEPROM.put( g.addr_reg[0], g.reg);
          break;

        case 'C': //@ *C: Mirror lock: 0-3
          if (g.reg.mirror_lock < 3)
            g.reg.mirror_lock++;
          else
            g.reg.mirror_lock = 0;
          display_all();
          EEPROM.put( g.addr_reg[0], g.reg);
          break;


        case 'D': //@ *D: Buzzer on/off
          g.reg.buzzer = 1 - g.reg.buzzer;
          display_all();
          EEPROM.put( g.addr_reg[0], g.reg);
          break;


        case '4': //@ *4: Change N_timelapse
          g.editing = 1;
          g.edited_param = PARAM_N_TIMELAPSE;
          editor('I');
          break;

        case '7': //@ *7: Change dt_timelapse
          g.editing = 1;
          g.edited_param = PARAM_DT_TIMELAPSE;
          editor('I');
          break;

        case '0': //@ *0: Save energy on / off
          g.reg.save_energy = 1 - g.reg.save_energy;
          update_save_energy();
          display_all();
          EEPROM.put( g.addr_reg[0], g.reg);
          break;

      } // switch
    }
  }

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Single-key commands (fake key events are allowed)
  else
  {
    if (state0_changed && (state0 == PRESSED || state0 == RELEASED))
    {

      if (state0 == PRESSED)
      {
        if (state0_changed && !fake_key)
        {
          // Memorizing the time when a key was pressed:
          // (Only when it is a true state change, not a fake key)
          g.t_key_pressed = g.t;
          // Any key pressed when g.error=4 (calibration warning) will initiate calibration
          // Also, pressing any key except for '1' will cancel factory reset
          if (g.error == 4 || g.error == 3 && key0 != '1')
          {
            g.error = 0;
            display_all();
            return;
          }
        }

        // Keys interpretation depends on the stacker_mode:
        if (g.stacker_mode == 0)
          // Mode 0: default; rewinding etc.
        {
          // When error 1 (limiter on initially), the only commands accepted are rewind and fast forward:
          if (g.error == 1 && key0 != '1' && key0 != 'A')
          {
            return;
          }

          if (g.help_mode == 1 && key0 != '1' && key0 != 'A')
            // Any key except for 1 and A exits help screen:
          {
            g.help_mode = 0;
            display_all();
            return;
          }

          switch (key0)
          {
            case '1':  //@ 1: Rewinding, or moving 10 frames back for the current stacking direction (if paused)
              // Using key "1" to confirm factory reset
              if (g.editing == 1)
              {
                editor(key0);
                break;
              }
              if (g.help_mode == 1)
              {
                g.help_page--;
                if (g.help_page < 0)
                {
                  g.help_page = 0;
                  break;
                }
                help();
                break;
              }
              if (g.error == 3)
              {
                initialize(1);
                break;
              }
              else if (g.paused > 1)
                break;
              if (g.paused)
              {
                if (g.moving)
                  break;
                frame_counter0 = g.frame_counter;
                g.frame_counter = g.frame_counter - 10;
                ipos_target = frame_coordinate();
                move_to_next_frame(&ipos_target, &frame_counter0);
              }
              else
              {
                if (g.model_type == MODEL_NONE || g.model_type == MODEL_FF || g.model_type == MODEL_STOP)
                  // Rewinding is done with small acceleration:
                {
                  g.model_type = MODEL_REWIND;
                  g.model_init = 1;
                }
              }
              break;

            case 'A':  //@ A: Fast forwarding, or moving 10 frames forward for the current stacking direction (if paused)
              if (g.editing == 1)
              {
                editor(key0);
                break;
              }
              if (g.help_mode == 1)
              {
                g.help_page++;
                if (g.help_page > N_HELP_PAGES - 1)
                {
                  g.help_page = N_HELP_PAGES - 1;
                  break;
                }
                help();
                break;
              }
              if (g.paused > 1)
                break;
              if (g.paused)
              {
                if (g.moving)
                  break;

                // Moving +10 frames when paused:
                frame_counter0 = g.frame_counter;
                g.frame_counter = g.frame_counter + 10;
                ipos_target = frame_coordinate();
                move_to_next_frame(&ipos_target, &frame_counter0);

              }
              else
              {
                if (g.model_type == MODEL_NONE || g.model_type == MODEL_REWIND || g.model_type == MODEL_STOP)
                  // Fastforwarding is done with small acceleration:
                {
                  g.model_type = MODEL_FF;
                  g.model_init = 1;
                }
              }
              break;

            case '4': //@ 4: Set foreground point (delayed)
              if (g.editing == 0)
              {
                //                set_memory_point(1);
                g.init_delayed_key = 1;
                g.t_delayed_key = micros();
              }
              else
                editor('4');
              break;

            case 'B': //@ B: Set background point (delayed)
              if (g.editing == 0)
              {
                //                set_memory_point(2);
                g.init_delayed_key = 1;
                g.t_delayed_key = micros();
              }
              else
                editor('B');
              break;

            case '7':  //@ 7: Go to the foreground point
              if (g.editing == 1)
              {
                editor(key0);
                break;
              }
              goto_memory_point(1);
              break;

            case 'C':  //@ C: Go to the background point
              if (g.editing == 1)
              {
                editor(key0);
                break;
              }
              goto_memory_point(2);
              break;


            case 'D': //@ D: Start shooting the stacking sequence
              if (g.editing || g.moving)
                break;
              // Checking the correctness of point1/2
              if (g.reg.point[BACKGROUND] < g.reg.point[FOREGROUND] || g.reg.point[FOREGROUND] < 0 || g.reg.point[BACKGROUND] > g.limit2)
              {
                // Should print error message
                display_comment_line("   Bad 2 points!    ");
                break;
              }
              if (g.paused > 0 && (g.reg.i_mode == CONT_MODE || g.reg.i_mode == NONCONT_MODE))
              {
                if (g.paused == 1)
                  // Resuming 2-point stacking from a paused state
                {
                  g.paused = 0;
                  g.start_stacking = 1;
                  if (g.continuous_mode)
                  {
                    // The flag means we just initiated stacking:
                    letter_status(" ");
                  }
                  else
                  {
                    g.noncont_flag = 1;
                    letter_status("S");
                  }
                  // Time when stacking was initiated:
                  g.t0_stacking = g.t;
                  g.ipos_to_shoot = g.ipos;
                  g.stacker_mode = 2;
                }
                else if (g.paused == 3)
                  // Restarting from a pause which happened between stacks (in timelapse mode)
                {
                  g.paused = 0;
                  display_all();
                }
                else if (g.paused == 2)
                  // Restarting from a pause which happened during the initial travel to the starting point
                {
                  go_to(g.reg.point[g.point1], SPEED_LIMIT);
                  g.stacker_mode = 1;
                  g.start_stacking = 0;
                  g.paused = 0;
                  display_all();
                }
                break;
              }
              if (g.reg.i_mode == CONT_MODE && g.paused == 0)
              {
                // Initiating a new stack (or timelapse sequence of stacks)
                // Using the simplest approach which will result the last shot to always slightly undershoot
                g.Nframes = Nframes();
                g.starting_point = g.reg.point[g.point1];
                g.destination_point = g.reg.point[g.point2];
                go_to(g.starting_point, SPEED_LIMIT);
                g.stacker_mode = 1;
                g.continuous_mode = 1;
                g.start_stacking = 0;
                g.timelapse_counter = 0;
                if (g.reg.n_timelapse > 1)
                  g.timelapse_mode = 1;
                display_comment_line("   2-points stack   ");
              }
              else if (g.reg.i_mode == NONCONT_MODE && g.paused == 0)
              {
                // Using the simplest approach which will result the last shot to always slightly undershoot
                g.Nframes = Nframes();
                // Always starting from the foreground point, for full backlash compensation:
                go_to(g.reg.point[g.point1], SPEED_LIMIT);
                g.starting_point = g.reg.point[g.point1];
                g.destination_point = g.reg.point[g.point2];
                g.stacker_mode = 1;
                // This is a non-continuous mode:
                g.continuous_mode = 0;
                g.start_stacking = 0;
                g.timelapse_counter = 0;
                if (g.reg.n_timelapse > 1)
                  g.timelapse_mode = 1;
                display_comment_line("   2-points stack   ");
              }
              else
              {
                // 1-point stacking
                // The flag means we just initiated stacking:
                g.start_stacking = 1;
                // Time when stacking was initiated:
                g.t0_stacking = g.t;
                g.frame_counter = 0;
                display_frame_counter();
                g.ipos_to_shoot = g.ipos;
                g.starting_point = g.ipos;
                g.stacker_mode = 3;
                g.continuous_mode = 1;
                display_comment_line("   1-point stack    ");
              }
              break;


            case '*':  //@ * : Show alternative display (for *X commands)
              if (g.editing == 1)
                break;
              if (g.paused)
                break;
              if (!g.moving && g.alt_flag == 0)
              {
                g.alt_flag = 1;
                // Kind=1 means "*" screen
                g.alt_kind = 1;
                display_all();
              }
              break;

            case '0': //@ 0: Cycling through different modes: 1-shot, 2-shot continuous, 2-shot noncontinuous
              if (g.editing == 1)
              {
                editor(key0);
                break;
              }
              if (g.paused || g.moving)
                break;
              g.reg.i_mode++;
              if (g.reg.i_mode > 2)
                g.reg.i_mode = 0;
              display_all();
              EEPROM.put( g.addr_reg[0], g.reg);
              break;

            case '5':  //@ 5: Set the step (microsteps per frame)
              if (g.editing == 1)
              {
                editor(key0);
                break;
              }
              else
              {
                if (g.paused)
                  break;
                g.editing = 1;
                g.edited_param = PARAM_MSTEP;
                editor('I');
                break;
              }
              // Also used for different debugging modes, to decrease debugged parameters
              break;

            case '6':  //@ 6: Help menu
              if (g.editing == 1)
              {
                editor(key0);
                break;
              }
              // Also used for different debugging modes, to increase debugged parameters
              g.help_mode = 1;
              help();
              break;

            case '2':  //@ 2: GoTo command (for both non-paused and paused situations)
              if (g.editing == 1)
              {
                editor(key0);
                break;
              }
              else
              {
                g.editing = 1;
                g.edited_param = PARAM_GOTO;
                editor('I');
                break;
              }
              break;

            case '3':  //@ 3: Parking
              if (g.editing == 1)
              {
                editor(key0);
                break;
              }
              if (g.paused)
                break;
              parking();
              break;

            case '8':  //@ 8: FPS or First delay
              if (g.editing == 1)
              {
                editor(key0);
                break;
              }
              else
              {
                if (g.paused)
                  break;
                g.editing = 1;
                if (g.reg.i_mode == ONE_SHOT_MODE)
                  g.edited_param = PARAM_FPS;
                else if (g.reg.i_mode == CONT_MODE)
                  g.edited_param = PARAM_FPS;
                else if (g.reg.i_mode == NONCONT_MODE)
                  g.edited_param = PARAM_FIRST_DELAY;
                else
                  break;
                editor('I');
                break;
              }
              if (g.paused)
                break;
              break;

            case '9': //@ 9: Nshots or Second delay
              if (g.editing == 1)
              {
                editor(key0);
                break;
              }
              else
              {
                if (g.paused)
                  break;
                // Key is not used in the 2p cont mode:
                if (g.reg.i_mode == CONT_MODE)
                  break;
                g.editing = 1;
                if (g.reg.i_mode == ONE_SHOT_MODE)
                  g.edited_param = PARAM_N_SHOTS;
                else
                  g.edited_param = PARAM_SECOND_DELAY;
                editor('I');
                break;
              }
              break;

            case '#': //@ # : Dot (in editing mode)
              if (g.editing == 1)
              {
                editor(key0);
              }
              break;


          } // End of case
        }  // if (g.stacker_mode == 0)
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End of processing specific key presses

        else if (g.stacker_mode > 0)
          // Mode 1/2: focus stacking
        {
          // Any key press in stacking mode interrupts stacking
          if (g.moving)
          {
            g.model_type = MODEL_STOP;
            g.model_init = 1;
          }
          // All situations when we pause: during 2-point stacking, while travelling to the starting point, and while waiting between stacks in timelaspe mode:
          if (g.stacker_mode == 2 || g.stacker_mode == 1 || g.stacker_mode == 4)
          {
            // Paused during 2-point stacking
            if (g.stacker_mode == 2)
            {
              g.paused = 1;
              g.delayed_goto = 1; // A signal that once the breaking is finished, a goto move to the previous frame will be initiated in camera()
            }
            // Paused while travelling to the starting point
            else if (g.stacker_mode == 1)
            {
              g.paused = 2;
            }
            // Paused while waiting between stacks in timelaspe mode
            else
            {
              g.paused = 3;
            }
            display_comment_line("       Paused       ");
            letter_status("P");
          }
          else
            // In 1-point stacking, we abort
          {
            if (g.reg.mirror_lock == MIRROR_BURST)
            // In the burst mode, shuttre_on and AF_on do not reflect the real state anymore, so we have to manually enable them now
            {
              g.shutter_on = 1;
              g.AF_on = 1;
            }
            g.frame_counter = 0;
            display_comment_line("   Stacking abort   ");
          }
          g.stacker_mode = 0;
        }

      }  // if (PRESSED)

      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      else
        // if a key was just released
      {
        g.init_delayed_key = 0;
        // Resetting the counter of key repeats:
        g.N_repeats = 0;
        // Breaking / stopping if 1/A keys were depressed
        if ((g.key_old == '1' || g.key_old == 'A') && g.moving == 1 && state0 == RELEASED && state0_changed && g.paused == 0 &&
            (g.model_type == MODEL_REWIND || g.model_type == MODEL_FF))
        {
          g.model_type = MODEL_STOP;
          g.model_init = 1;
        }
        if (g.key_old == '*' && g.alt_flag != 0)
          // The '*' key was just released: switch to default screen from the alternative one
        {
          g.alt_flag = 0;
          display_all();
        }
      }

    }  // End of if(keyStateChanged)
  } // End of two-key / one-key if

  return;
}


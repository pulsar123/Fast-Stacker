float Msteps_per_frame ()
{
  return (MM_PER_FRAME[g.i_mm_per_frame] / MM_PER_ROTATION) * MICROSTEPS_PER_ROTATION;
}


short Nframes ()
{
  return short(((float)(g.point2 - g.point1)) / g.msteps_per_frame) + 1;
}


void process_keypad()
/*
 All the keypad runtime stuff goes here
 */
{
  float speed;

  // Disabling the last comment line displaying after COMMENT_DELAY interval:
  if (g.comment_flag == 1 && g.t > g.t_comment + COMMENT_DELAY)
  {
    g.comment_flag == 0;
    //    display_current_position();
  }


  // ?? Ignore keypad during emergency breaking
  if (g.breaking == 1 || (g.calibrate == 3 && g.calibrate_warning == 0))
    return;

  // Stuff to do at every call to keypad()
  if (g.stacker_mode == 1 && g.moving == 0)
  {
    // Estimating the required speed in microsteps per microsecond
    speed = SPEED_SCALE * FPS[g.i_fps] * MM_PER_FRAME[g.i_mm_per_frame];
    go_to(g.destination_point, speed);
    g.frame_counter = 0;
    g.pos_to_shoot = g.pos_short_old;
    g.stacker_mode = 2;
#ifdef DEBUG
    /*
            Serial.print("STACKER2: g.destination_point=");
            Serial.print(g.destination_point);
            Serial.print(", g.pos_to_shoot=");
            Serial.print(g.pos_to_shoot);
            Serial.print(", g.stacking_direction=");
            Serial.print(g.stacking_direction);
            Serial.print(", floorMy(g.pos-0.5*g.stacking_direction)=");
            Serial.print(floorMy(g.pos-0.5*g.stacking_direction));
            Serial.print(", g.shutter_on=");
            Serial.println(g.shutter_on);
            */
#endif
  }


  // Reading a keypad key if any:
  char key = keypad.getKey();
  KeyState state = keypad.getState();

  // Action is only needed if the kepad state changed since the last time,
  // or if one of the parameter change keys was pressed longer than T_KEY_LAG microseconds:
  if (state != g.state_old && (state == PRESSED || state == RELEASED) ||
      state == PRESSED && g.state_old == PRESSED && (key == '2' || key == '3' || key == '5'
          || key == '6' || key == '8' || key == '9') && g.t - g.t_key_pressed > T_KEY_LAG)
  {
#ifdef DEBUG
    Serial.print(state);
    Serial.print(", ");
    Serial.println(key);
#endif

    if (state == PRESSED)
    {
      if (g.state_old != PRESSED)
      {
        // Memorizing the time when a key was pressed:
        g.t_key_pressed = g.t;
        if (g.calibrate_warning == 1)
          // Any key pressed when calibrate_warning=1 will initiate calibration:
        {
          g.calibrate_warning = 0;
          display_all(" ");
          return;
        }
      }

      else
        // We are here when a change parameter key was pressed longer than T_KEY_LAG
      {
        if (g.N_repeats == 0)
          // Will be used for keys with repetition (parameter change keys):
          g.t_last_repeat = g.t;
        // We repeat a paramet change key once the time since the last repeat is larger than T_KEY_REPEat:
        if (g.t - g.t_last_repeat > T_KEY_REPEAT)
        {
          g.N_repeats++;
          g.t_last_repeat = g.t;
        }
        else
          // Too early for a repeated key, so returning now:
          return;
      }

      // Keys interpretation depends on the stacker_mode:
      if (g.stacker_mode == 0)
        // Mode 0: default; rewinding etc.
      {
        // When error 1 (limiter on initially), the only commands accepted are reqind and fast forward:
        if (g.error == 1 && key != '1' && key != 'A')
          return;

        switch (key)
        {
          case '1':  // Rewinding
            g.direction = -1;
            motion_status();
            change_speed(-SPEED_LIMIT, 0);
            break;

          case 'A':  // Fast forwarding
            g.direction = 1;
            motion_status();
            change_speed(SPEED_LIMIT, (short)0);
#ifdef DEBUG
            Serial.print(" g.moving=");
            Serial.println(g.moving);
            Serial.print(" speed1=");
            Serial.println(g.speed1, 6);
#endif

            break;

          case '4':  // Set foreground point
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

          case 'B':  // Set background point
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

          case '7':  // Go to the foreground point
            go_to(g.point1, SPEED_LIMIT);
            display_comment_line(" Going to P1  ");
            break;

          case 'C':  // Go to the background point
            go_to(g.point2, SPEED_LIMIT);
            display_comment_line(" Going to P2  ");
            break;

          case '0': // Start shooting (2-point focus stacking)
            // Checking the correctness of point1/2
            if (g.point2 > g.point1 && g.point1 >= g.limit1 && g.point2 <= g.limit2)
            {
              // Required microsteps per frame:
              g.msteps_per_frame = Msteps_per_frame();
              // Using the simplest approach which will result the last shot to always slightly undershoot
              g.Nframes = Nframes();
              // Finding the closest point:
              short d1 = (short)fabs(g.pos_short_old - g.point1);
              short d2 = (short)fabs(g.pos_short_old - g.point2);
              if (d1 < d2)
              {
                go_to(g.point1, SPEED_LIMIT);
                g.starting_point = g.point1;
                // The additional microstep is needed because camera() shutter is lagging by 1/2 microstep by design:
                g.destination_point = g.point2;
                g.stacking_direction = 1;
              }
              else
              {
                go_to(g.point2, SPEED_LIMIT);
                g.starting_point = g.point2;
                // The additional microstep is needed because camera() shutter is lagging by 1/2 microstep by design:
                g.destination_point = g.point1;
                g.stacking_direction = -1;
              }
              g.stacker_mode = 1;
              display_comment_line("2-points stack");
            }
            else
            {
              // Should print error message
              display_comment_line("Bad 2 points! ");
            }
            break;

          case '*':  // Initiate one-point focus stacking backwards
            // Simplest workaround: ignore the command if currently moving
            // (better solution would be to stop first)
            if (!g.moving)
            {
              // Required microsteps per frame:
              g.msteps_per_frame = Msteps_per_frame();
              // Estimating the required speed in microsteps per microsecond
              speed = SPEED_SCALE * FPS[g.i_fps] * MM_PER_FRAME[g.i_mm_per_frame];
              go_to(g.limit1, speed);
#ifdef DEBUG
              Serial.print("msteps_per_frame=");
              Serial.println(g.msteps_per_frame);
              Serial.print("speed=");
              Serial.println(speed, 6);
              Serial.print("g.limit1=");
              Serial.println(g.limit1);
              Serial.print("g.limit2=");
              Serial.println(g.limit2);
#endif
              g.frame_counter = 0;
              g.pos_to_shoot = g.pos_short_old;
              g.starting_point = g.pos_short_old;
              g.stacking_direction = -1;
              g.stacker_mode = 3;
              display_comment_line("1-point stack ");
            }
            break;

          case 'D':  // Initiate one-point focus stacking forward
            if (!g.moving)
            {
              // Required microsteps per frame:
              g.msteps_per_frame = Msteps_per_frame();
              // Estimating the required speed in microsteps per microsecond
              speed = SPEED_SCALE * FPS[g.i_fps] * MM_PER_FRAME[g.i_mm_per_frame];
              go_to(g.limit2, speed);
              g.frame_counter = 0;
              g.pos_to_shoot = g.pos_short_old;
              g.starting_point = g.pos_short_old;
              g.stacking_direction = 1;
              g.stacker_mode = 3;
              display_comment_line("1-point stack ");
            }
            break;

          case '2':  // Decrease parameter n_shots
            if (g.i_n_shots > 0)
              g.i_n_shots--;
            else
              g.i_n_shots = 0;
            EEPROM.put( ADDR_I_N_SHOTS, g.i_n_shots);
            //!!!
            //            display_one_point_params();
            display_all(" ");
            break;

          case '3':  // Increase parameter n_shots
            if (g.i_n_shots < N_PARAMS - 1)
              g.i_n_shots++;
            else
              g.i_n_shots = N_PARAMS - 1;
            EEPROM.put( ADDR_I_N_SHOTS, g.i_n_shots);
            display_one_point_params();
            break;

          case '5':  // Decrease parameter mm_per_frame
            if (g.i_mm_per_frame > 0)
              g.i_mm_per_frame--;
            else
              g.i_mm_per_frame = 0;
            // Required microsteps per frame:
            g.msteps_per_frame = Msteps_per_frame();
            // Using instead the simplest approach, which will result the last shot to always slightly undershoot
            g.Nframes = Nframes();
            display_all(" ");
            EEPROM.put( ADDR_I_MM_PER_FRAME, g.i_mm_per_frame);
            break;

          case '6':  // Increase parameter mm_per_frame
            if (g.i_mm_per_frame < N_PARAMS - 1)
              g.i_mm_per_frame++;
            else
              g.i_mm_per_frame = N_PARAMS - 1;
            // Required microsteps per frame:
            g.msteps_per_frame = Msteps_per_frame();
            // Using instead the simplest approach, which will result the last shot to always slightly undershoot
            g.Nframes = Nframes();
            EEPROM.put( ADDR_I_MM_PER_FRAME, g.i_mm_per_frame);
            display_all(" ");
            break;

          case '8':  // Decrease parameter fps
            if (g.i_fps > 0)
              g.i_fps--;
            else
              g.i_fps = 0;
            EEPROM.put( ADDR_I_FPS, g.i_fps);
            display_all(" ");
            break;

          case '9':  // Increase parameter fps
            if (g.i_fps < N_PARAMS - 1)
              g.i_fps++;
            else
              g.i_fps = N_PARAMS - 1;
            EEPROM.put( ADDR_I_FPS, g.i_fps);
            display_all(" ");
            break;

        } // End of case
      }
      else if (g.stacker_mode > 0)
        // Mode 1/2: focus stacking
      {
        // Any key press in stacking mode interrupts stacking
        g.stacker_mode = 0;
        change_speed(0.0, 0);
        display_comment_line("Stacking abort");
      }

      g.key_old = key;
    }  // if (PRESSED)
    else
      // if a key was just released
    {
      // Resetting the counter of key repeats:
      g.N_repeats = 0;
      // Breaking / stopping if no keys pressed (only after rewind/fastforward)
      if ((g.key_old == '1' || g.key_old == 'A') && g.moving == 1 && state == RELEASED)
      {
        change_speed(0.0, 0);
#ifdef DEBUG
        Serial.print(" g.moving=");
        Serial.println(g.moving);
#endif
      }
    }

    g.state_old = state;
  }  // End of if(keyStateChanged)


  return;
}


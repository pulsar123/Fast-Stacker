void initialize(byte factory_reset)
/* Initializing all the variables, with the optional factory reset (resetting all the EEPROM data).
*/
{
  unsigned char limit_on;
  int address;

  g.error = 0;
  for (byte i = 0; i < 4; i++)
  {
    g.delta_pos[i] = 0;
  }
  g.delta_pos_curr = 0;

  if (g.telescope)
    // Providing constant +5V to the temperature probe on telescope:
    digitalWrite(PIN_SHUTTER, HIGH);
  else
  {
#ifndef DISABLE_SHUTTER
    digitalWrite(PIN_SHUTTER, LOW);
#endif
    digitalWrite(PIN_AF, LOW);
  }

  // Keypad stuff:
  // No locking for keys:
  keypad.setHoldTime(65000);
  keypad.setDebounceTime(50);
  g.key_old = '=';

#ifndef MOTOR_DEBUG
  // Limiting switches should not be on when powering up:
  limit_on = digitalRead(PIN_LIMITERS);
  if (limit_on == HIGH)
  {
    g.error = 1;
    // If cable is disconnected, by default using macro rail mode:
    g.telescope = 0;
  }
#endif

  if (g.telescope)
  {
    // Initially not displaying register #:
    g.displayed_register = 0;
    g.accel_limit = ACCEL_LIMIT_TEL;
    g.mm_per_microstep = MM_PER_MICROSTEP_TEL;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TELESCOPE");
    delay(500);
    // Setting the pointer to the telescope memory registers in EEPROM:
    address = ADDR_REG1_TEL;
  }
  else
  {
    g.accel_limit = ACCEL_LIMIT;
    g.mm_per_microstep = MM_PER_MICROSTEP;
    address = ADDR_REG1;
  }
  // EEPROM addresses for memory registers (different for macro and telescope modes), including the 0th (default) register:
  for (unsigned char jj = 0; jj <= N_REGS; jj++)
  {
    g.addr_reg[jj] = address + jj * SIZE_REG;
  }

  // Initializing program parameters:
  g.moving = 0;
  g.speed1 = 0.0;
  g.accel = 0;
  g.speed0 = 0.0;
  g.speed = 0.0;
  g.pos_stop_flag = 0;
  g.stacker_mode = 0;
  g.shutter_on = 0;
  g.AF_on = 0;
  g.single_shot = 0;
  g.direction = 1;
  g.comment_flag = 0;
  g.status_flag = 0;
  g.current_point = -1;

  if (factory_reset)
  {
    clear_calibrate_state();
    // Parameters for the reg structure:
    g.reg.i_n_shots = 9;
    g.reg.i_fps = 16;
    g.reg.i_first_delay = 4;
    g.reg.i_second_delay = 3;
    g.reg.i_accel_factor = 1;
    g.reg.i_n_timelapse = 0;
    g.reg.i_dt_timelapse = 5;
    g.reg.mirror_lock = 1;
    g.reg.backlash_on = 1;
    update_backlash();
    g.reg.straight = 1;
    g.reg.save_energy = 1;
    update_save_energy();
    for (byte i = 0; i < 4; i++)
    {
      g.reg.point[i] = 2000;
    }
    g.limit1 = 1000;
    g.limit2 = 32000;
    g.pos = 2000;
    g.backlight = 0;

    if (g.telescope)
    {
      byte i;
      // Disabling calibration when operating telescope
      g.reg.i_mm_per_frame = 0;
      for (i = 0; i < 4; i++)
        g.reg.raw_T[i] = 512;
      // Initially registers are no locked:
      for (i = 0; i < N_REGS; i++)
      {
        g.locked[i] = 0;
        EEPROM.put( ADDR_LOCK + i, g.locked[i] );
      }
    }
    else
    {
#ifndef MOTOR_DEBUG
      g.calibrate = 3;
#endif
      g.reg.i_mm_per_frame = 5;
      EEPROM.put( ADDR_CALIBRATE, g.calibrate );
      EEPROM.put( ADDR_LIMIT1, g.limit1);
      EEPROM.put( ADDR_LIMIT2, g.limit2);
      EEPROM.put( ADDR_POS, g.pos );
    }
    EEPROM.put( ADDR_BACKLIGHT, g.backlight);

    // Initializing all EEPROM registers (including the default one):
    for (unsigned char jj = 0; jj <= N_REGS; jj++)
    {
      EEPROM.put(g.addr_reg[jj], g.reg);
    }
  }

  // Regular initialization (not a factory reset):
  else
  {
    // Reading the values from EEPROM:
    if (g.telescope)
    {
      for (byte i = 0; i < N_REGS; i++)
      {
        EEPROM.get( ADDR_LOCK + i, g.locked[i] );
      }
    }
    else
    {
      EEPROM.get( ADDR_POS, g.pos );
      EEPROM.get( ADDR_CALIBRATE, g.calibrate );
      EEPROM.get( ADDR_LIMIT1, g.limit1);
      EEPROM.get( ADDR_LIMIT2, g.limit2);
    }
    EEPROM.get( ADDR_BACKLIGHT, g.backlight);
    // Reading the default memory register:
    EEPROM.get(g.addr_reg[0], g.reg);
    update_backlash();
    update_save_energy();
  }  // if factory_reset

#ifdef TEMPERATURE
  if (g.telescope)
  {
    for (byte i = 0; i < 4; i++)
      g.Temp0[i] = compute_temperature(g.reg.raw_T[i]);
    // Measuring current temperature and updating delta_pos[] values:
    measure_temperature();
  }
#endif

  // Five possible floating point values for acceleration
  set_accel_v();

  set_backlight();

  g.calibrate_flag = 0;
  if (g.calibrate == 3)
    g.error = 4;
  // Memorizing the initial value of g.calibrate:
  g.calibrate_init = g.calibrate;
  g.pos0 = g.pos;
  g.pos_old = g.pos;
  g.pos_short_old = floorMy(g.pos);
  g.t0 = micros();
  g.t = g.t0;
  g.t_old = g.t0;
  g.t_key_pressed = g.t0;
  g.t_last_repeat = g.t0;
  g.t_display = g.t0;
  g.t_shutter = g.t0;
  g.t_shutter_off = g.t0;
  g.t_AF = g.t0;
  g.t_status = g.t0;
  g.t_mil = millis();

  g.N_repeats = 0;
  g.breaking = 0;
  g.backlashing = 0;
  g.pos_stop_flag = 0;
  g.frame_counter = 0;
  g.coords_change = 0;
  g.start_stacking = 0;
  g.make_shot = 0;
  g.paused = 0;
  g.starting_point = g.reg.point[0];
  g.timelapse_counter = 0;
  g.timelapse_mode = 0;

  if (factory_reset || g.telescope)
    // Not doing this in telescope mode (so we can hand-calibrate the coordinates by manually putting the focuser at the 0 position initially)
  {
    g.BL_counter = 0;
    g.backlash_init = 0;
  }
  else
  {
    // As we cannot be sure about the initial state of the rail, we are assuming the worst: a need for the maximum backlash compensation:
    g.BL_counter = g.backlash;
    g.backlash_init = 1;
  }
  g.started_moving = 0;
  g.dt_backlash = 0;
  g.continuous_mode = 1;
  g.noncont_flag = 0;
  g.alt_flag = 0;
  g.alt_kind = 1;
  g.disable_limiters = 0;
#ifdef EXTENDED_REWIND
  g.no_extended_rewind = 0;
#endif

  //  g.msteps_per_frame = MSTEP_PER_FRAME[g.reg.i_mm_per_frame];
  g.Nframes = Nframes();

  // Default lcd layout:
  // This sets g.speed_limit, among other things:
  display_all();

#ifdef TIMING
  if (g.moving == 0)
  {
    g.i_timing = (unsigned long)0;
    g.dt_max = (short)0;
    g.dt_min = (short)10000;
    g.bad_timing_counter = (short)0;
  }
#endif

#ifdef MOTOR_DEBUG
  clear_calibrate_state();
#endif
  if (g.telescope)
    // Disabling calibration when operating telescope
  {
    clear_calibrate_state();
    // No rail reverse in telescope mode:
    g.reg.straight = 1;
    g.pos = 0.0;
    g.pos_short_old = 0;
    g.pos0 = 0.0;
    // Setting two soft limits assuming that initilly the focuser is at its closest position;
    g.limit1 = (COORD_TYPE)TEL_INIT - (COORD_TYPE)BACKLASH_TEL - 100;
    // the second limit is equal to the TEL_LENGTH parameter:
    g.limit2 = (COORD_TYPE)TEL_LENGTH;
    go_to((COORD_TYPE)TEL_INIT, g.speed_limit);
  }

#ifdef CAMERA_DEBUG
  shutter_status(0);
  AF_status(0);
  g.reg.i_first_delay = 4;
  g.reg.i_second_delay = 3;
#endif

#ifdef TEST_SWITCH
  clear_calibrate_state();
  //  g.error = 0;
  g.test_flag = 0;
  g.reg.backlash_on = 0;
  update_backlash();
  g.speed_test = g.speed_limit;
  g.test_sum = 0.0;
  g.test_sum2 = 0.0;
  g.test_N = 0;
  g.delta_min = 1e6;
  g.delta_max = -1e6;
  g.test_dev = 0.0;
#endif

  return;
}


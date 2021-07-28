void initialize(byte factory_reset)
/* Initializing all the variables, with the optional factory reset (resetting all the EEPROM data).
*/
{
  int address;

  for (byte i = 0; i < 4; i++)
  {
    g.delta_pos[i] = 0;
  }
  g.delta_pos_curr = 0;

  if (g.telescope)
    // Providing constant +5V to the temperature probe on telescope:
    iochip.digitalWrite(EPIN_SHUTTER, HIGH);
  else
  {
#ifndef DISABLE_SHUTTER
    iochip.digitalWrite(EPIN_SHUTTER, LOW);
#endif
    iochip.digitalWrite(EPIN_AF, LOW);
  }
#if defined(TEST_SWITCH) || defined(TEST_HALL)
  // Providing +5V for Hall sensor:
  iochip.digitalWrite(EPIN_SHUTTER, HIGH);
  delay(10);
#endif

  // Assigning the limiters' state to g.limit_on:
  Read_limiters();

#ifdef TEST_LIMITER
  g.limiter_i = 0;
//  g.limiter_ini = g.limit_on;
  g.limiter_ini = 0;
#endif  

  // Keypad stuff:
  // No locking for keys:
  keypad.setHoldTime(65000);
  keypad.setDebounceTime(50);
  g.key_old = '=';

  if (g.telescope)
  {
    /* !!!
    g.speed_limit = SPEED_LIMIT_TEL;
    g.n_regs = N_REGS_TEL;
    g.accel_limit = ACCEL_LIMIT_TEL;
    g.mm_per_microstep = MM_PER_MICROSTEP_TEL;
    tft.fillScreen(TFT_BLACK);
    my_setCursor(0, 0);
    tft.print("TELESCOPE");
    delay(500);
    // Setting the pointer to the telescope memory registers in EEPROM:
    address = ADDR_REG1_TEL;
    */
  }
  else
  {
    g.speed_limit = SPEED_LIMIT;
    g.n_regs = N_REGS;
    g.ireg = 0;
    g.accel_limit = ACCEL_LIMIT;
    g.mm_per_microstep = MM_PER_MICROSTEP;
    address = ADDR_REG1;
  }
  // EEPROM addresses for memory registers (different for macro and telescope modes), including the 0th (default) register:
  for (unsigned char jj = 0; jj <= g.n_regs; jj++)
  {
    g.addr_reg[jj] = address + jj * SIZE_REG;
  }

  // Initializing program parameters:
  g.moving = 0;
  g.stacker_mode = 0;
  g.shutter_on = 0;
  g.AF_on = 0;
  g.single_shot = 0;
  g.direction = 1;
  g.dir = 1;
  g.comment_flag = 0;
  g.status_flag = 0;
  g.current_point = -1;
  g.limit1 = 0;
  g.accident = 0;
  g.limiter_counter = 0;

  if (factory_reset)
  {
    g.calibrate_flag = 1;
    g.error = 4;
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
      g.reg.point[i] = DELTA_LIMITER;
    }
    g.limit2 = 32000;
    g.ipos = DELTA_LIMITER;
    g.backlight = 0;

    if (g.telescope)
    {
      byte i;
      // Disabling calibration when operating telescope
      g.reg.i_mm_per_frame = 0;
      for (i = 0; i < 4; i++)
        g.reg.raw_T[i] = 512;
      // Initially registers are no locked:
      for (i = 0; i < g.n_regs; i++)
      {
        g.locked[i] = 0;
        EEPROM.put( ADDR_LOCK + i, g.locked[i] );
      }
      g.ireg = 1;
      EEPROM.put( ADDR_IREG, g.ireg );
    }
    else
    {
      g.reg.i_mm_per_frame = 5;
      EEPROM.put( ADDR_LIMIT2, g.limit2);
      EEPROM.put( ADDR_POS, g.ipos );
    }
    EEPROM.put( ADDR_BACKLIGHT, g.backlight);

    // Initializing all EEPROM registers (including the default one):
    for (byte jj = 0; jj <= g.n_regs; jj++)
    {
      EEPROM.put(g.addr_reg[jj], g.reg);
    }
  }

  // Regular initialization (not a factory reset):
  else
  {
    g.calibrate_flag = 0;
    g.error = 0;
    // Reading the values from EEPROM:
    if (g.telescope)
    {
      for (byte i = 0; i < g.n_regs; i++)
      {
        EEPROM.get( ADDR_LOCK + i, g.locked[i] );
      }
      EEPROM.get( ADDR_IREG, g.ireg );
    }
    else
    {
      EEPROM.get( ADDR_POS, g.ipos );
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

  g.model_ipos0 = g.ipos;
  g.t = micros();
  g.t_key_pressed = g.t;
  g.t_last_repeat = g.t;
  g.t_display = g.t;
  g.t_shutter = g.t;
  g.t_shutter_off = g.t;
  g.t_AF = g.t;
  g.t_status = g.t;
  g.t_mil = millis();

  g.N_repeats = 0;
  g.uninterrupted = 0;
  g.uninterrupted2 = 0;
  g.backlashing = 0;
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
  g.model_init = 0;
  g.dt_lost = 0;
  g.continuous_mode = 1;
  g.noncont_flag = 0;
  g.alt_flag = 0;
  g.alt_kind = 1;

  g.Nframes = Nframes();

  // Default lcd layout:
  // This sets g.speed_limit, among other things:
  display_all();

#ifdef TIMING
  if (g.moving == 0)
  {
    g.i_timing = (unsigned long)0;
    g.dt_max = 0;
    g.dt_min = 10000000;
    g.bad_timing_counter = 0;
    g.total_dt_timing = 0;
    g.moving_old = 0;
    g.dt_timing = 0;
  }
    g.d_sum += 0.0;
    g.d_N = 0;
    g.d_Nbad = 0;
    g.d_max = 0;
    g.N_insanity = 0;
#endif

#ifdef MOTOR_DEBUG
  g.calibrate_flag = 0;
#endif

  if (g.telescope)
  {
    // No rail reverse in telescope mode:
    g.reg.straight = 0;
    g.ipos = 0;
    g.limit2 = (COORD_TYPE)TEL_LENGTH;
#ifdef TELE_SWITCH
    // Initiating telescope calibration:
    g.calibrate_flag = 10;
#else // TELE_SWITCH
    g.calibrate_flag = 0;
    g.error = 0;
    // Moving focuser into safe area:
    go_to(TEL_INIT, g.speed_limit);
#endif // TELE_SWITCH
  }

#ifdef CAMERA_DEBUG
  shutter_status(0);
  AF_status(0);
  g.reg.i_first_delay = 4;
  g.reg.i_second_delay = 3;
#endif

#ifdef TEST_SWITCH
  g.calibrate_flag = 11;
  g.test_flag = 0;
  g.reg.backlash_on = 0;
  update_backlash();
  g.test_N = 0;
  for (byte i = 0; i < 2; i++)
  {
    g.test_limit_on[i] = 0;
    g.test_sum[i] = 0.0;
    g.test_sum2[i] = 0.0;
    g.delta_min[i] = 1e6;
    g.delta_max[i] = -1e6;
    g.test_dev[i] = 0.0;
    g.test_avr[i] = 0.0;
    g.test_std[i] = 0.0;
    g.count[i] = 0;
  }
  g.reg.straight = 1;
  // This will help to park the rail properly (at the next full step position) at the end:
  g.pos0_test = g.ipos;
#endif

#ifdef TEST_HALL
  tft.fillScreen(TFT_BLACK);
  g.backlight = 0;
  set_backlight();
#endif

#ifdef BUZZER
  g.dt1_buzz_us = DT_BUZZ_US;
#endif

  sprintf(g.empty_buffer, "                    ");  // 20 spaces, used to clear one LCD row
  
EEPROM.commit();
  
  return;
}


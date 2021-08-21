void initialize(byte factory_reset)
/* Initializing all the variables, with the optional factory reset (resetting all the EEPROM data).
*/
{
  int address;

#ifndef DISABLE_SHUTTER
  iochip.digitalWrite(EPIN_SHUTTER, LOW);
#endif
  iochip.digitalWrite(EPIN_AF, LOW);

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

  address = ADDR_REG1;

  // EEPROM addresses for memory registers, including the 0th (default) register:
  for (unsigned char jj = 0; jj <= N_REGS; jj++)
  {
    g.addr_reg[jj] = address + jj * SIZE_REG;
  }

  // Initializing program parameters:
  g.moving = 0;
  g.model_init = 0;
  g.model_type = MODEL_NONE;
  g.motion_status_code = STATUS_NONE;
  g.dt_lost = 0;
  g.stacker_mode = 0;
  g.shutter_on = 0;
  g.AF_on = 0;
  g.single_shot = 0;
  g.direction = 1;
  g.dir = 0; // 0 so it's guaranteed to execute the proper direction command at first motor_direction() call
  g.dir_raw = 1;
  g.comment_flag = 0;
  g.current_point = -1;
  g.limit1 = 0;
  g.accident = 0;
  g.delayed_goto = 0;
  g.editing = 0; 
  g.ipos_raw = 0;
//  g.limiter_counter = 0;

  if (factory_reset)
  {
    g.calibrate_flag = 1;
    g.error = 4;
    // Parameters for the reg structure:
    g.reg.i_mode = ONE_SHOT_MODE;
    g.reg.n_shots = 10;
    g.reg.fps = 1.0;
    g.reg.first_delay = 1.0;
    g.reg.second_delay = 1.0;
    g.reg.i_accel_factor = 1;
    g.reg.n_timelapse = 1;
    g.reg.dt_timelapse = 300.0;
    g.reg.mirror_lock = 1;
    g.reg.backlash_on = 0;
    update_backlash();
    g.reg.straight = 1;
    g.reg.save_energy = 1;
    update_save_energy();
    for (byte i = 0; i < 2; i++)
    {
      g.reg.point[i] = DELTA_LIMITER;
    }
    g.limit2 = HUGE;
    g.ipos = DELTA_LIMITER;

    g.reg.mstep = 10;
    EEPROM.put( ADDR_LIMIT2, g.limit2);
    EEPROM.put( ADDR_POS, g.ipos );

    // Initializing all EEPROM registers (including the default one):
    for (byte jj = 0; jj <= N_REGS; jj++)
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
    EEPROM.get( ADDR_POS, g.ipos );
    EEPROM.get( ADDR_LIMIT2, g.limit2);
    // Reading the default memory register:
    EEPROM.get(g.addr_reg[0], g.reg);
    update_backlash();
    update_save_energy();
  }  // if factory_reset

  // Five possible floating point values for acceleration
  set_accel_v();

  g.model_ipos0 = g.ipos;
  g.t = micros();
  g.t_key_pressed = g.t;
  g.t_last_repeat = g.t;
  g.t_display = g.t;
  g.t_shutter = g.t;
  g.t_shutter_off = g.t;
  g.t_AF = g.t;
  g.t_mil = millis();
  g.t_next_step = g.t;
  g.t_key_delay = g.t;

  g.N_repeats = 0;
  g.uninterrupted = 0;
  g.uninterrupted2 = 0;
  g.Backlashing = 0;
  g.frame_counter = 0;
  g.coords_change = 0;
  g.start_stacking = 0;
  g.make_shot = 0;
  g.paused = 0;
  g.starting_point = g.reg.point[g.point1];
  g.timelapse_counter = 0;
  g.timelapse_mode = 0;
  g.key_delay_on = 0;

  if (factory_reset)
  {
    g.BL_counter = 0;
    g.Backlash_init = 0;
  }
  else
  {
    // As we cannot be sure about the initial state of the rail, we are assuming the worst: a need for the maximum backlash compensation:
    g.BL_counter = g.backlash;  // Can be + or -
    g.Backlash_init = 1;
  }
  
  g.continuous_mode = 1;
  g.noncont_flag = 0;
  g.alt_flag = 0;
  g.alt_kind = 1;

  g.Nframes = Nframes();

  // Default lcd layout:
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

#ifdef CAMERA_DEBUG
  shutter_status(0);
  AF_status(0);
  g.reg.first_delay = 1.0;
  g.reg.second_delay = 1.0;
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

#ifdef BUZZER
  g.buzz_state = LOW;
  iochip.digitalWrite(EPIN_BUZZ, g.buzz_state);
  g.dt1_buzz_us = DT_BUZZ_US;
  g.t_beep = g.t;
  g.t_buzz = g.t;
  g.beep_on = 0;
  g.beep_length = 100000;
#endif

  sprintf(g.empty_buffer, "                    ");  // 20 spaces, used to clear one LCD row

  motor_direction(); // Sending the motor the initial direction signal

  EEPROM.commit();

  return;
}


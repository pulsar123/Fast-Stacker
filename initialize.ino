void initialize(byte factory_reset)
/* Initializing all the variables, with the optional factory reset (resetting all the EEPROM data).
*/
{
  g.error = 0;
  g.calibrate_warning = 0;

  // Writing initial values to the motor pins:
#ifdef SAVE_ENERGY
  digitalWrite(PIN_ENABLE, HIGH); // Not using the holding torque feature (to save batteries)
#else
  digitalWrite(PIN_ENABLE, LOW); // Using the holding torque feature (bad for batteries; good for holding torque and accuracy)
#endif

  digitalWrite(PIN_SHUTTER, LOW);
  digitalWrite(PIN_AF, LOW);


  // Keypad stuff:
  // No locking for keys:
  keypad.setHoldTime(1000000);
  keypad.setDebounceTime(50);
  g.key_old = '=';


#ifndef MOTOR_DEBUG
  // Limiting switches should not be on when powering up:
  unsigned char limit_on = digitalRead(PIN_LIMITERS);
  if (limit_on == HIGH)
  {
    g.error = 1;
  }
#endif

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

  if (factory_reset)
  {
#ifdef MOTOR_DEBUG
    g.calibrate = 0;
    g.calibrate_warning = 0;
    g.calibrate_init = g.calibrate;
#else
    g.calibrate = 3;
#endif
    // Parameters for the reg structure:
    g.i_n_shots = 9;
    g.i_mm_per_frame = 5;
    g.i_fps = 16;
    g.i_first_delay = 4;
    g.i_second_delay = 3;
    g.i_accel_factor = 1;
    g.i_n_timelapse = 0;
    g.i_dt_timelapse = 5;
    g.mirror_lock = 1;
    g.backlash_on = 1;
    update_backlash();
    g.straight = 1;
    g.point1 = 2000;
    g.point2 = 3000;

    g.limit1 = 0;
    g.limit2 = 32767;
    g.pos = (g.point1 + g.point2) / 2;
    g.backlight = 1;
    // Assigning values to the reg structure:
    to_reg();
    // Saving these values in EEPROM:
    EEPROM.put( ADDR_POS, g.pos );
    EEPROM.put( ADDR_CALIBRATE, g.calibrate );
    EEPROM.put( ADDR_LIMIT1, g.limit1);
    EEPROM.put( ADDR_LIMIT2, g.limit2);
    EEPROM.put( ADDR_BACKLIGHT, g.backlight);
    EEPROM.put( ADDR_REG1, g.reg);
    EEPROM.put( ADDR_REG2, g.reg);
    EEPROM.put( ADDR_REG3, g.reg);
    EEPROM.put( ADDR_REG4, g.reg);
    EEPROM.put( ADDR_REG5, g.reg);
    put_reg();
  }
  else
  {
    // Reading the values from EEPROM:
    EEPROM.get( ADDR_POS, g.pos );
    EEPROM.get( ADDR_CALIBRATE, g.calibrate );
    EEPROM.get( ADDR_LIMIT1, g.limit1);
    EEPROM.get( ADDR_LIMIT2, g.limit2);
    EEPROM.get( ADDR_BACKLIGHT, g.backlight);
    get_reg();
  }

  // Five possible floating point values for acceleration
  set_accel_v();

  set_backlight();

  g.calibrate_flag = 0;
  if (g.calibrate == 3)
    g.calibrate_warning = 1;
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
  g.starting_point = g.point1;
  g.timelapse_counter = 0;
  g.timelapse_mode = 0;

  if (factory_reset)
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
  g.disable_limiters = 0;

  g.msteps_per_frame = Msteps_per_frame();
  g.Nframes = Nframes();

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
  g.calibrate = 0;
  g.calibrate_warning = 0;
  g.calibrate_init = g.calibrate;
  skipped_total = 0;
  n_fixed = 0;
  n_failed = 0;
  n1 = n2 = n3 = n4 = 0;
#endif

  // Default lcd layout:
  // This sets g.speed_limit, among other things:
  display_all();

#ifdef CAMERA_DEBUG
  shutter_status(0);
  AF_status(0);
  g.i_first_delay = 4;
  g.i_second_delay = 3;
#endif

  return;
}


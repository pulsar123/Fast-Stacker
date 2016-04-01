void camera()
/*
 Everything related to camera shutter and AF triggering
 */
{
  float speed;

  if (g.error > 0)
    return;


  if (g.stacker_mode == 1 && g.moving == 0 && g.started_moving == 0 && g.backlashing == 0 && g.start_stacking == 0)
    // We are here if the rail had to travel to the starting point for stacking, and now is ready for stacking
  {
    g.t0_mil = millis();
    g.start_stacking = 1;
    g.end_of_stacking = 0;
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
    g.frame_counter = 0;
    display_frame_counter();
    g.pos_to_shoot = g.pos_short_old;
    g.stacker_mode = 2;
  }

  // Enforcing the initial delay before a stacking (only for continuous mode):
  if (g.start_stacking == 2 && (g.t - g.t0_stacking > CONT_STACKING_DELAY || g.continuous_mode == 0 || AF_SYNC))
  {
    g.start_stacking = 3;

    if (g.continuous_mode)
    {
      // Required microsteps per frame:
      g.msteps_per_frame = Msteps_per_frame();
      // Estimating the required speed in microsteps per microsecond
      speed = target_speed();
      if (g.stacker_mode == 3)
        // 1-point stacking
      {
        go_to((float)g.limit2 + 0.5, speed);
      }
      else if (g.stacker_mode == 2)
        // 2-point stacking (after moving to the starting point)
      {
        go_to((float)g.destination_point + 0.5, speed);
      }

    }
  }

  // Non-continuous stacking mode
  if (g.continuous_mode == 0 && g.start_stacking == 3 && g.moving == 0 && g.started_moving == 0 && g.stacker_mode == 2)
  {
    if (g.noncont_flag == 2 && g.t - g.t_shot > FIRST_DELAY[g.i_first_delay] * 1e6)
    {
      g.noncont_flag = 3;
      // Initiating the second camera trigger (actual shot) in MIRROR_LOCK situation, or the only shot otherwise:
      g.make_shot = 1;
      g.t_shot = g.t;
    }
    else if (g.noncont_flag == 3 && g.t - g.t_shot > SECOND_DELAY[g.i_second_delay] * 1e6)
    {
      if (g.frame_counter < g.Nframes)
      {
        g.noncont_flag = 4;
        // Travelling to the next frame position in non-continuous stacking:
        go_to((float)g.pos_to_shoot + 0.5, g.speed_limit);
      }
      else
      {
        // The end of non-continuous stacking:
        g.start_stacking = 0;
        g.noncont_flag = 0;
        g.stacker_mode = 0;
        g.frame_counter = 0;
        display_frame_counter();
        letter_status(" ");
        g.end_of_stacking = 1;
      }
    }
  }

  // Triggering camera shutter when needed
  // This block is shared between continuous and non-continuous modes (in the latter case, it does the first shutter trigger, to lock the mirror)
  if (g.stacker_mode >= 2 && g.backlashing == 0 && g.start_stacking == 3)
  {
    if (g.pos_short_old == g.pos_to_shoot && g.shutter_on == 0 && (g.continuous_mode == 1 || g.noncont_flag == 1))
    {
      // Setting the shutter on:
      // If MIRROR_LOCK if not defined, the following shutter actuation will only take place in a continuous stacking mode
      // If it is defined, it will also happen in non-continuous mode, where it will be used to lock the mirror
      if (g.continuous_mode || g.mirror_lock == 1)
      {
        g.make_shot = 1;
      }
      // Even if the shutter is not triggered above, we need to record the current time, to set up the second delay (non-continuous mode):
      g.t_shot = g.t; 
      display_frame_counter();
      g.frame_counter++;
      // Position at which to shoot the next shot:
      g.pos_to_shoot = frame_coordinate();
      if (g.stacker_mode == 3 && g.frame_counter == N_SHOTS[g.i_n_shots])
      {
        // End of one-point stacking
        change_speed(0.0, 0, 2);
        g.stacker_mode = 0;
        g.frame_counter = 0;
        display_frame_counter();
      }
      if (g.continuous_mode == 0)
        g.noncont_flag = 2;
      else if (g.stacker_mode == 2 && g.frame_counter == g.Nframes)
      {
        g.stacker_mode = 0;
        g.frame_counter = 0;
        display_frame_counter();
        g.end_of_stacking = 1;
      }
    }
  }


  if (g.paused && g.noncont_flag == 2 && g.mirror_lock == 1)
    // We paused when the mirror is locked; release the lock right away
  {
    g.make_shot = 1;
    g.t_shot = g.t;
    g.noncont_flag = 0;
  }


  // Making decisions regarding whether to turn AF and shutter on or off:

  // Triggering camera's AF:
  if (g.start_stacking == 1 || g.AF_on == 0 && g.make_shot == 1 && (g.continuous_mode == 0 || g.single_shot == 1 || AF_SYNC))
  {
    // Switching camera's AF on
    if (g.continuous_mode == 1 && g.start_stacking == 1 || g.AF_on == 0 && g.make_shot == 1 && (g.continuous_mode == 0 || g.single_shot == 1 || AF_SYNC))
    {
      // Initiating AF now:
      digitalWrite(PIN_AF, HIGH);
      g.t_AF = g.t;
      g.AF_on = 1;
#ifdef CAMERA_DEBUG
      AF_status(1);
#endif
      g.single_shot = 0;
    }
    if (g.start_stacking == 1)
      g.start_stacking = 2;
  }

  // Triggering camera's shutter:
  if (g.make_shot == 1 && g.AF_on == 1 && ((g.mirror_lock < 2 && g.t - g.t_AF >= SHUTTER_ON_DELAY) || (g.mirror_lock == 2 && g.t - g.t_AF >= SHUTTER_ON_DELAY2)))
  {
#ifndef DISABLE_SHUTTER
    digitalWrite(PIN_SHUTTER, HIGH);
#endif
#ifdef CAMERA_DEBUG
    shutter_status(1);
#endif
    g.shutter_on = 1;
    g.t_shutter = g.t;
    g.make_shot = 0;
  }

  // Making sure that the shutter is pressed for at least SHUTTER_TIME microseconds
  if (g.shutter_on == 1 && g.t - g.t_shutter >= SHUTTER_TIME_US)
  {
    // Releasing the shutter:
#ifndef DISABLE_SHUTTER
    digitalWrite(PIN_SHUTTER, LOW);
#endif
#ifdef CAMERA_DEBUG
    shutter_status(0);
#endif
    g.shutter_on = 0;
    g.t_shutter_off = g.t;
  }

  // Depress the camera's AF when it's no longer needed
  // and only if the shutter has been off for at least SHUTTER_OFF_DELAY microseconds:
  if (g.make_shot == 0 && g.AF_on == 1 && g.shutter_on == 0 && ((g.mirror_lock < 2 && g.t - g.t_shutter_off >= SHUTTER_OFF_DELAY) || (g.mirror_lock == 2 && g.t - g.t_shutter_off >= SHUTTER_OFF_DELAY2)) &&
      (g.continuous_mode == 0 || g.stacker_mode == 0 || g.paused == 1 || AF_SYNC))
  {
    digitalWrite(PIN_AF, LOW);
#ifdef CAMERA_DEBUG
    AF_status(0);
#endif
    g.AF_on = 0;
    g.single_shot = 0;
  }


  // Timelapse module:
  if (g.end_of_stacking && g.moving == 0 && g.paused == 0)
  {
    if (g.timelapse_counter < N_TIMELAPSE[g.i_n_timelapse] - 1)
    {
      // Special stacker mode: waiting between stacks in a timelapse sequence:
      g.stacker_mode = 4;
      g.t_mil = millis();
      if (((float)(g.t_mil - g.t0_mil)) / 1000.0 > (float)DT_TIMELAPSE[g.i_dt_timelapse])
        // We are initiating the next stacking in the timelapse sequence
      {
        g.end_of_stacking = 0;
        g.t0_mil = g.t_mil;
        g.timelapse_counter++;
        go_to((float)g.point1 + 0.5, g.speed_limit);
        g.stacker_mode = 1;
        g.start_stacking = 0;
      }
    }
    else
      // End of timelapse, or when no timelapse (N_timelapse=1)
    {
      g.end_of_stacking = 0;
      g.timelapse_mode = 0;
      display_all();
    }
  }

  return;
}


void camera()
/*
 Everything related to camera shutter triggering
 */
{
  float speed;

  if (g.error > 0)
    return;

  // Enforcing the initial delay before a stacking:
  if (g.start_stacking == 1 && g.t - g.t0_stacking > STACKING_DELAY)
  {
    g.start_stacking = 0;

    if (g.continuous_mode)
    {
      // Required microsteps per frame:
      g.msteps_per_frame = Msteps_per_frame();
      // Estimating the required speed in microsteps per microsecond
      speed = target_speed();
      if (g.stacker_mode == 3)
        // 1-point stacking
      {
        if (g.stacking_direction == -1)
          go_to((float)g.limit1 + 0.5, speed);
        else
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
  if (g.continuous_mode == 0 && g.start_stacking == 0)
  {
    if (g.moving == 0 && g.stacker_mode == 2)
    {
      if (g.noncont_flag == 2 && g.t - g.t_shutter > FIRST_DELAY[g.i_first_delay] * 1e6)
      {
        g.noncont_flag = 3;
        // Triggering shutter second time (actual shot):
        digitalWrite(PIN_SHUTTER, HIGH);
#ifdef CAMERA_DEBUG
        shutter_status(1);
#endif
        g.shutter_on = 1;
        g.t_shutter = g.t;
      }
      else if (g.noncont_flag == 3 && g.t - g.t_shutter > SECOND_DELAY[g.i_second_delay] * 1e6)
      {
        if (g.frame_counter < g.Nframes)
        {
          g.noncont_flag = 4;
          go_to((float)g.pos_to_shoot + 0.5, g.speed_limit);
        }
        else
        {
          // The end of non-continuous stacking:
          g.noncont_flag = 0;
          g.stacker_mode = 0;
          g.frame_counter = 0;
          display_frame_counter();
          letter_status("  ");
        }
      }
    }
  }



  // Triggering camera shutter when needed
  // This block is shared between continuous and non-continuous modes (in the latter case, it does the first shutter trigger, to lock the mirror)
  if (g.stacker_mode >= 2 && g.backlashing == 0 && g.start_stacking == 0)
  {
    if (g.pos_short_old == g.pos_to_shoot && g.shutter_on == 0 && (g.continuous_mode == 1 || g.noncont_flag == 1))
    {
      // Setting the shutter on:
#ifndef MIRROR_LOCK
      // If MIRROR_LOCK if not defined, the following shutter actuation will only take place in a continuous stacking mode
      // If it is defined, it will also happen in non-continuous mode, where it will be used to lock the mirror
      if (g.continuous_mode)
#endif
        digitalWrite(PIN_SHUTTER, HIGH);
#ifdef CAMERA_DEBUG
      shutter_status(1);
#endif
      g.shutter_on = 1;
      g.t_shutter = g.t;
      display_frame_counter();
      g.frame_counter++;
      // Position at which to shoot the next shot:
      g.pos_to_shoot = g.starting_point + g.stacking_direction * nintMy(((float)g.frame_counter) * g.msteps_per_frame);
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
      }
    }
  }



  // Making sure that the shutter is pressed for at least SHUTTER_TIME microseconds
  if (g.shutter_on == 1 && g.t - g.t_shutter >= SHUTTER_TIME_US && g.start_stacking == 0)
  {
    // Releasing the shutter:
    digitalWrite(PIN_SHUTTER, LOW);
#ifdef CAMERA_DEBUG
    shutter_status(0);
#endif
    g.shutter_on = 0;
  }

  if (g.paused && g.noncont_flag == 2)
    // We paused when the mirror is locked; release the lock right away
  {
    digitalWrite(PIN_SHUTTER, HIGH);
#ifdef CAMERA_DEBUG
    shutter_status(1);
#endif
    g.shutter_on = 1;
    g.t_shutter = g.t;
    g.noncont_flag = 0;
  }

#ifdef H1.2
  // Depress the camera's AF when it's no longer needed (after stacking, when pasued, and after a single shot),
  // and only if the shutter is off:
  if (g.AF_on == 1 && g.shutter_on == 0 && (g.stacker_mode == 0 || g.single_shot == 1 || g.paused == 1))
  {
    digitalWrite(PIN_AF, LOW);
#ifdef CAMERA_DEBUG
    AF_status(0);
#endif
    g.AF_on = 0;
    g.single_shot = 0;
  }
#endif

  return;
}


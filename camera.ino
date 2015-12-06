void camera()
/*
 Everything related to camera shutter triggering
 */
{
  float speed;

  if (g.error > 0)
    return;

  // Initiating the go_to travel for stacking
  if (g.start_stacking == 1 && g.t - g.t0_stacking > STACKING_DELAY)
  {
    g.start_stacking = 0;
    // Required microsteps per frame:
    g.msteps_per_frame = Msteps_per_frame();
    // Estimating the required speed in microsteps per microsecond
    speed = SPEED_SCALE * FPS[g.i_fps] * MM_PER_FRAME[g.i_mm_per_frame];
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

  // Triggering camera shutter when needed
  if (g.stacker_mode >= 2 && g.backlashing == 0)
  {
    if (g.pos_short_old == g.pos_to_shoot && g.shutter_on == 0)
    {
      // Setting the shutter on:
      digitalWrite(PIN_SHUTTER, HIGH);
      g.shutter_on = 1;
      g.t_shutter = g.t;
      g.frame_counter++;
      display_frame_counter();
      // Position at which to shoot the next shot:
      g.pos_to_shoot = g.starting_point + g.stacking_direction * nintMy(((float)g.frame_counter) * g.msteps_per_frame);
      if (g.stacker_mode == 3 && g.frame_counter == N_SHOTS[g.i_n_shots])
      {
        // End of one-point stacking
        change_speed(0.0, 0, 2);
        g.stacker_mode = 0;
      }
      if (g.stacker_mode == 2 && g.frame_counter == g.Nframes)
        g.stacker_mode = 0;
    }
  }

  // Making sure that the shutter is pressed for at least SHUTTER_TIME microseconds
  if (g.shutter_on == 1 && g.t - g.t_shutter >= SHUTTER_TIME_US && g.start_stacking == 0)
  {
    // Releasing the shutter:
    digitalWrite(PIN_SHUTTER, LOW);
    g.shutter_on = 0;
  }

  return;
}


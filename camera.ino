void camera()
/*
 Everything related to camera shutter triggering
 */
{

  if (g.stacker_mode >= 2)
  {
    if (g.pos_short_old == g.pos_to_shoot && g.shutter_on == 0)
    {
      // Setting the shutter on:
      digitalWrite(PIN_SHUTTER, HIGH);
      g.shutter_on = 1;
      g.t_shutter = g.t;
      g.frame_counter++;
      // Position at which to shoot the next shot:
      g.pos_to_shoot = g.first_point + g.stacking_direction * nintMy(((float)g.frame_counter) * g.msteps_per_frame);
      if (g.stacker_mode == 3 && g.frame_counter == N_SHOTS[g.i_n_shots])
      {
        // End of one-point stacking
        change_speed(0.0, 0);
        g.stacker_mode = 0;
      }
    }
  }

  // Making sure that the shutter is pressed for at least SHUTTER_TIME microseconds
  if (g.shutter_on==1 && g.t-g.t_shutter>=SHUTTER_TIME_US)
  {
    // Releasing the shutter:
    digitalWrite(PIN_SHUTTER, LOW);
    g.shutter_on = 0;
  }

  return;
}


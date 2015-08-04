void camera()
/*
 Everything related to camera shutter triggering
 */
{

  if (g.pos_short_old==g.pos_to_shoot && g.shutter_on==0)
  {
    // Setting the shutter on:
    digitalWrite(PIN_SHUTTER, HIGH);
    g.shutter_on = 1;
    g.t_shutter = g.t;
    g.frame_counter++;
    // Position at which to shoot the next shot:
    g.pos_to_shoot = g.first_point + g.stacking_direction * nintMy(((float)g.frame_counter) * g.msteps_per_frame);
  }

// Making sure that the shutter is pressed for at least SHUTTER_TIME microseconds
  if (g.t - g.t_shutter >= SHUTTER_TIME_US && g.shutter_on == 1)
  {
    // Releasing the shutter:
    digitalWrite(PIN_SHUTTER, LOW);
    g.shutter_on = 0;
  }

  return;
}


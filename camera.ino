void camera()
/*
 Everything related to camera shutter triggering
 */
{

  if (g.error > 0)
    return;

  if (g.stacker_mode >= 2)
  {
#ifdef DEBUG
/*
        Serial.print(" g.pos_to_shoot=");
        Serial.print(g.pos_to_shoot);
        Serial.print(", floorMy(g.pos-0.5*g.stacking_direction)=");
        Serial.print(floorMy(g.pos-0.5*g.stacking_direction));
        Serial.print(", g.shutter_on=");
        Serial.println(g.shutter_on);
        */
#endif  
    // The additional factor -0.5*g.stacking_direction is to ensure that shutter always triggeres between microsteps:
    // (Make sure that in 2-points mode we always travel one more microstep, to allow the last shot to happen)
    if (floorMy(g.pos-0.5*g.stacking_direction) == g.pos_to_shoot && g.shutter_on == 0)
    {
#ifdef DEBUG
  Serial.println("Shutter ON");      
#endif  
      // Setting the shutter on:
      digitalWrite(PIN_SHUTTER, HIGH);
      g.shutter_on = 1;
      g.t_shutter = g.t;
      g.frame_counter++;
      // Position at which to shoot the next shot:
      g.pos_to_shoot = g.starting_point + g.stacking_direction * nintMy(((float)g.frame_counter) * g.msteps_per_frame);
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
#ifdef DEBUG
  Serial.println("Shutter OFF");      
#endif  
    // Releasing the shutter:
    digitalWrite(PIN_SHUTTER, LOW);
    g.shutter_on = 0;
  }

  return;
}


// Cleaning up at the end of each loop

void cleanup()
{
  g.t_old = g.t;
  g.accel_old = g.accel;
  
  if (g.moving == 0 && g.started_moving == 0)
    EEPROM.commit(); // The actual EEPROM update only happens when not moving

#ifdef TIMING
  timing();  
#endif
  return;
}

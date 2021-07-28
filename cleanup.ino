// Cleaning up at the end of each loop

void cleanup()
{
  
  if (g.moving == 0 && g.model_init == 0)
    EEPROM.commit(); // The actual EEPROM update only happens when not moving

#ifdef TIMING
  timing();  
#endif
  return;
}

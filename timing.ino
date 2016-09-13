#ifdef TIMING
void timing()
{

  if (g.moving == 0)
    return;

  g.i_timing++;

  if (g.i_timing == 1)
    g.t0_timing = g.t;

  // Skipping the first call, as it measures the pre-motion loop timing:
  if (g.i_timing < 2)
    return;

  // Last loop length:
  short dt = (short)(g.t - g.t_old);

  // Counting the number of loops longer than the shortest microstep interval allowed, for the last movement:
  if ((float)dt > (1.0 / SPEED_LIMIT))
    g.bad_timing_counter++;

  // Finding the longest loop length, for the last movement:
  //  float dpos = g.pos - (float)g.pos_short_old;
  //  dt = 10.0 * (abs(dpos));
  if (dt > g.dt_max)
    g.dt_max = dt;

  if (dt < g.dt_min)
    g.dt_min = dt;

  return;
}
#endif



#ifdef DUMP_REGS
void dump_regs()
/*
   Dumping the contents of the memory registers to serial monitor.
   For now it's only (raw) positions and temperatures for the telescope mode.
*/
{
  int eeAddress; //EEPROM address

  // Macro regs:
  /*
    eeAddress = ADDR_REG1;
    for (byte i = 0; i < N_REGS; i++)
    {
      EEPROM.get(eeAddress, g.reg);
      Serial.println(g.reg);
      eeAddress = eeAddress + SIZE_REG;
    }
  */
  // Telescope regs:
  eeAddress = ADDR_REG1_TEL;
  for (byte i = 0; i < N_REGS; i++)
  {
    EEPROM.get(eeAddress, g.reg);
    for (byte j = 0; j < 4; j++)
    {
      Serial.print(g.reg.point[j]);
      Serial.print(" ");
      Serial.print(g.reg.raw_T[j]);
      if (j < 3)
        Serial.print(" ");
    }
    Serial.print("\n");
    eeAddress = eeAddress + SIZE_REG;
  }

}


void write_regs()
/*
   Opposite to dump_regs - updating EEPROM regs with the data read from the serial monitor.
   For now it's only (raw) positions and temperatures for the telescope mode.
*/
{
  int eeAddress; //EEPROM address
  int value1, value2;

  // Telescope regs:
  eeAddress = ADDR_REG1_TEL;
  for (byte i = 0; i < N_REGS; i++)
  {
//    Serial.println(i);
    for (byte j = 0; j < 4; j++)
    {
      do
      {
        value1 = Serial.parseInt();
      }
      while (value1 == 0);
      do
      {
        value2 = Serial.parseInt();
      }
      while (value2 == 0);
      //      Serial.println(value1);
      //      Serial.println(value2);
      g.reg.point[j] = value1;
      g.reg.raw_T[j] = value2;
    }
    // Updating the reg's value in EEPROM only if it changed:
    EEPROM.put(eeAddress, g.reg);
    eeAddress = eeAddress + SIZE_REG;
  }

}
#endif


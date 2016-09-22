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
  for (byte i = 0; i < g.n_regs; i++)
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
  for (byte i = 0; i < g.n_regs; i++)
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


#ifdef TEST_SWITCH
void test_switch()
{
  float breaking_distance;

  if (g.moving || g.started_moving || g.error)
    return;

  switch (g.test_flag)
  {
    case 0:
      // Alternatively testing maximum speed, and slow speed:
//      if (g.test_N % 2 == 0)
//        g.speed_test = g.speed_limit;
//      else
        g.speed_test = g.speed_limit / 5.0;
      breaking_distance = 0.5 * g.speed_test * g.speed_test / g.accel_limit;
      // Initial positioning:
      go_to(g.pos + 4.0 * breaking_distance, g.speed_limit);
      g.test_flag = 1;
      break;

    case 2:
      // Moving toward foreground switch with current speed:
      change_speed(-g.speed_test, 0, 2);
      g.test_flag = 3;
      break;

    case 4:
      // We just made a test switch triggering and stopped; the trigger position is stored in g.limit_tmp
      if (g.test_N == 0)
        g.test_pos0 = g.limit_tmp;
      g.test_N++;
      // Coordinates relative to the first triggered position, to minimize roundoff errors:
      float delta = g.limit_tmp - g.test_pos0;
      g.test_sum = g.test_sum + delta;
      g.test_sum2 = g.test_sum2 + delta * delta;
      if (delta > g.delta_max)
        g.delta_max = delta;
      if (delta < g.delta_min)
        g.delta_min = delta;
      if (g.test_N > 1)
      {
        g.test_avr = g.test_sum / (float)g.test_N;
        g.test_std = sqrt(g.test_sum2 / (float)g.test_N - g.test_avr * g.test_avr);
        g.test_dev = (g.delta_max - g.delta_min) / 2.0;
      }
      //      display_current_position();
      if (g.test_N > TEST_N_MAX)
        g.test_flag = 10;
      else
        g.test_flag = 0;
      break;
  }
  return;
}
#endif


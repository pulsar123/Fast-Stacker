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
  for (byte i = 0; i < N_REGS_TEL; i++)
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
  for (byte i = 0; i < N_REGS_TEL; i++)
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
  float breaking_distance, delta, x;
  byte i;

  if (g.moving || g.started_moving || g.error)
    return;

  switch (g.test_flag)
  {
    case 0:
      g.speed_test = g.speed_limit;
      breaking_distance = 0.5 * g.speed_test * g.speed_test / g.accel_limit;
      // Initial positioning; padding extra 5mm to make sure the switch lever is touched while moving at the maximum speed
      x = g.pos + 2 * breaking_distance + 5.0 / MM_PER_MICROSTEP;
      go_to(x, g.speed_limit);
      g.test_flag = 1;
      g.limit_on[1] = 0;
      break;

    case 2:
      // Computing the switch off stats, starting from test_N=1
      i = 1;
      if (g.test_N > 0)
      {
#ifdef SERIAL_SWITCH
        Serial.print("1 ");
        Serial.println(g.pos_tmp2);
#endif
        delay(250);
      }
      if (g.test_N > 1)
      {
        // Coordinates relative to the first triggered position, to minimize roundoff errors:
        delta = g.pos_tmp2 - g.test_limit;
        g.test_sum[i] = g.test_sum[i] + delta;
        g.test_sum2[i] = g.test_sum2[i] + delta * delta;
        if (delta > g.delta_max[i])
          g.delta_max[i] = delta;
        if (delta < g.delta_min[i])
          g.delta_min[i] = delta;
      }
      if (g.test_N > 2)
      {
        g.test_avr[i] = g.test_sum[i] / (float)(g.test_N - 1);
        g.test_std[i] = sqrt(g.test_sum2[i] / (float)(g.test_N - 1) - g.test_avr[i] * g.test_avr[i]);
        g.test_dev[i] = (g.delta_max[i] - g.delta_min[i]) / 2.0;
      }
      display_all();

      // Moving toward foreground switch with current speed:
      change_speed(-g.speed_test, 0, 2);
      g.test_flag = 3;
      g.limit_on[0] = 0;
      break;

    case 4:
      // We just made a test switch triggering and stopped; the trigger position is stored in g.pos_tmp
#ifdef SERIAL_SWITCH
      Serial.print("0 ");
      Serial.println(g.pos_tmp);
#endif
      delay(250);
      i = 0;
      if (g.test_N == 0)
        g.test_limit = g.pos_tmp;
      g.test_N++;
      // Coordinates relative to the first triggered position, to minimize roundoff errors:
      delta = g.pos_tmp - g.test_limit;
      g.test_sum[i] = g.test_sum[i] + delta;
      g.test_sum2[i] = g.test_sum2[i] + delta * delta;
      if (delta > g.delta_max[i])
        g.delta_max[i] = delta;
      if (delta < g.delta_min[i])
        g.delta_min[i] = delta;
      if (g.test_N > 1)
      {
        g.test_avr[i] = g.test_sum[i] / (float)g.test_N;
        g.test_std[i] = sqrt(g.test_sum2[i] / (float)g.test_N - g.test_avr[i] * g.test_avr[i]);
        g.test_dev[i] = (g.delta_max[i] - g.delta_min[i]) / 2.0;
      }
      //      display_current_position();
      if (g.test_N > TEST_N_MAX)
        // Stuff to do after the test is done
      {
        g.test_flag = 10;
        COORD_TYPE d = g.pos_short_old - g.pos0_test;
        if (d % N_MICROSTEPS > 0)
        {
          // The next full step coordinate:
          float next_step = g.pos_short_old + N_MICROSTEPS * (d / N_MICROSTEPS + 1) + 0.5;
          // Parking macro rail at the next full step position after the test:
          go_to(next_step, g.speed_limit);
        }
      }
      else
        // Next test:
        g.test_flag = 0;
      display_all();
      break;
  }
  return;
}
#endif


#ifdef TEST_HALL
void Testing_Hall()
{
  byte limiter_on = digitalRead(PIN_LIMITERS);
  if (g.hall_on == 0 && limiter_on == 1)
  {
    g.hall_on = 1;
    g.backlight = N_BACKLIGHT - 1;
    set_backlight();
  }
  if (g.hall_on == 1 && limiter_on == 0)
  {
    g.hall_on = 0;
    g.backlight = 0;
    set_backlight();
  }
  return;
}
#endif


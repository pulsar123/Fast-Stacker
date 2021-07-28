#ifdef TIMING
void timing()
{

  if (g.moving_old == 0 && g.moving == 1)
    g.t0_timing = g.t;

  if (g.moving_old == 1 && g.moving == 0)
  {
      g.total_dt_timing = g.total_dt_timing + (g.t - g.t0_timing);
      g.dt_timing = 0;
  }

  g.moving_old = g.moving;
  
  if (g.moving == 0 || g.dt_timing == 0)
    return;

  g.i_timing++;

  // Counting the number of loops longer than the shortest microstep interval allowed, for the last movement:
  if ((float)g.dt_timing > (1.0 / SPEED_LIMIT))
    g.bad_timing_counter++;

  // Finding the longest loop length, for the last movement:
  //  float dpos = g.pos - (float)g.ipos;
  //  dt = 10.0 * (abs(dpos));
  if (g.dt_timing > g.dt_max)
    g.dt_max = g.dt_timing;

  if (g.dt_timing < g.dt_min)
    g.dt_min = g.dt_timing;

  return;
}
#endif

#ifdef TEST_SWITCH
void test_switch()
{
  float breaking_distance, delta, x;
  byte i;

  if (g.moving || g.model_init || g.error)
    return;

  switch (g.test_flag)
  {
    case 0:
      g.speed_test = SPEED_LIMIT;
      breaking_distance = 0.5 * g.speed_test * g.speed_test / ACCEL_LIMIT;
      // Initial positioning; padding extra 5mm to make sure the switch lever is touched while moving at the maximum speed
      x = g.ipos + 2 * breaking_distance + 5.0 / MM_PER_MICROSTEP;
      go_to(x, SPEED_LIMIT);
      g.test_flag = 1;
      g.test_limit_on[1] = 0;
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
        g.test_avr[i] = g.test_sum[i] / (float)(g.test_N - 1);
      }
      if (g.test_N > 2)
      {
        g.test_std[i] = sqrt(g.test_sum2[i] / (float)(g.test_N - 1) - g.test_avr[i] * g.test_avr[i]);
        g.test_dev[i] = (g.delta_max[i] - g.delta_min[i]) / 2.0;
      }
      display_all();

      // Moving toward foreground switch with current speed:
      go_to(g.limit1, g.speed_test);
      g.test_flag = 3;
      g.test_limit_on[0] = 0;
      g.on_init = 0;
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
      g.test_avr[i] = g.test_sum[i] / (float)g.test_N;
      if (g.test_N > 1)
      {
        g.test_std[i] = sqrt(g.test_sum2[i] / (float)g.test_N - g.test_avr[i] * g.test_avr[i]);
        g.test_dev[i] = (g.delta_max[i] - g.delta_min[i]) / 2.0;
      }
      if (g.test_N > TEST_N_MAX)
        // Stuff to do after the test is done
      {
        g.test_flag = 10;
        COORD_TYPE d = g.ipos - g.pos0_test;
        if (d % N_MICROSTEPS > 0)
        {
          // The next full step coordinate:
          float next_step = g.ipos + N_MICROSTEPS * (d / N_MICROSTEPS + 1) + 0.5;
          // Parking macro rail at the next full step position after the test:
          go_to(next_step, SPEED_LIMIT);
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




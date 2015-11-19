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



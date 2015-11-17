#ifdef TIMING
void timing()
{
  g.i_timing++;
  if (g.i_timing == N_TIMING)
  {
    float dt_loop = (float)(g.t - g.t_old) / (float)N_TIMING;
    // Inverse speed_limit is us/ustep:
    float loops_per_step = 1.0 / SPEED_LIMIT / dt_loop;
    // Displaying the average loop time (us), and number of loops per motor step (at maximum allowed speed)
    //    sprintf(g.buffer, "%5fus, %5.1f", dt_loop, dt_step / dt_loop);
    sprintf(g.buffer, "%5dus, %3d.%1d", (int)dt_loop, (int)loops_per_step, (int)(10.0 * (loops_per_step - (int)loops_per_step)));
#ifdef DEBUG
    Serial.println(g.buffer);
#endif
#ifdef LCD
    lcd.setCursor(0, 4);
    lcd.print(g.buffer);
#endif
    g.i_timing = 0;
    g.t_old = g.t;
  }
}
#endif


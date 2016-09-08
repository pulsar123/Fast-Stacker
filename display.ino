/* Regular (every Arduino loop) display processing stuff
*/

void display_stuff()
{
  // Disabling the last comment line displaying after COMMENT_DELAY interval:
  if (g.comment_flag == 1 && g.t > g.t_comment + COMMENT_DELAY)
  {
    g.comment_flag = 0;
    if (g.moving == 0)
      if (g.alt_flag)
        display_all();
      else
        display_current_position();
    //        display_all();
  }

  // Refreshing battery status regularly (only when not moving, as it is slow):
  if (g.moving == 0 && g.calibrate_warning == 0 && g.t - g.t_display > DISPLAY_REFRESH_TIME)
  {
    g.t_display = g.t;
#ifdef TEMPERATURE
    if (g.telescope)
      // Measuring temperature (only in telescope mode), from thermistor connected to PIN_AF:
      measure_temperature();
#endif
    if (g.alt_flag)
      display_all();
    else
      //       battery_status();
      display_status_line();
  }

  return;
}

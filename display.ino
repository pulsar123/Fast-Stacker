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

  // Refreshing whole display regularly (only when not moving, as it is slow):
  if (g.moving == 0 && g.calibrate_warning == 0 && g.t - g.t_display > DISPLAY_REFRESH_TIME)
  {
    g.t_display = g.t;
    battery_status();
  }

  return;
}

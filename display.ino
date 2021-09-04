/* Regular (every Arduino loop) display processing stuff
*/

void display_stuff()
{

  if (g.help_mode)
    return;

  // Disabling the last comment line displaying after COMMENT_DELAY interval:
  if (g.comment_flag == 1 && g.t > g.t_comment + COMMENT_DELAY)
  {
    g.comment_flag = 0;
    if (g.alt_flag == 0)
      display_current_position(0);
  }

#ifdef TEST_SWITCH
  if (g.t - g.t_display > DISPLAY_REFRESH_TIME && g.moving == 0)
  {
    g.t_display = g.t;
    display_current_position(0);
  }
#endif


  // Refreshing display (position and battery status) regularly:
  if (g.stacker_mode == 0 && g.alt_flag == 0 && g.error == 0 && g.editing == 0 && g.t - g.t_display > DISPLAY_REFRESH_TIME)
  {
    g.t_display = g.t;
    battery_status(1);
    if (g.comment_flag == 0)
      display_current_position(1);
  }


  return;
}

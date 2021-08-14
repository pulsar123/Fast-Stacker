#ifdef BUZZER
void buzzer()
{
  if (g.beep_on == 0)
    return;

  if (g.t - g.t_beep > g.beep_length)
    g.beep_on = 0;

  if (g.t - g.t_buzz > g.dt1_buzz_us)
    {
      g.buzz_state = 1 - g.buzz_state;
      iochip.digitalWrite(EPIN_BUZZ, g.buzz_state);
      g.t_buzz = g.t;  
    }

}
#endif

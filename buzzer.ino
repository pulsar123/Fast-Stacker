#ifdef BUZZER
void buzzer()
{
  //!!!
  /*
    if (g.t-g.t_beep > 10000000)
    {
        g.t_beep = g.t;
        g.buzz_state = 1 - g.buzz_state;
        iochip.digitalWrite(EPIN_BUZZ, g.buzz_state);
    }
    return;
  */

  if (g.reg.buzzer == 0)
    return;

#ifdef BUZZER_PASSIVE
  if (g.beep_on == 0)
    return;
#endif

  TIME_UTYPE t = micros();

  if (g.beep_on == 1 && t - g.t_beep > g.beep_length)
  {
    g.beep_on = 0;
    g.buzz_state = 0;
    iochip.digitalWrite(EPIN_BUZZ, g.buzz_state);
    return;
  }

#ifdef BUZZER_PASSIVE
  if (t - g.t_buzz > g.dt1_buzz_us)
  {
    g.buzz_state = 1 - g.buzz_state;
    iochip.digitalWrite(EPIN_BUZZ, g.buzz_state);
    g.t_buzz = t;
  }

#else
  // Active buzzer:
  if (g.beep_on == 1 && g.buzz_state == LOW)
    // Turning the buzzer on
  {
    g.buzz_state = HIGH;
    iochip.digitalWrite(EPIN_BUZZ, g.buzz_state);
  }

  if (g.beep_on == 0 && g.buzz_state == HIGH)
    // Turning the buzzer off
  {
    g.buzz_state = LOW;
    iochip.digitalWrite(EPIN_BUZZ, g.buzz_state);
  }
#endif

  return;
}
#endif

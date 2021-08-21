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

#ifdef BUZZER_PASSIVE
  if (g.beep_on == 0)
    return;
#endif    

  if (g.beep_on == 1 && g.t - g.t_beep > g.beep_length)
  {
    g.beep_on = 0;
    return;
  }

#ifdef BUZZER_PASSIVE
  if (g.t - g.t_buzz > g.dt1_buzz_us)
    {
      g.buzz_state = 1 - g.buzz_state;
      iochip.digitalWrite(EPIN_BUZZ, g.buzz_state);
      g.t_buzz = g.t;  
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

}
#endif

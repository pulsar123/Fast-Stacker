#ifdef BUZZER
void buzzer()
{

  if (g.t - g.t_buzz > DT_BUZZ_US)
    {
      g.buzz_state = 1 - g.buzz_state;
      iochip.digitalWrite(EPIN_BUZZ, g.buzz_state);
      g.t_buzz = g.t;  
    }

}
#endif

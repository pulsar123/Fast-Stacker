void process_keypad()
/*
 All the keypad runtime stuff goes here
 */
{
  // Action is only needed if the kepad state changed since the last time:
  if (keypad.keyStateChanged())
  {
    // Reading a keypad key if any:
    char key = keypad.getKey();

    switch (key)
    {
      case NO_KEY: // Breaking / stopping if no keys pressed (only after rewind/fastforward)
      if (g.key_old=='1' || g.key_old=='A')
          change_speed(0.0, 0);
      case '1':  // Rewinding
        change_speed(-SPEED_LIMIT, 0);
        break;
      case 'A':  // Fast forwarding
        change_speed(SPEED_LIMIT, 0);
        break;
      case '7':  // Go to the foreground point
        go_to(g.point1);
        break;
      case 'C':  // Go to the background point
        go_to(g.point2);
        break;
    }
    g.key_old = key;
  }

  return;
}


void help()
/*
 * Help menu
 * 
 */
{

#define H_TEXT TFT_SILVER
#define H_TITLE TFT_WHITE
#define H_SUBTITLE TFT_SKYBLUE
#define H_PAGE TFT_SKYBLUE
#define H_KEY TFT_ORANGE

  if (g.help_mode == 0)
    return;

  tft.fillScreen(TFT_BLACK);

  tft.setCursor(0, 0);
  tft.setTextColor(H_TEXT, TFT_BLACK);

  switch (g.help_page)
  {
    case 0:
      tft.setTextColor(H_TITLE, TFT_BLACK);
      tft.println(" HELP (1/A to scroll)");
      help_topic("      MAIN SCREEN");
      help_line("1/A", "Rewind/FastForward");
      help_line("4", "Set foreground point");
      help_line("7", "GoTo foreground pnt");
      help_line("B", "Set background point");
      help_line("C", "GoTo background pnt");
      break;

    case 1:
      help_line("0", "Switch mode");
      help_line("D", "Start shooting");
      help_line("5", "Stack step, um");
      help_line("8", "Number of shots [1PC]");
      tft.println("   Frames per sec [2PC]");
      tft.println("   Delay 1, sec [2PN]");
      help_line("9", "Delay 2, sec [2PN]");
      break;

    case 2:
      help_line("*", "Alternative screen");
      help_line("2", "GoTo (mm)");
      help_line("3", "Parking");
      help_line("6", "Help");
      help_line("#1", "Move one step back");
      help_line("#A", "Move one step ahead");
      help_line("#B", "Break");
      break;

    case 3:
      help_line("#C", "Calibrate rail");
      help_line("#*", "Factory reset");
      help_line("#7", "Trigger camera");
      help_line("#2/3", "Write/Read bank 1");
      help_line("#5/6", "Write/Read bank 2");
      help_line("#8/9", "Write/Read bank 3");
      help_line("*2/3", "Write/Read bank 4");
      break;

    case 4:
      help_line("*5/6", "Write/Read bank 5");
      help_line("*1", "Rail reverse");
      help_line("*4", "N timelapse passes");
      help_line("*7", "Time between time-");
      tft.println("    lapse passes, sec");
      help_line("*0", "Save energy on/off");
      break;

    case 5:
      help_line("*A", "Acceleration factor");
      tft.println("    larger means slower");
      help_line("*9", "Accel factor 2");
      help_line("*B", "Backlash mode");
      help_line("*C", "Camera mirror mode");
      help_line("*D", "Buzzer on/off");
      break;

    case 6:
      help_topic("    WHEN STACKING");
      help_line("Any key", "Abort [1PC]");
      tft.println("      Pause [2PC,2PN]");
      break;

    case 7:
      help_topic("    WHEN PAUSED");
      help_line("D", "Resume stacking");
      help_line("1", "Move 10 frames back");
      help_line("A", "Move 10 frames ahead");
      help_line("2", "GoTo specific frame");
      help_line("6", "Help");
      break;

    case 8:
      help_line("#1", "Move 1 frame back");
      help_line("#A", "Move 1 frame ahead");
      help_line("#B", "Abort stacking");
      help_line("#7", "Trigger camera");
      break;

    case 9:
      help_topic("    EDITING MODE");
      help_line("0-9", "Type the digit");
      help_line("#", "Type dot");
      help_line("A", "Accept the number");
      help_line("B", "Backspace");
      help_line("C", "Cancel");
      break;
  }



  my_setCursor(TFT_NX - 5, TFT_NY, 0);
  g.y0 = g.y0 - 16;
  tft.setCursor(g.x0, g.y0);
  tft.setTextColor(H_PAGE, TFT_BLACK);
  tft.setTextFont(1);

  tft.print(g.help_page + 1);
  tft.print("/");
  tft.print(N_HELP_PAGES);
  tft.setTextFont(2);


  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void help_line(char* key, char* string)
// Print one line in  help mode
{
  tft.setTextColor(H_KEY, TFT_BLACK);
  tft.print(key);
  tft.print(" ");
  tft.setTextColor(H_TEXT, TFT_BLACK);
  tft.println(string);
  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void help_topic(char* topic)
// Print help topic (one line)
{

  tft.setTextColor(H_SUBTITLE, TFT_BLACK);
  tft.println(topic);

  return;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


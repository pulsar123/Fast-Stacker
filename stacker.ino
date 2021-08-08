/* Sergey Mashchenko 2015-2021

   Fast Stacker: an automated macro rail for focus stacking

   Online tutorial: http://pulsar124.wikia.com/wiki/Fast_Stacker

    Major rehaul of hardware and software.
    Hardware h2.0
    
 - ESP8266 microcontroller (D1 Mini)
 - MCP23S17 chip - 16 GPIO expander using SPI
 - SPI-driven TFT display ST7735
 - 4x4 keypad
 - DRV8825 stepper motor driver
 - stepper motor
 
   The hardware used:
   Original (h1.0):
    - Velbon Super Mag Slider manual macro rail, with some customization
    - Ultrathin 2-Phase 4-Wire 42 Stepper Motor 1.8 Degree 0.7A (http://www.ebay.ca/itm/Ultrathin-2-Phase-4-Wire-42-Stepper-Motor-1-8-Degree-0-7A-e-/261826922341?pt=LH_DefaultDomain_0&hash=item3cf619c765 )
    - Arduino Uno R3
    - EasyDriver stepping Stepper Motor Driver V4.4
    - 4x4 keys keypad (http://www.ebay.ca/itm/4x4-Matrix-high-quality-Keyboard-Keypad-Use-Key-PIC-AVR-Stamp-Sml-/141687830020?pt=LH_DefaultDomain_0&hash=item20fd40b604 )
    - Nokia 5110 LCD display + four 10 kOhm resistors + 1 kOhm + 330 Ohm (https://learn.sparkfun.com/tutorials/graphic-lcd-hookup-guide)
    - 5V Relay SIP-1A05 + 1N4004 diode + 33 Ohm resistor; to operate camera shutter (http://www.forward.com.au/pfod/HomeAutomation/OnOffAddRelay/index.html)
    - Voltage divider for battery sensor: resistors 270k, 360k, capacitor 0.1 uF.
    New in h1.1: extra 10k resistor.
    New in h1.2: extra SIP-1A05 relay, 1N4004 diode, 0.1 uF capacitor, 33 Ohm and 47 k resistors.
    New in h1.3: motor driver upgrade EasyDriver -> BigEasyDriver (16 microsteps/step), more powerful motor (1.3A/coil). Added telescope focuser module (second identical stepper motor
       plus a voltage divider consisting of a thermistor and regular resistor with a similar resistance - both 10k in my case). Added 10uF capacitor for LCD backlighting (to fix the
       LCD instability due to PWM ripples in backlighting control).
    h2.0: complete rehaul of the hardware and software: ESP8266, MCP23S17, ST7735 TFT, DRV8825, optocouplers.

   I am using the following libraries:

    - TFT_eSPI - fast library for ST7735 TFT display (https://github.com/Bodmer/TFT_eSPI); copy the customized file User_Setup.h to 
      the library's folder
    - MCP23S17 Class for Arduino (included, customized; https://playground.arduino.cc/Main/MCP23S17/)
    - Keypad library (included, customized): http://playground.arduino.cc/Code/KeypadTdu


   Hardware revisions [software versions supported]:

   h1.0 [s0.08]: Original public release design.

   h1.1 [s0.10, s0.12, s0.14, s0.08a]: Second row keypad pin moved from 10 to 7. Pin 10 left free (for hardware SPI). Display's pin SCE (CE / chip select) disconnected from pin 7.
                Instead, display SCE pin is soldered to the ground via 10k (pulldown) resistor.
   h1.2 [s1.00 and newer]: LCD reset pin (RST) disconnected from Arduino; instead it is now hardware controlled via RC delay circuit (R=47k, C=0.1uF, connected to VCC=+3.3V).
                  Arduino pin 6 is now used to control the second relay (+ diod + R=33 Ohm), for camera autofocus.
   h1.3 [s1.18 and up]: Upgraded EasyDriver to BigEasyDriver. Swapped pins A3-5 with 0-2, and 6 with A2. Increased N_MICROSTEPS to 16. Added thermometer (10k resistor + 10k thermistor).
                  Using Hall sensor instead of micro switch in telescope mode.
   h2.0 [s2.00 and up]: complete rehaul of the hardware and software: ESP8266, MCP23S17, ST7735 TFT, DRV8825, optocouplers. New motion algorithm (prediction-correction).
                  Removed the telescope mode. Removed backlight stuff.
*/
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <math.h>
#include <limits.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "Keypad.h" // #include  "MCP23S17.h" is called there
#include "stacker.h"
#include "stdio.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void setup() {
  // Should be the first line in setup():
  g.setup_flag = 1;

  // Completely disabling WiFi:
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
//  ESP.deepSleep(1000, WAKE_RF_DISABLED);
  
  EEPROM.begin(ADDR_END); // Initializing EEPROM
  
  iochip.begin(); // Initializing the port expander
  iochip.pinMode    (IO_MODE);
  iochip.pullupMode (IO_PULLUP);
  iochip.inputInvert(0B0000000000000000); // Likely not needed
  // Setting the requested microstepping mode:
  iochip.digitalWrite(EPIN_M0, MOTOR_M0);
  iochip.digitalWrite(EPIN_M1, MOTOR_M1);
  iochip.digitalWrite(EPIN_M2, MOTOR_M2);

#ifdef BUZZER
  iochip.pinMode(EPIN_BUZZ, OUTPUT);
  g.buzz_state = LOW;
  iochip.digitalWrite(EPIN_BUZZ, g.buzz_state);
  g.t_buzz = micros();
#endif

#ifdef SER_DEBUG
  Serial.begin(115200);
#endif

#ifdef TEST_SWITCH
#ifdef SERIAL_SWITCH
  Serial.begin(9600);
#endif
#endif

  // Setting pins for motor Driver to OUTPUT:
#ifndef DISABLE_MOTOR
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_STEP, OUTPUT);
#endif
  g.enable_flag = HIGH;
  iochip.digitalWrite(EPIN_ENABLE, g.enable_flag);
  pinMode(PIN_LIMITERS, INPUT_PULLUP);

  g.error = 0;

// Initializing the display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextWrap(true);
  tft.setTextFont(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  


  // Checking if EEPROM was never used:
  if (EEPROM.read(0) == 255 && EEPROM.read(1) == 255)
  {
    // Initializing with a factory reset (setting EEPROM values to the initial state):
    initialize(1);
  }
  else
  {
    // Initializing, with EEPROM data read from the device
    initialize(0);
  }

  // This should not be done in initialize():
  keypad.key[0].kstate = (KeyState)0;
  keypad.key[1].kstate = (KeyState)0;

  // Should be the last line in setup:
  g.setup_flag = 0;

/*
int tmp1, tmp2;
char buf21[21];
sprintf(buf21, "                    ");
g.stacker_mode = 1;
tft.fillScreen(TFT_WHITE);
  
for (int i=0; i<15; i++)
{
  for(g.frame_counter=1; g.frame_counter<590; g.frame_counter+=588)
  {
tmp1 = micros();
//display_frame_counter();
//  tft.fillScreen(TFT_BLACK);
for (int j=0; j<6; j++)
{
  my_setCursor(0, j, 1);
  tft.print(buf21);
}

tmp2 = micros();
Serial.println(tmp2-tmp1);
delay(1000);
  }
}
delay(10000);
*/
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void loop()
{
#ifdef TEST_SWITCH
  test_switch();
#endif

  // Performing backlash compensation after bad direction moves:
  backlash();

#ifdef BUZZER
  buzzer();
#endif  

  // Display related regular activities:
  display_stuff();

  // Processing the keypad:
  process_keypad();

  // All the processing related to the two extreme limits for the macro rail movements:
  limiters();

  // Perform calibration of the limiters if requested (only when the rail is at rest):
  calibration();

  // Camera control:
  camera();

  // Issuing write to stepper motor driver pins if/when needed:
  motor_control();

  // Cleaning up at the end of each loop:
  cleanup();
  
}


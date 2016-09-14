/* Sergey Mashchenko 2015, 2016

   Fast Stacker: an automated macro rail for focus stacking

   Online tutorial: http://pulsar124.wikia.com/wiki/Fast_Stacker

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
       plus a voltage divider consisting of a thermistor and regular resistor with a similar resistance - both 10k in my case).

   I am using the following libraries:

    - pcd8544 (for Nokia 5110): https://github.com/snigelen/pcd8544
    - Keypad library: http://playground.arduino.cc/Code/KeypadT

   Since s0.10, I customized the above libraries, and provide the custom versions with these package. (No need to install the libraries.)

   Hardware revisions [software versions supported]:

   h1.0 [s0.08]: Original public release design.

   h1.1 [s0.10, s0.12, s0.14, s0.08a]: Second row keypad pin moved from 10 to 7. Pin 10 left free (for hardware SPI). Display's pin SCE (CE / chip select) disconnected from pin 7.
                Instead, display SCE pin is soldered to the ground via 10k (pulldown) resistor.
   h1.2 [s1.00 and newer]: LCD reset pin (RST) disconnected from Arduino; instead it is now hardware controlled via RC delay circuit (R=47k, C=0.1uF, connected to VCC=+3.3V).
                  Arduino pin 6 is now used to control the second relay (+ diod + R=33 Ohm), for camera autofocus.
   h1.3 [s1.18 and up]: Upgraded EasyDriver to BigEasyDriver. Swapped pins A3-5 with 0-2, and 6 with A2. Increased N_MICROSTEPS to 16. Added thermometer (10k resistor + 10k thermistor).
*/
#include <EEPROM.h>
#include <math.h>
#include <SPI.h>
#include "Keypad.h"
#include "pcd8544.h"
#include "stacker.h"
#include "stdio.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void setup() {
  // Should be the first line in setup():
  g.setup_flag = 1;

#ifdef DUMP_REGS
  Serial.begin(9600);
  Serial.setTimeout(5000);
  // Dumping the old reg values from EEPROM to serial monitor:
  dump_regs();
//  Serial.println("Dump is done. Send new regs data if needed.");
  // Optionally reading the new values from the serial monitor and updating EEPROM:
  write_regs();
//  Serial.println("New regs values in EEPROM:");
  // Sending the new EEPROM values to serial monitor:
  dump_regs();
  Serial.end();
  return;
#endif

  // Changing the frequency of PWM signal driving backlighting (trying to address the LCD instability at lower brightness levels):
  //  TCCR1B = TCCR1B & 0b11111000 | 0x01;

  // Setting pins for BigEasyDriver to OUTPUT:
#ifndef DISABLE_MOTOR
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_STEP, OUTPUT);
#endif
  pinMode(PIN_ENABLE, OUTPUT);
  digitalWrite(PIN_ENABLE, HIGH);
  pinMode(PIN_LIMITERS, INPUT_PULLUP);

  // Temporarily borrowing the AF pin to check if we are connected to the telescope or macro rail.
  pinMode(PIN_AF, INPUT_PULLUP);
  // Dynamically detecting whether we are connected to the macro rail
  int raw = analogRead(PIN_AF);
  // If the resistance is low we are grounded via 500 Ohm relay -> it is macro rail;
  // if it's high, we are grounded via thermistor (~10k or higher) -> telescope mode:
  g.telescope = (raw > 100);

  // In macro mode, this will operate the shutter relay; in telescope mode, this will provide constant +5V to the voltage divider with thermistor
  // (to measure temperature)
  pinMode(PIN_SHUTTER, OUTPUT);

#ifdef TEMPERATURE
  if (g.telescope)
  {
    // In telescope mode, PIN_AF will be used to measure the temperature on the telescope
    //    pinMode(PIN_AF, INPUT_PULLUP);
    // Without pullup, as for now I am using an external pullup resistor:
    pinMode(PIN_AF, INPUT);
  }
#else
  pinMode(PIN_AF, OUTPUT);
#endif

  pinMode(PIN_LCD_LED, OUTPUT);

#ifndef SOFTWARE_SPI
  // My Nokia 5110 didn't work in SPI mode until I added this line (reference: http://forum.arduino.cc/index.php?topic=164108.0)
  // Some LCD's don't work with this settings (empty screen) - try to change the constant to SPI_CLOCK_DIV16 if this is the case
  SPI.setClockDivider(SPI_CLOCK_DIV8);
#endif
  lcd.begin();  // Always call lcd.begin() first.

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

/*
#ifdef ROUND_OFF
  float fsteps;
  // Rounding off small values of MM_PER_FRAME to the nearest whole number of microsteps:
  float scale = MM_PER_FRAME[0] / g.mm_per_microstep;
  for (int i = 0; i < N_PARAMS; i++)
  {
    fsteps = MM_PER_FRAME[i] / g.mm_per_microstep;
    if (g.telescope)
      // Rescaling the MM_PER_FRAME table in telescope mode so the smallest value = one microstep:
      fsteps = fsteps / scale;
    short steps = (short)nintMy(fsteps);
    if (g.telescope || steps < 20)
      MM_PER_FRAME[i] = ((float)steps) * g.mm_per_microstep;
  }
#endif
*/

  // Should be the last line in setup:
  g.setup_flag = 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void loop()
{
#ifndef DUMP_REGS

  // Performing backlash compensation after bad direction moves:
  backlash();

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

#ifdef TIMING
  timing();
#endif
#endif // DUMP_REGS
}


/* Sergey Mashchenko 2015-2021

   User header file. Contains user adjustable parameters (constants), and other stuff.

*/

#ifndef STACKER_H
#define STACKER_H
#include <TFT_eSPI.h>

// Requires hardware version h2.0
#define VERSION "2.03"

//++++++++++ Major features +++++++++++++++++
#define BUZZER
#define BUZZER_PASSIVE  // If you are using a passive buzzer (requires explicit PWM signal)

//+++++++++++++ Data types +++++++++++++++++++
// Using both real64 and LONG_TIME (int64 for all timings) surprisingly adds very little to the Arduino loop timing - during motion,
// the average loop interval / std grow from 11.33 / 9.03 us (all 32 bit) to 12.54 / 9.16 us (all 64 bit). These are all well below
// the maximum allowed interval of 62 us (time between motor step commands when moving at the maximum speed - 10 mm/s). Using
// 64-bit data is crucial for the accuracy and stability of very long (> 10 minutes) moves, like in 2-point continuous stacking.
#define FLOAT_TYPE real64
// If defined, time will use 64-bit integers
#define LONG_TIME
// Integer type for all coordinates (cannot be an unsigned type!). Use "short" if the total number of microsteps for your rail is <32,000,
// and use "long" for larger numbers (will consume more memory)
#define COORD_TYPE s32
#ifdef LONG_TIME
// Long unsigned type (for times and such):
#define TIME_UTYPE uint64
// Long signed type (for time differences and such):
#define TIME_STYPE sint64
#else
// Long unsigned type (for times and such):
#define TIME_UTYPE uint32
// Long signed type (for time differences and such):
#define TIME_STYPE s32
#endif

//////// Debugging options ////////
// Uncomment this line to measure the BACKLASH parameter for your rail (you don't need this if you are using Velbon Super Mag Slider - just use my value of BACKLASH)
// Only positive values are accepted. Make sure backlash compensations works against the gravity!
// When BL_DEBUG is defined, key "5" becomes "eidit BACKLASH" function. Once proper value for BACKLASH is determined, copy it back in the code (below)
// Don't use BL_DEBUG together with either BL2_DEBUG or DELAY_DEBUG!
//#define BL_DEBUG
// Uncomment this line to measure the BACKLASH_2 parameter for your rail (you don't need this if you are using Velbon Super Mag Slider - just use my value of BACKLASH_2)
// When BL2_DEBUG is defined, key "5" become "edit BACKLASH_2" function. Once proper value for BACKLASH_2 is determined, copy it back in the code (below)
// Don't use BL2_DEBUG together with either BL_DEBUG or DELAY_DEBUG!
//#define BL2_DEBUG
// Uncomment this line to measure SHUTTER_ON_DELAY2 (electronic shutter for Canon DSLRs; when mirror_lock=2).
// When DELAY_DEBUG is defined, key "5" becomes "edit SHUTTER_ON_DELAY2" function. Once proper value is determined, copy it back in the code (below)
// Don't use DELAY_DEBUG together with either BL_DEBUG or BL2_DEBUG!
//#define DELAY_DEBUG

// If defined, no display updates when moving
//#define NO_DISP
// For timing the main loop:
//#define TIMING
// Motor debugging mode: limiters disabled (used for finetuning the motor alignment with the macro rail knob, finding the minimum motor current,
// and software debugging without the motor unit)
//#define MOTOR_DEBUG
// Uncomment this line when debugging the control unit without the motor unit:
//#define DISABLE_MOTOR
// Battery debugging mode (prints actual raw voltage sensor value)
//#define BATTERY_DEBUG
// If defined, disables critically low voltage action:
#define NO_CRITICAL_VOLTAGE
// If defined, do camera debugging:
//#define CAMERA_DEBUG
// Uncomment to disable shutter triggering:
//#define DISABLE_SHUTTER
// Uncomment to display the amount of used EEPROM in "*" screen (bottom line)
//#define SHOW_EEPROM
// Display positions and temperature in raw units:
//#define SHOW_RAW
// If defined, macro rail will be used to test the accuracy of the foreground switch (repeatedly triggering it and measuring the spread of trigger positions)
//#define TEST_SWITCH
// If defined, use serial monitor to receive switch test data (only in TEST_SWITCH mode):
//#define SERIAL_SWITCH
//#define TEST_LIMITER // If defined, displays limiter state after the coordinate
//#define SER_DEBUG  // Debugging motion algorithms using serial interface
//#define SER_DEBUG_TIME

// Port expander initial state:
// For MCP23S17 expander; ports numbering is 16, 15, ..., 1 (B7, B6, ..., A7, ..., A0):
s16 IO_MODE =   0B1000000011111111;  // 1: input; 0: output
s16 IO_PULLUP = 0B1000000011111111;

//////// Camera related parameters: ////////
// Delay between triggering AF on and starting shooting in continuous stacking mode; microseconds
// (If your continuous focus stacking skips the very first shot, increase this parameter)
const TIME_STYPE CONT_STACKING_DELAY = 100000;  // 100000
const TIME_STYPE SHUTTER_TIME_US = 100000; // Time to keep the shutter button pressed (us) 100000
const TIME_STYPE SHUTTER_ON_DELAY = 5000; // Delay in microseconds between setting AF on and shutter on  5000
const TIME_STYPE SHUTTER_OFF_DELAY = 5000; // Delay in microseconds between setting shutter off and AF off  5000
// The mode of AF synching with the shutter (Canon 50D: 0; Canon 6D: 1):
//  0: AF is synched with shutter (when shutter is on AF is on; when shutter is off AF is off) only
//      for non-continuous stacking (#0); during continuous stacking, AF is permanently on (this can increase the maximum FPS your camera can yield);
//  1: AF is always synched with shutter, even for continuous stacking. Use this feature only if your camera requires it.
const short AF_SYNC = 1;
// When DELAY_DEBUG is defined, key "5" becomes "edit SHUTTER_ON_DELAY2" function. Once proper value is determined, copy it back in the code (below)
#ifdef DELAY_DEBUG
// Initial values for the two electronic shutter delays during delay debugging:
TIME_STYPE SHUTTER_ON_DELAY2 = 500000;
#else
// The ON and OFF delays used only for mirror_lock=2 (Full Resolution Silent Picture - FRSP - for Canon with Magic Lantern firmware).
// For FRSP to work, the AF relay should be connected as usual (to the AF camera circuit), but the shutter relay should operate the external flash
// (and should be dosconnected from the camera's shutter).
// SHUTTER_ON_DELAY2+SHUTTER_OFF_DELAY2+SHUTTER_TIME_US is the time the AF relay will be pressed on - this should be long enough for
// a silent picture to be taken (at least 0.8s for Canon 50D).
// SHUTTER_ON_DELAY2 is the delay between initiating a silent picture (by pressing AF) and the external flash actuation. It should be just
// right (not too short, not too long; 1.1s works for Canon 50D) for the flash being triggered during the electronic shutter exposure.
// The camera exposure also should be long enough (at least 0.25s for Canon 50D) to capture the flash.
// FRSP should only be used with non-continuous stacking, with DELAY1+DELAY2 long enough for multiple silent pictures to be taken successfully.
// (For Canon 50D at least 5.5s: DELAY1=4s, DELAY2=1.5s)
const TIME_STYPE SHUTTER_ON_DELAY2 = 500000; //  1100000 for 50D, 500000 for 6D/MLV (4/1.5s delays; 1/4s exposure; ExpOverride ON, ExpSim ON)
#endif
const TIME_STYPE SHUTTER_OFF_DELAY2 = 100000; // 100000

//////// Pin assignment ////////
// SPI 16 GPIOs port expander MCP23S17, pin CS:
//#define PIN_MCP_CS D3  // should be defined in Keypad.cpp
// Stepper driver (DRV8825)
// Can only use native pins for stepping and direction, as the expander pins are much slower:
const byte PIN_STEP = D1;
const byte PIN_DIR = D2;
// LOW: enable motor; HIGH: disable motor (to save energy):
const byte EPIN_ENABLE = 9; // Expander B3
// Microstepping control:
const byte EPIN_M0 = 10; // Expander B0
const byte EPIN_M1 = 11; // Expander B1
const byte EPIN_M2 = 12; // Expander B2
// -----------  Display pins  --------------------------------------
// The actual display pin assignment is done inside User_Setup.h file (part of the TFT_eSPI library)
//const byte TFT_CS = D0;
//#define TFT_RST  -1 // Connect to RST pin of D1 Mini
//const byte TFT_DC = D4;
// Pin to read digital input from the two limiting switches (normally LOW; HIGH when limiters are triggered)
// Make sure to solder a 3-10 nF capacitor between this pin and the ground pin, on the microcontroller, to reduce reading noise from the cable
const byte PIN_LIMITERS = D8; // Native GPIO is much better for limiters vs port expander, as it's much faster (0.34 vs 7.9 us)
// Pin to trigger camera shutter:
const byte EPIN_SHUTTER = 13;  // Green LED; expander B4
const byte EPIN_AF = 14;  // Red LED; expander B5
// Analogue pin for the battery life sensor:
#define PIN_BATTERY A0
#ifdef BUZZER
const byte EPIN_BUZZ = 15; // expander B6
#endif

//////// Voltage parameters: ////////
// Scaling coefficient to derive the battery voltage (depends on the resistance of the two dividing resistors, R3 and R4.
// Assuming R3 is the one directly connected to "+" of the battery, the scaler is (R3+R4)/R4. R3+R4 should be ~0.5M)
// To reduce reading noise, a 0.1uF capacitor can be soldered parallel to R4.
// The simplest way is to measure it: define DEBUG_VOLTAGE above (this will display raw voltage measurement),
// then compute VOLTASGE_SCALER as actual_voltage (in volts) / raw_voltage / 8 (so it's per AA battery)
const float VOLTAGE_SCALER = 10.72 / 450.0 / 8.0;
// Critically low voltage, per AA battery (when V becomes lower than this, the macro rail is disabled)
// Set it slightly above the value when the rail with camera starts skipping steps
const float V_LOW = 0.9;
// Highest voltage from a freshly charged AA battery:
const float V_HIGH = 1.4;
// Threshold voltage (Volts) between battery and AC operations (divided by 8 - so it's "per battery" value)
const float V_THRESHOLD = 11.3 / 8;
// Wait this long (us) after booting to determine whether we are battery or AC powered (for Auto energy save operation)
const TIME_STYPE AC_DELAY = 500000;


//////// Keypad stuff: ////////
const byte rows = 4; //four rows
const byte cols = 4; //three columns
char keys[rows][cols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
// Pins are for the port expander MCP23S17 (the whole portA):
byte rowPins[rows] = {5, 6, 7, 8}; //connect to the row pinouts of the keypad
byte colPins[cols] = {1, 2, 3, 4}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );
const TIME_STYPE KEY_DELAY = 1000000; // Delay for activating certain keys (4, B), us


//////// Parameters related to the motor and the rail: ////////
// Number of full steps per rotation for the stepper motor:
const COORD_TYPE MOTOR_STEPS = 200;
// M0, M1, M2 values determine the number of microsteps per step (see DRV8825 specs; all three HIGH correspond to 32 microsteps)
const byte MOTOR_M0 = HIGH;
const byte MOTOR_M1 = HIGH;
const byte MOTOR_M2 = HIGH;
// Number of microsteps in a step (corresponding to the above M0, M1, M2 values):
const COORD_TYPE N_MICROSTEPS = 32;
// Macro rail parameter: travel distance per one rotation, in mm (3.98mm for Velbon Mag Slider):
const FLOAT_TYPE MM_PER_ROTATION = 3.98;
// Backlash compensation (in mm); positive direction (towards background) is assumed to be the good one (no BL compensation required);
// all motions moving in the bad (negative) direction at the end will need some BL compensation.
// v2.0: Now one can designate either negative or positive directions as bad ones (*B keys)
// Using the simplest BL model (assumption: rail physically doesn't move until rewinding the full g.backlash amount,
// and then instantly starts moving; same when moving to the positive direction after moving to the bad direction).
// The algorithm guarantees that every time the rail comes to rest, it is fully BL compensated (so the code coordinate = physical coordinate).
// Should be determined experimentally: too small values will produce visible backlash (two or more frames at the start of the stacking
// sequence will look alsmost identical). For my Velbon Super Mag Slide rail I measured the BL to be ~0.2 mm.
// Set it to zero to disable BL compensation.
const FLOAT_TYPE BACKLASH_MM = 0.2; // 0.2mm for Velbon Super Mag Slider
// This is the second backlash related parameter you need to measure for you rail (or just use the value provided if your rail is Velbon Super Mag Slider)
// This parameter is only relevant for one operation - rail reversal (*1 function). Unlike the above parameter (BACKLASH_MM) which can be equal to or
// larger than the actual backlash value for the rail movements to be perfectly accurate, the BACKLASH_2 parameter has to have a specific value (not larger, no smaller); if
// your rail backlash changes as a function of the rail angle, position on the rail, camera weight etc., BACKLASH_2 would have to change as well.
// Because it is not practical, your value of BACKLASH_2 should be a compromise, giving reasonable results under normal usage scenario. In any case
// rail reversal (*1) wasn't meant to be a perfectly accurate operation, whereas the standard backlash compensation is.
// Use the BL2_DEBUG mode to find the good value of this parameter. You should convert the displayed value of BL2_DEBUG from microsteps
// to mm, by multiplying by MM_PER_ROTATION/(MOTOR_STEPS*N_MICROSTEPS).
// Adjust this parameter only after you found a good value for BACKLASH_MM parameter.
const FLOAT_TYPE BACKLASH_2_MM = 0.3333; // 0.3333mm for Velbom Super Mag Slider
// Speed limiter, in mm/s. Higher values will result in lower torques and will necessitate larger travel distance
// between the limiting switches and the physical limits of the rail. In addition, too high values will result
// in Arduino loop becoming longer than inter-step time interval, which effectively will limit your maxium speed.
// For an arbitrary rail and motor, make sure the following condition is met:
// 10^6 * MM_PER_ROTATION / (MOTOR_STEPS * N_MICROSTEPS * SPEED_LIMIT_MM_S) > typical Arduino loop timing (us)
// Macro rail speed limit:
const FLOAT_TYPE SPEED_LIMIT_MM_S = 10; // 10
// Breaking distance (mm) for the rail when stopping while moving at the fastest speed (SPEED_LIMIT)
// This will determine the maximum acceleration/deceleration allowed for any rail movements - important
// for reducing the damage to the (mostly plastic) rail gears. Make sure that this distance is smaller
// than the smaller distance of the two limiting switches (between the switch actuation and the physical rail limits)
const FLOAT_TYPE BREAKING_DISTANCE_MM = 1.0;
// Padding (in mm) for a soft limit, before hitting the limiters (increase if you constantly hit the limiter by accident)
const FLOAT_TYPE LIMITER_PAD_MM = 0.5;
// During calibration, after hitting the first limiter, breaking, and moving in the opposite direction,
// travel this many mm, before starting checking the limiter again (should be large enough that the limiter is guaranteed to go off by that point)
const FLOAT_TYPE DELTA_LIMITER_MM = 4.0;
// Final calibratio leg (after hitting limit1); should be long enogh for limiter1 to go off, but smaller than 0.5 of the rail length
const FLOAT_TYPE CALIBRATE_FINAL_LEG_MM = 4.0;
// Delay in microseconds between LOW and HIGH writes to PIN_STEP
// For DRV8825 stepper driver it should be at least 1.9 us. Form my measurements, setting STEP_LOW_DT to 2 us results
// in 2.8 us impulses, to 1 us - in 1.7 us impulses, so I choose to use 2 us:
const short STEP_LOW_DT = 2;
// Delay after writing to PIN_ENABLE, ms (only used in SAVE_ENERGY mode):
const short ENABLE_DELAY_MS = 3;
const FLOAT_TYPE OVERSHOOT = 0.5; // (0.0-1.0) In all moves, overshoot the target by these many microsteps (stop will happen at the accurate target position). To account for roundoff errors.
const COORD_TYPE HUGE = 1000000;  // Should be larger than the number of microsteps for the whole rail length


//////// User interface parameters: ////////
const TIME_STYPE COMMENT_DELAY = 2000000; // time in us to keep the comment line visible
const TIME_STYPE T_KEY_LAG = 500000; // time in us to keep a parameter change key pressed before it will start repeating
const TIME_STYPE T_KEY_REPEAT = 100000; // time interval in us for repeating with parameter change keys
const TIME_STYPE DISPLAY_REFRESH_TIME = 100000; // time interval in us for refreshing battery status
const TIME_STYPE KEY_DELAY_US = 500000; // Delay for keys (4,B)
const FLOAT_TYPE POS_SPEED_FRACTION = 0.1; // If speed is larger than this factor times MAXIMUM_SPEED, stop updating currently displayed position (to minimize noise and vibrations)

///// Editor related parameters //////
#define MAX_POS 10 // Maximum number of characters in the edited value
// Limits for editable parameters:
#define MSTEP_MIN 1
#define MSTEP_MAX 8000 // Make sure it's not larger than the length of the rail! 8000 -> 5mm
#define FPS_MIN 0.01
#define FPS_MAX 4.5 // Adjust to your camera's (and flash) maximum possible frame rate
#define FIRST_DELAY_MIN 0.5
#define FIRST_DELAY_MAX 8.0
#define SECOND_DELAY_MIN 0.5
#define SECOND_DELAY_MAX 8.0
#define N_SHOTS_MIN 2
#define N_SHOTS_MAX 600
#define N_TIMELAPSE_MIN 1
#define N_TIMELAPSE_MAX 999
#define DT_TIMELAPSE_MIN 0
#define DT_TIMELAPSE_MAX 9999

//////// INPUT PARAMETERS: ////////
// Number of custom memory registers:
const byte N_REGS = 5;
// Table of possible values for accel_factor parameter:
const byte N_ACCEL_FACTOR = 7;
const byte ACCEL_FACTOR[N_ACCEL_FACTOR] = {1, 2, 4, 8, 16, 32, 64};

// Buzzer stuff:
#ifdef BUZZER
const TIME_STYPE DT_BUZZ_US = 125; // Half-period for the buzzer sound, us; computed as 10^6/(2*freq_Hz) ; 125 for a 4kHz buzzer
const TIME_STYPE KEY_BEEP_US = 50000; // Delayed key beep length, us
const TIME_STYPE ACCIDENT_BEEP_US = 250000; // Accidental limiter triggering key beep length, us
#endif


//////////////////////////////////////////// Normally you shouldn't modify anything below this line ///////////////////////////////////////////////////

//////// LCD stuff: ////////
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
#define FONT_HEIGHT 10 // Font height in pixels
#define FONT_WIDTH 8 // Font width in pixels
#define TFT_NX 20 // Chars per line
#define TFT_NY 7 // Number of lines
#define TOP_GAP 1 // Empty top gap in pixels (same for bottom gap)
#define LEFT_GAP 4 // Empty left gap in pixels
#define LINE_GAP 9 // Empty gap between lines in pixels
#define DEL_BITMAP 3 // Offset for drawBitmap relative to print

// MM per microstep:
const FLOAT_TYPE MM_PER_MICROSTEP = MM_PER_ROTATION / ((FLOAT_TYPE)MOTOR_STEPS * (FLOAT_TYPE)N_MICROSTEPS);
// Number of microsteps per rotation
const COORD_TYPE MICROSTEPS_PER_ROTATION = MOTOR_STEPS * N_MICROSTEPS;
// Breaking distance in internal units (microsteps):
const FLOAT_TYPE BREAKING_DISTANCE = MICROSTEPS_PER_ROTATION * BREAKING_DISTANCE_MM / (1.0 * MM_PER_ROTATION);
const FLOAT_TYPE SPEED_SCALE = MICROSTEPS_PER_ROTATION / (1.0e6 * MM_PER_ROTATION); // Conversion factor from mm/s to usteps/usecond
// Speed limit in internal units (microsteps per microsecond):
const FLOAT_TYPE SPEED_LIMIT = SPEED_SCALE * SPEED_LIMIT_MM_S;
// Maximum acceleration/deceleration allowed, in microsteps per microseconds^2 (a float)
// (This is a limiter, to minimize damage to the rail and motor)
const FLOAT_TYPE ACCEL_LIMIT = SPEED_LIMIT * SPEED_LIMIT / (2.0 * BREAKING_DISTANCE);
// Backlash in microsteps (+0.5 for proper round-off):
const COORD_TYPE BACKLASH = (COORD_TYPE)(BACKLASH_MM / MM_PER_MICROSTEP + 0.5);
#ifdef BL2_DEBUG
// Initial value for BACKLASH_2:
COORD_TYPE BACKLASH_2 = (COORD_TYPE)(BACKLASH_2_MM / MM_PER_MICROSTEP + 0.5);
#else
// Backlash correction for rail reversal (*1) in microsteps:
const COORD_TYPE BACKLASH_2 = (COORD_TYPE)(BACKLASH_2_MM / MM_PER_MICROSTEP + 0.5);
#endif
// Maximum FPS possible (depends on various delay parameters above; the additional factor of 2000 us is to account for a few Arduino loops):
const FLOAT_TYPE MAXIMUM_FPS = 1e6 / (FLOAT_TYPE)(SHUTTER_TIME_US + SHUTTER_ON_DELAY + SHUTTER_OFF_DELAY + 2000);
const COORD_TYPE DELTA_LIMITER = (COORD_TYPE)(DELTA_LIMITER_MM / MM_PER_MICROSTEP + 0.5);
const COORD_TYPE LIMITER_PAD = (COORD_TYPE)(LIMITER_PAD_MM / MM_PER_MICROSTEP + 0.5);
const COORD_TYPE CALIBRATE_FINAL_LEG = (COORD_TYPE)(CALIBRATE_FINAL_LEG_MM / MM_PER_MICROSTEP + 0.5);

// Structure to have custom parameters saved to EEPROM
struct regist
{
  COORD_TYPE mstep; // Number of microsteps per frame
  FLOAT_TYPE fps; // frames per second for continuous mode
  int n_shots; // Number of shots in 1-point mode
  FLOAT_TYPE first_delay; // First delay in non-continuous mode, seconds
  FLOAT_TYPE second_delay; // Second delay in non-continuous mode, seconds
  byte i_mode; // counter for the current mode:
#define ONE_SHOT_MODE 0
#define CONT_MODE 1
#define NONCONT_MODE 2
  byte i_accel_factor; // Index for accel_factor (initial acceleration for REWIND and FASTFORWARD)
  byte i_accel_factor2; // Index for accel_factor2 (stopping acceleration, except when BREAK)
  int n_timelapse; // Number of passses in a timelapse sequence (set to 1 to disable timelapsing)
  FLOAT_TYPE dt_timelapse; // Time interval (seconds) between timelapse passes. If shorter than the length of one pass, passes will occur one after another without a gap
  byte mirror_lock; // See the posible values below:
#define MIRROR_OFF 0 // no mirror lock in non-continuous stacking
#define MIRROR_ON 1 // mirror lock is used in non-continuous stacking
#define MIRROR_FRSP 2 // Full Resolution Silet Picture (electronic shutter; only Canon DSLRs, with Magick Lantern firmware)
#define MIRROR_BURST 3  // Using the burst shooting mode of the camera, instead of initiating each shot (only for 1-point continuous mode)
  signed char backlash_on; // =1 when g.backlash=BACKLASH; =0 when g.backlash=0.0; =-1 when g.backlash=-BACKLASH
  byte straight;  // 0: reversed rail (PIN_DIR=LOW is positive); 1: straight rail (PIN_DIR=HIGH is positive)
  byte save_energy; // =0: always using the motor's torque, even when not moving (should improve accuracy and holding torque); =1: save energy (only use torque during movements)
  COORD_TYPE point[2];  // two memory points:
#define FOREGROUND 0
#define BACKGROUND 1
  byte buzzer; // 1: buzzer on; 0: buzzer off
};
// Add 1 (byte) if SIZE_REG is odd, to make the total regist size even (I suspect EEPROM wants data to have even number of bytes):
short SIZE_REG = sizeof(regist) + 1;

const short dA = sizeof(COORD_TYPE);

// EEPROM addresses: make sure they don't go beyong the ESP8266 EEPROM size of 4k!
const int ADDR_POS = 0;  // Current position (integer, 4 bytes)
const int ADDR_LIMIT2 = ADDR_POS + 4; // pos_int for the background limiter (4 bytes)
const int ADDR_REG1 = ADDR_LIMIT2 + dA;  // Start of default + N_REGS custom memory registers for macro mode
const int ADDR_END = ADDR_REG1 + (N_REGS + 1) * SIZE_REG;   // End of used EEPROM

// 2-char bitmaps to display the battery status; 5 levels: 0 for empty, 4 for full:
const uint8_t battery_char [][20] = {{
    0B00111111, 0B11111111,
    0B00100000, 0B00000001,
    0B00100000, 0B00000001,
    0B11100000, 0B00000001,
    0B11100000, 0B00000001,
    0B11100000, 0B00000001,
    0B11100000, 0B00000001,
    0B00100000, 0B00000001,
    0B00100000, 0B00000001,
    0B00111111, 0B11111111
  }, {
    0B00111111, 0B11111111,
    0B00100000, 0B00001111,
    0B00100000, 0B00001111,
    0B11100000, 0B00001111,
    0B11100000, 0B00001111,
    0B11100000, 0B00001111,
    0B11100000, 0B00001111,
    0B00100000, 0B00001111,
    0B00100000, 0B00001111,
    0B00111111, 0B11111111
  }, {
    0B00111111, 0B11111111,
    0B00100000, 0B01111111,
    0B00100000, 0B01111111,
    0B11100000, 0B01111111,
    0B11100000, 0B01111111,
    0B11100000, 0B01111111,
    0B11100000, 0B01111111,
    0B00100000, 0B01111111,
    0B00100000, 0B01111111,
    0B00111111, 0B11111111
  }, {
    0B00111111, 0B11111111,
    0B00100011, 0B11111111,
    0B00100011, 0B11111111,
    0B11100011, 0B11111111,
    0B11100011, 0B11111111,
    0B11100011, 0B11111111,
    0B11100011, 0B11111111,
    0B00100011, 0B11111111,
    0B00100011, 0B11111111,
    0B00111111, 0B11111111
  }, {
    0B00111111, 0B11111111,
    0B00111111, 0B11111111,
    0B00111111, 0B11111111,
    0B11111111, 0B11111111,
    0B11111111, 0B11111111,
    0B11111111, 0B11111111,
    0B11111111, 0B11111111,
    0B00111111, 0B11111111,
    0B00111111, 0B11111111,
    0B00111111, 0B11111111
  }
};
// AC power symbol:
const uint8_t AC_char[] = {
  0B00000000, 0B00000000,
  0B00000000, 0B00100000,
  0B00000011, 0B11100000,
  0B00000111, 0B11111110,
  0B11111111, 0B11100000,
  0B11111111, 0B11100000,
  0B00000111, 0B11111110,
  0B00000011, 0B11100000,
  0B00000000, 0B00100000,
  0B00000000, 0B00000000
};
// 8x10 bitmaps
const uint8_t rewind_char[] = {
  0B00001100, 0B00000000, 0B00000000,
  0B00011000, 0B00000000, 0B00000000,
  0B00110000, 0B00000000, 0B00000000,
  0B01100000, 0B00000000, 0B00000000,
  0B11111111, 0B11111111, 0B00000000,
  0B11111111, 0B11111111, 0B00000000,
  0B01100000, 0B00000000, 0B00000000,
  0B00110000, 0B00000000, 0B00000000,
  0B00011000, 0B00000000, 0B00000000,
  0B00001100, 0B00000000, 0B00000000
};
const uint8_t forward_char[] = {
  0B00000000, 0B00110000, 0B00000000,
  0B00000000, 0B00011000, 0B00000000,
  0B00000000, 0B00001100, 0B00000000,
  0B00000000, 0B00000110, 0B00000000,
  0B11111111, 0B11111111, 0B00000000,
  0B11111111, 0B11111111, 0B00000000,
  0B00000000, 0B00000110, 0B00000000,
  0B00000000, 0B00001100, 0B00000000,
  0B00000000, 0B00011000, 0B00000000,
  0B00000000, 0B00110000, 0B00000000
};
const uint8_t straight_char[] = {
  0B00001000, 0B00000000, 0B00000000,
  0B00001100, 0B00000000, 0B00000000,
  0B00001110, 0B00000000, 0B00000000,
  0B00001111, 0B00000000, 0B00000000,
  0B00001111, 0B10000000, 0B00000000,
  0B00001111, 0B10000000, 0B00000000,
  0B00001111, 0B00000000, 0B00000000,
  0B00001110, 0B00000000, 0B00000000,
  0B00001100, 0B00000000, 0B00000000,
  0B00001000, 0B00000000, 0B00000000
};
const uint8_t reverse_char[] = {
  0B00000000, 0B00010000, 0B00000000,
  0B00000000, 0B00110000, 0B00000000,
  0B00000000, 0B01110000, 0B00000000,
  0B00000000, 0B11110000, 0B00000000,
  0B00000001, 0B11110000, 0B00000000,
  0B00000001, 0B11110000, 0B00000000,
  0B00000000, 0B11110000, 0B00000000,
  0B00000000, 0B01110000, 0B00000000,
  0B00000000, 0B00110000, 0B00000000,
  0B00000000, 0B00010000, 0B00000000
};


// All global variables belong to one structure - global:
struct global
{
  // New vars in v2.0
  byte model_init; // 0: default. 1: a new model has just been requested (will be reset to 0 in motor_control, once the new model is generated)
  TIME_UTYPE model_t0; // Absolute (model) time for the first model point.
  COORD_TYPE model_ipos0; // The coordinate at the start of a movement.
  byte model_type; // Code for the current (or requested) model. Codes:
#define MODEL_NONE 0 // No model, no motion
#define MODEL_GOTO 1 // GoTo model, can only start from rest, cannot be interrupted by FF, REWIND, STOP. Needs model_speed and model_ipos1
#define MODEL_FF 2 // Fast-Forward model, ignored if current model is GOTO or BREAK. Uses intermediate acceleration, and maximum speed limit
#define MODEL_REWIND 3 // Rewind model, ignored if current model is GOTO or BREAK. Uses intermediate acceleration, and maximum speed limit
#define MODEL_STOP 4 // Decelerate until stopped, using intermediate acceleration. Can be interrupted by FF and REWIND
#define MODEL_BREAK 5 // Emergency breaking (hit a limiter etc). Decelerate until stopped, using maximum acceleration. Cannot be interrupted by anything
  FLOAT_TYPE model_speed_max; // Desired (maximum) speed for the next goto motion (always positive).
  COORD_TYPE model_ipos1; // Desired target position for the next move (goto, accelerate, or stop)
  byte Npoints; // Number of points in the model (2..5). Points correspond to times when acceleration or direction changes. (Normally do not coincide with steps.)
#define N_POINTS_MAX 5  // Largest possible value for Npoints
  FLOAT_TYPE model_accel[N_POINTS_MAX]; // Acceleration (signed) at each model point
  TIME_UTYPE model_time[N_POINTS_MAX]; // Model time for each model point (relative to the 0-th point)
  FLOAT_TYPE model_speed[N_POINTS_MAX]; // Model speed (signed)
  FLOAT_TYPE model_pos[N_POINTS_MAX]; // Model position (relative to the 0-th point) at each point
  byte model_ptype[N_POINTS_MAX]; // Model point type:
#define INIT_POINT 0  // Starting moving from rest; currently not used
#define ZERO_ACCEL_POINT 1  // Acceleration becomes zero
#define ACCEL_POINT 2  // Acceleration becomes non-zero; currently not used
#define STOP_POINT 3  // Final (stop) point; currently not used
#define DIR_CHANGE_POINT 4  // Changing direction point
  signed char model_dir[N_POINTS_MAX]; // Model direction (-1, 0, 1), in the sense of the desired g.direction values
  TIME_UTYPE t_next_step; // Absolute timing prediction for the next step
  COORD_TYPE ipos_next_step; // Absolute coordinate for the next step
  byte motion_status_code; // Used for displaying motion status. Possible values:
#define STATUS_NONE 0 // no motion
#define STATUS_REWIND 1
#define STATUS_REVERSE 2
#define STATUS_FORWARD 3
#define STATUS_STRAIGHT 4
  byte delayed_goto; // set to 1 when pausing focus stacking - a signal to execute goto inside camera() after the breaking is finished
  byte enable_flag; // Tracks down status of the motor enable pin: HIGH: disable motor, LOW: enable motor
  byte editing; // =1 when editing a value
  FLOAT_TYPE edited_value; // Edited value
  int edited_param; // Parameter which is being edited. Possible values:
#define PARAM_MSTEP 0 // Number of microsteps per frame
#define PARAM_FPS 1
#define PARAM_N_SHOTS 2 // Number of shots
#define PARAM_FIRST_DELAY 3
#define PARAM_SECOND_DELAY 4
#define PARAM_GOTO 5  // GoTo target coordinate
#define PARAM_ACCEL_FACTOR 6
#define PARAM_N_TIMELAPSE 7
#define PARAM_DT_TIMELAPSE 8
  byte cursor_pos; // Initial cursor position (when editing)
  signed char dot_pos; // Counting dots n the edited value
  char value[MAX_POS + 1]; // String to store the digits of the edited value
  TIME_UTYPE t_key_delay; // Time when a key with the delay function (4,B) was pressed
  byte key_delay_on; // 1: in the process of delaying a key; 0: otherwise
  COORD_TYPE ipos_raw; // Raw coordinate, used for parking
  signed char dir_raw; // Raw motor direction, for parking
  byte init_delayed_key; // =1 when we just pressed a delayed key (4, B)
  TIME_UTYPE t_delayed_key; // Time when a delayed key (4, B) was pressed
  byte help_mode; //=1: initialized the help screen mode
#define N_HELP_PAGES 10 // Number of help pages  
  short help_page; // help page (0...N_HELP_PAGES-1)
  signed char level_old; // the old value of the battery level (0...4)
  TIME_UTYPE t_init; // Controller initialization time
  COORD_TYPE ipos_printed; // The last printed value of the position
  byte refresh; // if 1, each display line gets refreshed
  //-----------------
  struct regist reg; // Custom parameters register
  int addr_reg[N_REGS + 1]; // The starting addresses of the EEPROM memory registers, including the default (0th) one
  // Variables used to communicate between modules:
  TIME_UTYPE t;  // Model time in us, measured at the beginning of motor_control() module
  byte moving;  // 0 for stopped, 1 when moving; can only be set to 0 in motor_control()
  FLOAT_TYPE accel_v[5]; // Five possible floating point values for acceleration
  FLOAT_TYPE accel_limit; // Maximum allowed acceleration
  FLOAT_TYPE accel_v2; // Acceleration for normal stopping (not breaking). Computed from g.reg.i_accel_factor2
  COORD_TYPE ipos;  // Current position (in microsteps).
  TIME_UTYPE t_key_pressed; // Last time when a key was pressed
  TIME_UTYPE t_last_repeat; // Last time when a key was repeated (for parameter change keys)
  int N_repeats; // Counter of key repeats
  TIME_UTYPE t_display; // time since the last display refresh (only when not moving)
  /* a flag for each leg of calibration:
    0: no calibration;
    1: initiating full calibration: moving towards switch 2 for its calibration, with maximum speed and acceleration;
    2: triggered limit2 and stopped, initiating move towards switch 1
    3: triggered limit1 and stopped, initiating move forward to calibrate limit1 on the first switch-off position
    4: moving forward to calibrate limit1 on the first switch-off position;
    5: end of calibration; updating coordinates;
  */
  byte calibrate_flag;
  COORD_TYPE limit1; // ipos for the foreground limiter (temporary value, only used when accidently triggering foreground switch; normally it's 0)
  COORD_TYPE limit2; // ipos for the background limiter; limit2 > limit1
  byte accident;  // =1 if we accidently triggered limit1; 0 otherwise
  byte limit_on; //  The last recorded state of the limiter switches
  byte uninterrupted;  // =1 disables checking for limits (hard and soft); used for emergency breaking and during calibration
  byte uninterrupted2;  // =1 disables checking for limits (hard and soft); used for recovering rail when it's confused (#D command)
  char key_old;  // peviously pressed key; used in keypad()
  COORD_TYPE starting_point; // The starting point in the focus stacking with two points
  COORD_TYPE destination_point; // The destination point in the focus stacking with two points
  byte stacker_mode;  // 0: default (rewind etc.); 1: pre-winding for focus stacking; 2: 2-point focus stacking; 3: single-point stacking; 4: waiting between stacks in a timelapse sequence
  short Nframes; // Number of frames for 2-point focus stacking
  short frame_counter; // Counter for shots
  COORD_TYPE ipos_to_shoot; // Position to shoot the next shot during focus stacking
  byte shutter_on; // flag for camera shutter state: 0/1 corresponds to off/on
  byte AF_on; // flag for camera AF state: 0/1 corresponds to off/on
  byte single_shot; // flag for a single shot (made with #7): =1 when the shot is in progress, 0 otherwise
  TIME_UTYPE t_shutter; // Time when the camera shutter was triggered
  TIME_UTYPE t_shutter_off; // Time when the camera shutter was switched off
  TIME_UTYPE t_AF; // Time when the camera AF was triggered
  signed char direction; // -1/1 for reverse/forward directions of moving (request to change direction)
  signed char dir; // -1/1 for reverse/forward directions of moving (the actual state of the motor)
  char buffer[21];  // char buffer to be used for lcd print; 1 more element than the lcd width (20)
  char empty_buffer[21];  // char buffer to be used to clear one row of the LCD; 1 more element than the lcd width (20)
  char buf_comment[21]; // Keeps a copy of the comment line
  TIME_UTYPE t_comment; // time when commment line was triggered
  byte comment_flag; // flag used to trigger the comment line briefly
  byte x0, y0;  // Display pixel coordinates, set in misc/my_setCursor
  byte error; // error code (no error if 0); 1: initial limiter on or cable disconnected; 2: battery drained; non-zero value will disable the rail (with some exceptions)
  COORD_TYPE coords_change; // if >0, coordinates have to change (because we hit limit1, so we should set limit1=0 at some point)
  byte start_stacking; // =1 if we just initiated focus stacking, =2 when AF is triggered initially, =3 after CONT_STACKING_DELAY delay in continuous mode, =0 when no stacking
  byte make_shot; // =1 if we just initiated a shot; 0 otherwise
  TIME_UTYPE t_shot; // the time shot was initiated
  TIME_UTYPE t0_stacking; // time when stacking was initiated;
  //Pause state:
  // 0: no pause
  // 1: when 2-point stacking was paused
  // 2: pause which happened during the initial travel to the starting point after hitting any key
  // 3: pause which happened between stacks (in timelapse mode)
  byte paused;
  COORD_TYPE BL_counter; // Counting microsteps made in the bad direction. Possible values 0...BACKLASH. Each step in the good (+) direction decreases it by 1.
  byte Backlashing; // A flag to ensure that backlash compensation is uniterrupted (except for emergency breaking, #B); =1 when BL compensation is being done, 0 otherwise
  byte continuous_mode; // 2-point stacking mode: =0 for a non-continuous mode, =1 for a continuous mode
  byte noncont_flag; // flag for non-continuous mode of stacking; 0: no stacking; 1: initiated; 2: first shutter trigger; 3: second shutter; 4: go to the next frame
  byte setup_flag; // Flag used to detect if we are in the setup section (then the value is 1; otherwise 0)
  byte alt_flag; // 0: normal display; 1: alternative display
  byte alt_kind; // The kind of alternative display: 1: *
  char tmp_char;
  byte Backlash_init; // 1: initializing a full backlash loop; 2: initializing a rail reverse
  char buf6[6]; // Buffer to store the stacking length for displaying
  char buf10[10];
  short timelapse_counter; // Counter for the time lapse feature
  TIME_UTYPE t_mil; // millisecond accuracy timer; used to set up timelapse stacks
  TIME_UTYPE t0_mil; // millisecond accuracy timer; used to set up timelapse stacks
  byte end_of_stacking; // =1 when we are done with stacking (might still be moving, in continuoius mode)
  byte timelapse_mode; // =1 during timelapse mode, 0 otherwise
  COORD_TYPE backlash; // current value of backlash in microsteps (can be either 0 or BACKLASH)
  byte point1; // First point for stacking (either FOREGROUND or BACKGROUND, depending on the backlash direction)
  byte point2; // Second point for stacking (either BACKGROUND or FOREGROUND, depending on the backlash direction)
  //  int limiter_counter; // Used in impulse noise suppression inside Read_limiters()
#ifdef BUZZER
  TIME_UTYPE t_buzz; // timer for the buzzer
  byte buzz_state; // HIGH or LOW for the buzzer state
#endif
  TIME_STYPE dt_lost;
#ifdef TIMING
  TIME_UTYPE t_prev;
  TIME_UTYPE i1_timing;
  TIME_UTYPE t1_timing;
  TIME_UTYPE t2_timing;
  FLOAT_TYPE d_sum;
  FLOAT_TYPE d2_sum;
  byte moving_old;
#endif
  signed char current_point; // The index of the currently loaded memory point. Can be 0/1 for fore/background (macro mode). -1 means no point has been loaded/saved yet.
#ifdef TEST_SWITCH
  // Number of tests to perform:
#define TEST_N_MAX 50
  FLOAT_TYPE speed_test;
  short test_flag;
  short on_init;
  FLOAT_TYPE test_sum[2];
  FLOAT_TYPE test_sum2[2];
  short test_N;
  FLOAT_TYPE test_pos0[2];
  FLOAT_TYPE delta_min[2];
  FLOAT_TYPE delta_max[2];
  FLOAT_TYPE test_dev[2];
  FLOAT_TYPE test_avr[2];
  FLOAT_TYPE test_std[2];
  int count[2];
  byte test_limit_on[2];
  FLOAT_TYPE pos_tmp;
  FLOAT_TYPE pos_tmp2;
  FLOAT_TYPE test_limit;
  char buf9[10];
  COORD_TYPE pos0_test;
#endif
#ifdef BUZZER
  TIME_STYPE dt1_buzz_us = 1000; // Current half-period for the buzzer sound, us
  TIME_UTYPE t_beep; // time when beep was initiated
  TIME_STYPE beep_length; // Length of beep, us
  byte beep_on; // 1: beeping
  byte accident_buzzer; // =1 when a limiter is accidently triggered
#endif
#ifdef TEST_LIMITER
  int limiter_i; // counter for the false limiter readings
  byte limiter_ini; // initial limiter state
#endif
#ifdef LONG_TIME
  uint32 t_curr;
  uint32 t_old;  
  uint64 overflow_correction;
#endif
#ifdef SER_DEBUG_TIME    
  uint64 i_debug;
#endif  
};

struct global g;

#endif


/* Sergey Mashchenko 2015-2021

   User header file. Contains user adjustable parameters (constants), and other stuff.

   To be used with automated macro rail for focus stacking, and (since h1.3) for a telescope focuser
*/

#ifndef STACKER_H
#define STACKER_H
#include <TFT_eSPI.h>

// Requires hardware version h2.0
#define VERSION "2.0"

//++++++++++ Major features +++++++++++++++++
// Use temperature sensor (only in telescope mode), to maintain accurate focus at different temperatures:
//#define TEMPERATURE
/* Use one (foreground) microswitch in telescope mode.
   If undefined, you always have to manually move the focuser to the closest to the telescope position before powering up the controller.
   If defined, the focuser will automatically self-calibrate: first it will move away from the telescope until the switch is off and more than one breaking distance away from the switch;
   next, it will move full speed towards the telescope until the switch is triggered, at which point the emergency breaking will be engaged.
   Finally, it will move away from the telescope until the switch is off again (which sets 0 for the coordinate) + some safety margin. This procedure ensures that regardless of the initial focuser position it will
   always hit the switch at the same (maximum) speed, which should improve the switch accuracy (repeatability).
*/
//#define TELE_SWITCH
// If defined, inverts the limiter switch logic (HIGH when triggered; LOW otherwise). Needed only in telescope mode if you use Hall effect sensor as the limiting switch
//#define HALL_SENSOR
//#define BUZZER

//+++++++++++++ Data types +++++++++++++++++++
// Integer type for all coordinates (cannot be an unsigned type!). Use "short" if the total number of microsteps for your rail is <32,000,
// and use "long" for larger numbers (will consume more memory)
#define COORD_TYPE s32
// Signed shorter type (only used for telescope):
#define COORD_STYPE s16
// Long signed type (for timers and such):
#define TIME_TYPE s32

//////// Debugging options ////////
// If defined, no display updates when moving
//#define NO_DISP
// For timing the main loop:
//#define TIMING
// Motor debugging mode: limiters disabled (used for finetuning the motor alignment with the macro rail knob, finding the minimum motor current,
// and software debugging without the motor unit)
//#define MOTOR_DEBUG
// Uncomment this line when debugging the control unit without the motor unit:
//#define DISABLE_MOTOR
// Battery debugging mode (prints actual voltage per AA battery in the status line; needed to determine the lowest voltage parameter, V_LOW - see below)
//#define BATTERY_DEBUG
// If defined, disables critically low voltage action:
#define NO_CRITICAL_VOLTAGE
// If defined, debug buzzer (find the resonance frequency):
// two keys get reassigned: keys "5" and "6" (change frequency)
//#define BUZZER_DEBUG
//#define DELTA_BUZZ_US  1 // Steps in us for changing buzzer timing when doing buzzer debugging
// If defined, do camera debugging:
//#define CAMERA_DEBUG
// Uncomment this line to measure the BACKLASH parameter for your rail (you don't need this if you are using Velbon Super Mag Slider - just use my value of BACKLASH)
// When BL_DEBUG is defined, two keys get reassigned: keys "5" and "6" become "reduce BACKLASH" and "increase BACKLASH" functions
// Don't use BL_DEBUG together with either BL2_DEBUG or DELAY_DEBUG!
//#define BL_DEBUG
// Uncomment this line to measure the BACKLASH_2 parameter for your rail (you don't need this if you are using Velbon Super Mag Slider - just use my value of BACKLASH_2)
// When BL2_DEBUG is defined, two keys get reassigned: keys "5" and "6" become "reduce BACKLASH_2" and "increase BACKLASH_2" functions
// Don't use BL2_DEBUG together with either BL_DEBUG or DELAY_DEBUG!
//#define BL2_DEBUG
// Step for changing both BACKLASH and BACKLASH_2, in microsteps:
const COORD_TYPE BL_STEP = 1;
// Uncomment this line to measure SHUTTER_ON_DELAY2 (electronic shutter for Canon DSLRs; when mirror_lock=2).
// When DELAY_DEBUG is defined, two keys get reassigned: keys "5" and "6" become "reduce SHUTTER_ON_DELAY2" and "increase SHUTTER_ON_DELAY2" functions
// Don't use DELAY_DEBUG together with either BL_DEBUG or BL2_DEBUG!
//#define DELAY_DEBUG
// Step used durinmg DELAY_DEBUG (in us)
//const TIME_TYPE DELAY_STEP = 50000;
// Uncomment to disable shutter triggering:
//#define DISABLE_SHUTTER
// Uncomment to display the amount of used EEPROM in "*" screen (bottom line)
//#define SHOW_EEPROM
// Display positions and temperature in raw units:
//#define SHOW_RAW
// Dumping the contents of the telescope memory registers to serial monitor, and optionally updating EEPROM with new values read from the monitor:
//#define DUMP_REGS
// If defined, macro rail will be used to test the accuracy of the foreground switch (repeatedly triggering it and measuring the spread of trigger positions)
//#define TEST_SWITCH
// If defined, use serial monitor to receive switch test data (only in TEST_SWITCH mode):
//#define SERIAL_SWITCH
// Testing the Hall sensor (takes +5V from PIN_SHUTTER, sends signal to PIN_LIMITERS). Turns backlight on when the sensor is engaged, off otherwise
//#define TEST_HALL
//#define TEST_LIMITER // If defined, displays limiter state after the coordinate

// Memory saving tricks:
// Show only short error messages instead of detailed ones (saves space):
// Show bitmaps (takes more space):
#define BATTERY_BITMAPS
#define REWIND_BITMAPS

// Port expander initial state:
// For MCP23S17 expander; ports numbering is 16, 15, ..., 1 (B7, B6, ..., A7, ..., A0):
s16 IO_MODE =   0B1100000011111111;
s16 IO_PULLUP = 0B1111111111111111;

//////// Camera related parameters: ////////
// Delay between triggering AF on and starting shooting in continuous stacking mode; microseconds
// (If your continuous focus stacking skips the very first shot, increase this parameter)
const TIME_TYPE CONT_STACKING_DELAY = 100000;  // 100000
const TIME_TYPE SHUTTER_TIME_US = 100000; // Time to keep the shutter button pressed (us) 100000
const TIME_TYPE SHUTTER_ON_DELAY = 5000; // Delay in microseconds between setting AF on and shutter on  5000
const TIME_TYPE SHUTTER_OFF_DELAY = 5000; // Delay in microseconds between setting shutter off and AF off  5000
// The mode of AF synching with the shutter:
//  0 (default): AF is synched with shutter (when shutter is on AF is on; when shutter is off AF is off) only
//      for non-continuous stacking (#0); during continuous stacking, AF is permanently on (this can increase the maximum FPS your camera can yield);
//  1: AF is always synched with shutter, even for continuous stacking. Use this feature only if your camera requires it.
const short AF_SYNC = 0;
#ifdef DELAY_DEBUG
// Initial values for the two electronic shutter delays during delay debugging:
// The SHUTTER_ON_DELAY2 value can be modified during debugging (keys 2/3); the SHUTTER_OFF_DELAY2 value is fixed
TIME_TYPE SHUTTER_ON_DELAY2 = 1100000;
TIME_TYPE SHUTTER_OFF_DELAY2 = 100000;
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
const TIME_TYPE SHUTTER_ON_DELAY2 = 500000; // !!! 1100000 for 50D, 500000 for 6D/MLV (4/1.5s delays; 1/4s exposure; ExpOverride ON, ExpSim ON)
const TIME_TYPE SHUTTER_OFF_DELAY2 = 100000; // 100000
#endif

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
// Using hardware SPI (pins D5 and D7)
//const byte TFT_CS = D0;
//#define TFT_RST  -1 // Connect to RST pin of D1 Mini
//const byte TFT_DC = D4;
// Pin to read digital input from the two limiting switches (normally LOW; HIGH when limiters are triggered)
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
// To reduce reading noise, a 0.1uF capacitor has to be soldered parallel to R4.
// The second factor is 3.3V/1024/8 (assumes 8 AA batteries) - don't change it.
const float VOLTAGE_SCALER = 15.28 * 3.3 / 1024.0 / 8.0; // TOCHANGE
// Critically low voltage, per AA battery (when V becomes lower than this, the macro rail is disabled)
// Set it slightly above the value when the rail with camera starts skipping steps
const float V_LOW = 1;
// Highest voltage from a freshly charged AA battery:
const float V_HIGH = 1.4;


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


//////// Parameters related to the motor, the rail, and the telescope: ////////
// Number of full steps per rotation for the stepper motor:
const COORD_TYPE MOTOR_STEPS = 200;
// M0, M1, M2 values determine the number of microsteps per step (see DRV8825 specs; all three HIGH correspond to 32 microsteps)
const byte MOTOR_M0 = HIGH;
const byte MOTOR_M1 = HIGH;
const byte MOTOR_M2 = HIGH;
// Number of microsteps in a step (corresponding to the above M0, M1, M2 values):
const COORD_TYPE N_MICROSTEPS = 32;
// Macro rail parameter: travel distance per one rotation, in mm (3.98mm for Velbon Mag Slider):
const float MM_PER_ROTATION = 3.98;
// The value for alternative device (TELESCOPE mode):
const float MM_PER_ROTATION_TEL = 24;
// Backlash compensation (in mm); positive direction (towards background) is assumed to be the good one (no BL compensation required);
// all motions moving in the bad (negative) direction at the end will need some BL compensation.
// Using the simplest BL model (assumption: rail physically doesn't move until rewinding the full g.backlash amount,
// and then instantly starts moving; same when moving to the positive direction after moving to the bad direction).
// The algorithm guarantees that every time the rail comes to rest, it is fully BL compensated (so the code coordinate = physical coordinate).
// Should be determined experimentally: too small values will produce visible backlash (two or more frames at the start of the stacking
// sequence will look alsmost identical). For my Velbon Super Mag Slide rail I measured the BL to be ~0.2 mm.
// Set it to zero to disable BL compensation.
const float BACKLASH_MM = 0.2; // 0.2mm for Velbon Super Mag Slider
// The backlash value for the second device:
const float BACKLASH_TEL_MM = 0.2; // 0.2mm for Celestron telescope focuser
// This is the second backlash related parameter you need to measure for you rail (or just use the value provided if your rail is Velbon Super Mag Slider)
// This parameter is only relevant for one operation - rail reversal (*1 function). Unlike the above parameter (BACKLASH_MM) which can be equal to or
// larger than the actual backlash value for the rail movements to be perfectly accurate, the BACKLASH_2 parameter has to have a specific value (not larger, no smaller); if
// your rail backlash changes as a function of the rail angle, position on the rail, camera weight etc., BACKLASH_2 would have to change as well.
// Because it is not practical, your value of BACKLASH_2 should be a compromise, giving reasonable results under normal usage scenario. In any case
// rail reversal (*1) wasn't meant to be a perfectly accurate operation, whereas the standard backlash compensation is.
// Use the BL2_DEBUG mode to find the good value of this parameter. You should convert the displayed value of BL2_DEBUG from microsteps
// to mm, by multiplying by MM_PER_ROTATION/(MOTOR_STEPS*N_MICROSTEPS).
// Adjust this parameter only after you found a good value for BACKLASH_MM parameter.
// No equivalent parameter for the TELESCOPE mode as one doesn't need to reverse the directional keys meaning with a telescope focuser.
const float BACKLASH_2_MM = 0.3333; // 0.3333mm for Velbom Super Mag Slider
// Speed limiter, in mm/s. Higher values will result in lower torques and will necessitate larger travel distance
// between the limiting switches and the physical limits of the rail. In addition, too high values will result
// in Arduino loop becoming longer than inter-step time interval, which can screw up the algorithm.
// For an arbitrary rail and motor, make sure the following condition is met:
// 10^6 * MM_PER_ROTATION / (MOTOR_STEPS * N_MICROSTEPS * SPEED_LIMIT_MM_S) >~ 500 microseconds
// Macro rail speed limit:
const float SPEED_LIMIT_MM_S = 10; // 10
// The limit for TELESCOPE mode:
const float SPEED_LIMIT_TEL_MM_S = 5;
// Breaking distance (mm) for the rail when stopping while moving at the fastest speed (SPEED_LIMIT)
// This will determine the maximum acceleration/deceleration allowed for any rail movements - important
// for reducing the damage to the (mostly plastic) rail gears. Make sure that this distance is smaller
// than the smaller distance of the two limiting switches (between the switch actuation and the physical rail limits)
const float BREAKING_DISTANCE_MM = 1.0;
// The value for TELESCOPE mode:
const float BREAKING_DISTANCE_TEL_MM = 1.0;
// Padding (in mm) for a soft limit, before hitting the limiters (increase if you constantly hit the limiter by accident)
const float LIMITER_PAD_MM = 2.5;
// A bit of extra padding (in mm) when calculating the breaking distance before hitting the limiters (to account for inaccuracies of go_to()):
// (increase if you constantly hit the limiter by accident)
const float LIMITER_PAD2_MM = 0.6;
// During calibration, after hitting the first limiter, breaking, and moving in the opposite direction,
// travel this many mm, before starting checking the limiter again (should be large enough that the limiter is guaranteed to go off by that point)
const float DELTA_LIMITER_MM = 4.0;
// Delay in microseconds between LOW and HIGH writes to PIN_STEP
// For DRV8825 stepper driver it should be at least 1.9 us. Form my measurements, setting STEP_LOW_DT to 2 us results
// in 2.8 us impulses, to 1 us - in 1.7 us impulses, so I choose to use 2 us:
const short STEP_LOW_DT = 2;
// Delay after writing to PIN_ENABLE, ms (only used in SAVE_ENERGY mode):
const short ENABLE_DELAY_MS = 3;
// Initial coordinate (mm) for telescope:
const float TEL_INIT_MM = 1;
// The maximum travel distance in telescope mode, starting from the closest position:
const float TEL_LENGTH_MM = 45;
// Number of consequitive HIGH values to set g.limit_on to HIGH
// Set it to >1 if you get false limiter triggering when motor is in use. The larger the number, the more stable it is against the impulse noise
// (the drawback - you'll start having a lag between the actual trigger and the reaction to it.)
//const byte N_LIMITER = 1;  // Not used
const byte OVERSHOOT = 3; // In all moves, overshoot the target by these many microsteps (stop will happen at the accurate target position). To account for roundoff errors.


//////// User interface parameters: ////////
const TIME_TYPE COMMENT_DELAY = 1000000; // time in us to keep the comment line visible
const TIME_TYPE T_KEY_LAG = 500000; // time in us to keep a parameter change key pressed before it will start repeating
const TIME_TYPE T_KEY_REPEAT = 200000; // time interval in us for repeating with parameter change keys
const TIME_TYPE DISPLAY_REFRESH_TIME = 1000000; // time interval in us for refreshing the whole display (only when not moving). Mostly for updating the battery status and temperature


//////// INPUT PARAMETERS: ////////
// Number of custom memory registers; macro:
const byte N_REGS = 5;
// telescope:
const byte N_REGS_TEL = 1;
// Number of backlight levels (not used in h2.0):
#define N_BACKLIGHT 4
// Specific backlight levels (N_BACKLIGHT of them; 255 is the maximum value):
const byte Backlight[] = {0, 110, 127, 255};
// If defined, the smaller values (< 20 microsteps) in the MM_PER_FRAME table below will be rounded off to the nearest whole number of microsteps.
//#define ROUND_OFF
// Number of values for the input parameters (mm_per_frame etc):
const COORD_TYPE N_PARAMS = 25;
// Now we are using microsteps per frame input table, for both macro and telescope modes:
const COORD_TYPE MSTEP_PER_FRAME[] = {1,       2,          4,      6,    8,    12,   16,    20,   24,   32,   40,   48,   64,  80,  120, 160,  200, 240, 320, 400, 480, 640, 800, 1200, 1600};
// Frame per second parameter (Canon 50D can do up to 4 fps when Live View is not enabled, for 20 shots using 1000x Lexar card):
const float FPS[] = {0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.08, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.5, 0.6, 0.8, 1, 1.2, 1.5, 2, 2.5, 3, 3.5, 4};
// Number of shots parameter (to be used in 1-point stacking):
const COORD_TYPE N_SHOTS[] = {2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 75, 100, 125, 150, 175, 200, 250, 300, 400, 500, 600};
// Two delay parameters for the non-continuous stacking mode (initiated with "#0"):
// The length of the first delay table:
const byte N_FIRST_DELAY = 7;
// First delay in non-continuous stacking (from the moment rail stops until the shot is initiated), in seconds:
const float FIRST_DELAY[N_FIRST_DELAY] = {0.5, 1, 1.5, 2, 3, 4, 8};
// The length of the second delay table:
const byte N_SECOND_DELAY = 7;
// Second delay in non-continuous stacking (from the shot initiation until the rail starts moving again), in seconds
// (This should be always longer than the camera exposure time)
const float SECOND_DELAY[N_SECOND_DELAY] = {0.5, 1, 1.5, 2, 3, 4, 8};
// Table of possible values for accel_factor parameter:
const byte N_ACCEL_FACTOR = 4;
const byte ACCEL_FACTOR[N_ACCEL_FACTOR] = {1, 3, 6, 9};
// Table for N_timelapse parameter (number of stacking sequences in the timelapse mode); 1 means no timelapse (just one stack):
const byte N_N_TIMELAPSE = 7;
const COORD_TYPE N_TIMELAPSE[N_N_TIMELAPSE] = {1, 3, 10, 30, 100, 300, 999};
// Table for dt_timelapse parameter (time in seconds between different stacks in timelapse mode; if it is shorter than a single stack time, the latter is used)
const byte N_DT_TIMELAPSE = 9;
const COORD_TYPE DT_TIMELAPSE[N_DT_TIMELAPSE] = {1, 3, 10, 30, 100, 300, 1000, 3000, 9999};

// Buzzer stuff:
#ifdef BUZZER
const TIME_TYPE DT_BUZZ_US = 125; // Half-period for the buzzer sound, us; computed as 10^6/(2*freq_Hz)
#endif

//////////////////////  Telescope stuff //////////////////////
// Names for different focusing points.
char const Name[N_REGS_TEL][15] = {
  "              "
};
// Temperature related parameters
#ifdef TEMPERATURE
// Number of times temperature is measured in a loop (for better accuracy):
const byte N_TEMP = 10;
// Resistance of the pullup resistor at PIN_AF, kOhms. Should be determined by connecting a resistor with known resistance, R0, to PIN_AF in SHOW_RAW mode,
// and pressing the * key: this will show the raw read value at PIN_AF, raw_T (bottom line, on the left). Now R_pullup can be computed from the voltage
// divider equation:
//    R_pullup = R0 * (1024/raw_T - 1)
// Use R0 ~ R_pullup for the best measurement accuracy.
// For now, as I only have a 10k thermistor, my hack is to use an external pullup resistor, and use PIN_SHUTTER to deliver the +5V to the voltage divider on the telescope:
const float R_pullup = 10.045;  // 35.2K for my internal pullup resistor;
// The three thermistor coefficients in Steinhart–Hart equation (https://en.wikipedia.org/wiki/Thermistor). Should be computed by solving a set of three linear
// equations (three instances of Steinhart–Hart equation written for three different temperatures), with a,b,c being the unknowns. One can use online solvers,
// e.g. this one: http://octave-online.net . One has to enter two lines there. The first one contains the three measured resistances of the thermistor (k), at
// three different temperatures, and then the temperature values (C):
// > R1=49; R2=51; R3=53; T1=5; T2=15; T3=25;
// The second line solves the system of three Steinhart–Hart equations, and prints the solutions - coefficients a, b, c:
// > A=[1,log(R1),(log(R1))^3;1,log(R2),(log(R2))^3;1,log(R3),(log(R3))^3];T0=273.15;b=[1/(T0+T1);1/(T0+T2);1/(T0+T3)]; x=A\b
/*  Or one can use least squares method fore more accurate results (needs >3 measurements). E.g. for four measurements:
  octave:22> R1=9.03; R2=11.94; R3=32.04; R4=9.99; T1=27.2; T2=21.3; T3=-1.05; T4=25.3;
  octave:23> A=[1,log(R1),(log(R1))^3;1,log(R2),(log(R2))^3;1,log(R3),(log(R3))^3;1,log(R4),(log(R4))^3];T0=273.15;b=[1/(T0+T1);1/(T0+T2);1/(T0+T3);1/(T0+T4)];
  octave:24> ols(b,A)
*/
const float SH_a = 2.777994e-03;
const float SH_b = 2.403028e-04;
const float SH_c = 1.551810e-06;
// Thermal expansion coefficient for your telescope, in mm/K units. Focus moves by -CTE*(Temp-Temp0) when temperature changes.
// This is not the official CTE (normalized per 1mm of the telescope length), but rather the product of the official CTE x length of the telescope.
// It can be measured by focusing the same eyepice or camera on a star at two different temperatures, one of them designated as Temp0, the other one
// termed Temp1. After each focusing the precise focusing positions x0 and x1 (in mm) and the temperatures (as measured by Arduino) are written down. Then CTE is computed as
//   CTE = -(x1-x0) / (Temp1-Temp0)
// (the minus sign is because when the telescope tube expands, focus point moves closer to the telescope, resulting in a smaller coordinate).
const float CTE = 1.5e-2;
#endif
// Largest allowed focus shift due to changing temperature for the current memory point, in microsteps. If delta_pos becomes larger than this value,
// the memory point index in the status line starts flashing (meaning we need to travel to that point again).
const COORD_TYPE DELTA_POS_MAX = 2;
const TIME_TYPE FLASHING_DELAY = 300000;


//////////////////////////////////////////// Normally you shouldn't modify anything below this line ///////////////////////////////////////////////////

//////// LCD stuff: ////////
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
#define FONT_HEIGHT 10 // Font height in pixels
#define FONT_WIDTH 8 // Font width in pixels
#define TFT_NX 20 // Chars per line
#define TFT_NY 6 // Number of lines
#define TOP_GAP 4 // Empty top gap in pixels (same for bottom gap)
#define LEFT_GAP FONT_WIDTH // Empty left gap in pixels
#define LINE_GAP 10 // Empty gap between lines in pixels
#define DEL_BITMAP 2 // Offset for drawBitmap relative to print

// MM per microstep:
const float MM_PER_MICROSTEP = MM_PER_ROTATION / ((float)MOTOR_STEPS * (float)N_MICROSTEPS);
// TELESCOPE value:
const float MM_PER_MICROSTEP_TEL = MM_PER_ROTATION_TEL / ((float)MOTOR_STEPS * (float)N_MICROSTEPS);
// Number of microsteps per rotation
const COORD_TYPE MICROSTEPS_PER_ROTATION = MOTOR_STEPS * N_MICROSTEPS;
// Breaking distance in internal units (microsteps):
const float BREAKING_DISTANCE = MICROSTEPS_PER_ROTATION * BREAKING_DISTANCE_MM / (1.0 * MM_PER_ROTATION);
// TELESCOPE value:
const float BREAKING_DISTANCE_TEL = MICROSTEPS_PER_ROTATION * BREAKING_DISTANCE_TEL_MM / (1.0 * MM_PER_ROTATION_TEL);
const float SPEED_SCALE = MICROSTEPS_PER_ROTATION / (1.0e6 * MM_PER_ROTATION); // Conversion factor from mm/s to usteps/usecond
// TELESCOPE value:
const float SPEED_SCALE_TEL = MICROSTEPS_PER_ROTATION / (1.0e6 * MM_PER_ROTATION_TEL); // Conversion factor from mm/s to usteps/usecond
// Speed limit in internal units (microsteps per microsecond):
const float SPEED_LIMIT = SPEED_SCALE * SPEED_LIMIT_MM_S;
// TELESCOPE value:
const float SPEED_LIMIT_TEL = SPEED_SCALE_TEL * SPEED_LIMIT_TEL_MM_S;
// Maximum acceleration/deceleration allowed, in microsteps per microseconds^2 (a float)
// (This is a limiter, to minimize damage to the rail and motor)
const float ACCEL_LIMIT = SPEED_LIMIT * SPEED_LIMIT / (2.0 * BREAKING_DISTANCE);
// TELESCOPE value:
const float ACCEL_LIMIT_TEL = SPEED_LIMIT_TEL * SPEED_LIMIT_TEL / (2.0 * BREAKING_DISTANCE_TEL);
// Speed small enough to allow instant stopping (such that stopping within one microstep is withing ACCEL_LIMIT):
// 2* - to make goto accurate, but with higher decelerations at the end
// Currently not used
const float SPEED_SMALL = 2 * sqrt(2.0 * ACCEL_LIMIT);
// A small float (to detect zero speed):
const float SPEED_TINY = 1e-5 * SPEED_LIMIT;
// Backlash in microsteps (+0.5 for proper round-off):
const COORD_TYPE BACKLASH = (COORD_TYPE)(BACKLASH_MM / MM_PER_MICROSTEP + 0.5);
const COORD_TYPE BACKLASH_TEL = (COORD_TYPE)(BACKLASH_TEL_MM / MM_PER_MICROSTEP_TEL + 0.5);
#ifdef BL2_DEBUG
// Initial value for BACKLASH_2:
COORD_TYPE BACKLASH_2 = (COORD_TYPE)(BACKLASH_2_MM / MM_PER_MICROSTEP + 0.5);
#else
// Backlash correction for rail reversal (*1) in microsteps:
const COORD_TYPE BACKLASH_2 = (COORD_TYPE)(BACKLASH_2_MM / MM_PER_MICROSTEP + 0.5);
#endif
// Maximum FPS possible (depends on various delay parameters above; the additional factor of 2000 us is to account for a few Arduino loops):
const float MAXIMUM_FPS = 1e6 / (float)(SHUTTER_TIME_US + SHUTTER_ON_DELAY + SHUTTER_OFF_DELAY + 2000);
// Only matters if BACKLASH is non-zero. If defined, pressing the rewind key ("1") for a certain length of time will result in the travel by the same
// amount as when pressing fast-forward ("A") for the same period of time, with proper backlash compensation. This should result in smoother user experience.
// If undefined, to rewind by the same amount,
// one would have to press the rewind key longer (compared to pressing fast-forward key), to account for backlash compensation.
#define EXTENDED_REWIND
const float TEL_INIT = TEL_INIT_MM / MM_PER_MICROSTEP_TEL;
const float TEL_LENGTH = TEL_LENGTH_MM / MM_PER_MICROSTEP_TEL;
const COORD_TYPE DELTA_LIMITER = (COORD_TYPE)(DELTA_LIMITER_MM / MM_PER_MICROSTEP + 0.5);
const COORD_TYPE LIMITER_PAD = (COORD_TYPE)(LIMITER_PAD_MM / MM_PER_MICROSTEP + 0.5);
const COORD_TYPE LIMITER_PAD2 = (COORD_TYPE)(LIMITER_PAD2_MM / MM_PER_MICROSTEP + 0.5);

// Structure to have custom parameters saved to EEPROM
struct regist
{
  byte i_n_shots; // counter for n_shots parameter;
  byte i_mm_per_frame; // counter for mm_per_frame parameter;
  byte i_fps; // counter for fps parameter;
  byte i_first_delay; // counter for FIRST_DELAY parameter
  byte i_second_delay; // counter for SECOND_DELAY parameter
  byte i_accel_factor; // Index for accel_factor
  byte i_n_timelapse; // counter for N_TIMELAPSE parameter
  byte i_dt_timelapse; // counter for DT_TIMELAPSE parameter
  byte mirror_lock; // 1: mirror lock is used in non-continuous stacking; 0: not used; 2: similar to 0, but using SHUTTER_ON_DELAY2, SHUTTER_OFF_DELAY2 instead of SHUTTER_ON_DELAY, SHUTTER_OFF_DELAY
  byte backlash_on; // =1 when g.backlash=BACKLASH; =0 when g.backlash=0.0
  byte straight;  // 0: reversed rail (PIN_DIR=LOW is positive); 1: straight rail (PIN_DIR=HIGH is positive)
  byte save_energy; // =0: always using the motor's torque, even when not moving (should improve accuracy and holding torque); =1: save energy (only use torque during movements)
  COORD_TYPE point[4];  // four memory points (only 0th - foreground, and 3rd - background, are used in macro mode; all four are used in telescope mode)
  unsigned int raw_T[4]; // temperatures corresponding to the four memory points (only used in telescope mode), in raw units (so effectively resistance of the thermistor in relative units)
};
// Just in case adding a 1-byte if SIZE_REG is odd, to make the total regist size even (I suspect EEPROM wants data to have even number of bytes):
short SIZE_REG = sizeof(regist);

const short dA = sizeof(COORD_TYPE);

// EEPROM addresses: make sure they don't go beyong the ESP8266 EEPROM size of 4k!
const int ADDR_POS = 0;  // Current position (integer, 4 bytes)
const int ADDR_LIMIT2 = ADDR_POS + 4; // pos_int for the background limiter (4 bytes)
const int ADDR_BACKLIGHT = ADDR_LIMIT2 + dA;  // backlight level (1 byte)
const int ADDR_REG1 = ADDR_BACKLIGHT + 2;  // Start of default + N_REGS custom memory registers for macro mode
const int ADDR_REG1_TEL = ADDR_REG1 + (N_REGS + 1) * SIZE_REG; // Start of default + N_REGS custom memory registers for telescope mode
const int ADDR_LOCK = ADDR_REG1_TEL + (N_REGS_TEL + 1) * SIZE_REG; // Lock flags for N_REGS telescope registers
const int ADDR_IREG = ADDR_LOCK + N_REGS_TEL; // The register currently in use (1 byte)
const int ADDR_END = ADDR_IREG + 2;  // End of used EEPROM

// 2-char bitmaps to display the battery status; 5 levels: 0 for empty, 4 for full:
const uint8_t battery_char [][20] = {{
  0B00111111,0B11111111,
  0B00100000,0B00000001,
  0B00100000,0B00000001,
  0B11100000,0B00000001,
  0B11100000,0B00000001,
  0B11100000,0B00000001,
  0B11100000,0B00000001,
  0B00100000,0B00000001,
  0B00100000,0B00000001,
  0B00111111,0B11111111
},{
  0B00111111,0B11111111,
  0B00100000,0B00001111,
  0B00100000,0B00001111,
  0B11100000,0B00001111,
  0B11100000,0B00001111,
  0B11100000,0B00001111,
  0B11100000,0B00001111,
  0B00100000,0B00001111,
  0B00100000,0B00001111,
  0B00111111,0B11111111
},{
  0B00111111,0B11111111,
  0B00100000,0B01111111,
  0B00100000,0B01111111,
  0B11100000,0B01111111,
  0B11100000,0B01111111,
  0B11100000,0B01111111,
  0B11100000,0B01111111,
  0B00100000,0B01111111,
  0B00100000,0B01111111,
  0B00111111,0B11111111
},{
  0B00111111,0B11111111,
  0B00100011,0B11111111,
  0B00100011,0B11111111,
  0B11100011,0B11111111,
  0B11100011,0B11111111,
  0B11100011,0B11111111,
  0B11100011,0B11111111,
  0B00100011,0B11111111,
  0B00100011,0B11111111,
  0B00111111,0B11111111
},{
  0B00111111,0B11111111,
  0B00111111,0B11111111,
  0B00111111,0B11111111,
  0B11111111,0B11111111,
  0B11111111,0B11111111,
  0B11111111,0B11111111,
  0B11111111,0B11111111,
  0B00111111,0B11111111,
  0B00111111,0B11111111,
  0B00111111,0B11111111
}
};
// 8x10 bitmaps
const uint8_t rewind_char[] = {
  0B00001100,0B00000000,0B00000000,
  0B00011000,0B00000000,0B00000000,
  0B00110000,0B00000000,0B00000000,
  0B01100000,0B00000000,0B00000000,
  0B11111111,0B11111111,0B00000000,
  0B11111111,0B11111111,0B00000000,
  0B01100000,0B00000000,0B00000000,
  0B00110000,0B00000000,0B00000000,
  0B00011000,0B00000000,0B00000000,
  0B00001100,0B00000000,0B00000000
};
const uint8_t forward_char[] = {
  0B00000000,0B00110000,0B00000000,
  0B00000000,0B00011000,0B00000000,
  0B00000000,0B00001100,0B00000000,
  0B00000000,0B00000110,0B00000000,
  0B11111111,0B11111111,0B00000000,
  0B11111111,0B11111111,0B00000000,
  0B00000000,0B00000110,0B00000000,
  0B00000000,0B00001100,0B00000000,
  0B00000000,0B00011000,0B00000000,
  0B00000000,0B00110000,0B00000000
};
const uint8_t straight_char[] = {
  0B00001000,0B00000000,0B00000000,
  0B00001100,0B00000000,0B00000000,
  0B00001110,0B00000000,0B00000000,
  0B00001111,0B00000000,0B00000000,
  0B00001111,0B10000000,0B00000000,
  0B00001111,0B10000000,0B00000000,
  0B00001111,0B00000000,0B00000000,
  0B00001110,0B00000000,0B00000000,
  0B00001100,0B00000000,0B00000000,
  0B00001000,0B00000000,0B00000000
};
const uint8_t reverse_char[] = {
  0B00000000,0B00010000,0B00000000,
  0B00000000,0B00110000,0B00000000,
  0B00000000,0B01110000,0B00000000,
  0B00000000,0B11110000,0B00000000,
  0B00000001,0B11110000,0B00000000,
  0B00000001,0B11110000,0B00000000,
  0B00000000,0B11110000,0B00000000,
  0B00000000,0B01110000,0B00000000,
  0B00000000,0B00110000,0B00000000,
  0B00000000,0B00010000,0B00000000
};

const float TEMP0_K = 273.15;  // Zero Celcius in Kelvin

// All global variables belong to one structure - global:
struct global
{
  // New vars in v2.0
  // Each model of motion is completely described by the following parameters:
  byte Npoints; // Number of points in the model (2..5). Points correspond to times when acceleration or direction changes
  byte i_point; // Index of the current point (0..Npoints-1)
  #define N_POINTS_MAX 5  // Largest possible value for Npoints
  char model_accel[N_POINTS_MAX]; // Acceleration index (-2...2) at each model point
  TIME_TYPE model_time[N_POINTS_MAX]; // Model time for each model point (relative to the 0-th point)
  float model_speed[N_POINTS_MAX]; // Model speed (only matters for accel=0 legs; absolute values)
  float model_pos[N_POINTS_MAX]; // Model position (relative to the 0-th point) at each point
  byte model_ptype[N_POINTS_MAX]; // Model point type:
  #define INIT_POINT 0  // Starting moving from rest
  #define ACCEL_CHANGE_POINT 1  // Changing acceleration
  #define STOP_POINT 2  // Final (stop) point
  #define DIR_CHANGE_POINT 3  // Changing direction point
  char model_dir[N_POINTS_MAX]; // Model direction (-1, 0, 1), in the sense of g.direction values
  TIME_TYPE model_t0; // Absolute (model) time for the first model point.
  byte model_hits_limit; // if =1, the last leg of the model is after hitting a soft limit, and should be uninterrupted
  byte model_init; // 1: model was just initiated (in go_to etc), first motor_control() call will start processing it
  COORD_TYPE ipos0; // The coordinate at the start of a movement.
  byte direction_predict; // Direction prediction for the next change of direction event
  TIME_TYPE t_next_event; // Timing prediction for the next event (step or direction change)
  byte next_event_type; // =0 for a step, =1 for a dir
  byte model_change; // 0: nothing, 1: switch to accelerate model, 2: switch to stop model. Only matters if updated while moving
  //-----------------
  struct regist reg; // Custom parameters register
  int addr_reg[N_REGS + 1]; // The starting addresses of the EEPROM memory registers (different for macro and telescope modes), including the default (0th) one
  // Variables used to communicate between modules:
  TIME_TYPE t;  // Time in us measured at the beginning of motor_control() module
  TIME_TYPE t_predict; // Predicted time (in us) for the next event (make a step, change direction, or change acceleration)
  signed char t_type; // Type of the next event: 0 (make a step), 1 (change direction), 2 (change acceleration)
  byte moving;  // 0 for stopped, 1 when moving; can only be set to 0 in motor_control()
  float speed1; // Target speed, in microsteps per microsecond
  float speed;  // Current speed (negative, 0 or positive)
  signed char accel; // Current acceleration index. Allowed values: -2,1,0,1,2 . +-2 correspond to ACCEL_LIMIT, +-1 correspond to ACCEL_SMALL
  signed char accel_old; // Previous loop acceleration index.
  float accel_v[5]; // Five possible floating point values for acceleration
  float accel_limit; // Maximum allowed acceleration
  COORD_TYPE ipos;  // Current position (in microsteps). Should be stored in EEPROM before turning the controller off, and read from there when turned on
  COORD_TYPE ipos0;  // Last position when accel changed
  TIME_TYPE t0; // Last time when accel changed
  float speed0; // Last speed when accel changed
  float speed_old; // speed at the previous step
  float pos_stop; // Current stop position if breaked
  float pos_stop_old; // Previously computed stop position if breaked
  TIME_TYPE t_key_pressed; // Last time when a key was pressed
  TIME_TYPE t_last_repeat; // Last time when a key was repeated (for parameter change keys)
  int N_repeats; // Counter of key repeats
  TIME_TYPE t_display; // time since the last display refresh (only when not moving)
  /* a flag for each leg of calibration: 
    0: no calibration; 
    1: initiating full calibration: moving towards switch 2 for its calibration, with maximum speed and acceleration;
    2: triggered limit2 and stopped, initiating move towards switch 1
    3: triggered limit1 and stopped, initiating move forward to calibrate limit1 on the first switch-off position
    4: moving forward to calibrate limit1 on the first switch-off position;
    5: end of calibration; updating coordinates;
    10: initiating telescope calibration: moving forward until the switch goes off and the maximum speed is reached (accel=0)
   */
  byte calibrate_flag; 
  COORD_TYPE limit1; // pos_int for the foreground limiter (temporary value, only used when accidently triggering foreground switch)
  COORD_TYPE limit2; // pos_int for the background limiter
  byte accident;  // =1 if we accidently triggered limit1; 0 otherwise
  byte limit_on; //  The last recorded state of the limiter switches
  byte uninterrupted;  // =1 disables checking for limits (hard and soft); used for emergency breaking and during calibration
  byte uninterrupted2;  // =1 disables checking for limits (hard and soft); used for recovering rail when it's confused (#D command)
  byte travel_flag; // =1 when travel was initiated
  COORD_TYPE ipos_goto; // position to go to
  byte moving_mode; // =0 when using speed_change, =1 when using go_to
  byte pos_stop_flag; // flag to detect when motor_control is run first time
  char key_old;  // peviously pressed key; used in keypad()
  COORD_TYPE starting_point; // The starting point in the focus stacking with two points
  COORD_TYPE destination_point; // The destination point in the focus stacking with two points
  byte stacker_mode;  // 0: default (rewind etc.); 1: pre-winding for focus stacking; 2: 2-point focus stacking; 3: single-point stacking
  float msteps_per_frame; // Microsteps per frame for focus stacking
  short Nframes; // Number of frames for 2-point focus stacking
  short frame_counter; // Counter for shots
  COORD_TYPE pos_to_shoot; // Position to shoot the next shot during focus stacking
  byte shutter_on; // flag for camera shutter state: 0/1 corresponds to off/on
  byte AF_on; // flag for camera AF state: 0/1 corresponds to off/on
  byte single_shot; // flag for a single shot (made with #7): =1 when the shot is in progress, 0 otherwise
  TIME_TYPE t_shutter; // Time when the camera shutter was triggered
  TIME_TYPE t_shutter_off; // Time when the camera shutter was switched off
  TIME_TYPE t_AF; // Time when the camera AF was triggered
  signed char direction; // -1/1 for reverse/forward directions of moving (request to change direction)
  signed char dir; // -1/1 for reverse/forward directions of moving (the actual state of the motor)
  char buffer[21];  // char buffer to be used for lcd print; 1 more element than the lcd width (20)
  char empty_buffer[21];  // char buffer to be used to clear one row of the LCD; 1 more element than the lcd width (20)
  TIME_TYPE t_comment; // time when commment line was triggered
  byte comment_flag; // flag used to trigger the comment line briefly
  byte x0, y0;  // Displey pixel coordinates, set in misc/my_setCursor
  byte error; // error code (no error if 0); 1: initial limiter on or cable disconnected; 2: battery drained; non-zero value will disable the rail (with some exceptions)
  byte backlight; // backlight level;
  COORD_TYPE coords_change; // if >0, coordinates have to change (because we hit limit1, so we should set limit1=0 at some point)
  byte start_stacking; // =1 if we just initiated focus stacking, =2 when AF is triggered initially, =3 after CONT_STACKING_DELAY delay in continuous mode, =0 when no stacking
  byte make_shot; // =1 if we just initiated a shot; 0 otherwise
  TIME_TYPE t_shot; // the time shot was initiated
  TIME_TYPE t0_stacking; // time when stacking was initiated;
  byte paused; // =1 when 2-point stacking was paused, after hitting any key; =0 otherwise
  COORD_TYPE BL_counter; // Counting microsteps made in the bad (negative) direction. Possible values 0...BACKLASH. Each step in the good (+) direction decreases it by 1.
  byte started_moving; // =1 when we just started moving (the first loop), 0 otherwise
  byte backlashing; // A flag to ensure that backlash compensation is uniterrupted (except for emergency breaking, #B); =1 when BL compensation is being done, 0 otherwise
  byte continuous_mode; // 2-point stacking mode: =0 for a non-continuous mode, =1 for a continuous mode
  byte noncont_flag; // flag for non-continuous mode of stacking; 0: no stacking; 1: initiated; 2: first shutter trigger; 3: second shutter; 4: go to the next frame
  TIME_TYPE t_old;
  float speed_limit;  // Current speed limit, in internal units. Determined once, when the device is powered up
  byte setup_flag; // Flag used to detect if we are in the setup section (then the value is 1; otherwise 0)
  byte alt_flag; // 0: normal display; 1: alternative display
  byte alt_kind; // The kind of alternative display: 1: *; 2: # (telescope only)
  char tmp_char;
  byte backlash_init; // 1: initializing a full backlash loop; 2: initializing a rail reverse
  char buf6[6]; // Buffer to store the stacking length for displaying
  char buf7[7];
  short timelapse_counter; // Counter for the time lapse feature
  TIME_TYPE t_mil; // millisecond accuracy timer; used to set up timelapse stacks
  TIME_TYPE t0_mil; // millisecond accuracy timer; used to set up timelapse stacks
  byte end_of_stacking; // =1 when we are done with stacking (might still be moving, in continuoius mode)
  byte timelapse_mode; // =1 during timelapse mode, 0 otherwise
  COORD_TYPE backlash; // current value of backlash in microsteps (can be either 0 or BACKLASH)
  float mm_per_microstep; // Rail specific setting
  int limiter_counter; // Used in impulse noise suppression inside Read_limiters()
#ifdef BUZZER
  TIME_TYPE t_buzz; // timer for the buzzer
  byte buzz_state; // HIGH or LOW for the buzzer state
#endif  
  TIME_TYPE dt_lost;
#ifdef EXTENDED_REWIND
  byte no_extended_rewind;
#endif
#ifdef TIMING
  TIME_TYPE i_timing;
  TIME_TYPE t0_timing;
  int dt_max;
  int dt_min;
  int bad_timing_counter; // How many loops in the last movement were longer than the shortest microstep interval allowed
  int dt_timing; // Timing for the last loop in motion, us
  int total_dt_timing; // Cumulative movement time in microseconds
  byte moving_old;  // Old value of g.moving
  float d_sum;
  int d_N;
  int d_Nbad;
  int d_max;
  int N_insanity;
#endif
  byte telescope; // LOW if the controller is used with macro rail; HIGH if it's used with a telescope or another alternative device with PIN_SHUTTER unused.
  byte ireg; // The register number to display on the top line in telescope mode (0 means nothing to display).
  int raw_T;  // raw value measured at PIN_AF, used when calibrating temperature sensor (only in telescope mode; if TEMPERATURE is defined)
#ifdef TEMPERATURE
  float Temp; // Current temperature in Celsius; only in telescope mode
  float Temp0[4]; // Temperature for the four memory points for the current register (Celsius)
#endif
  COORD_STYPE delta_pos[4]; // Shift of telescope's focal plane due to thermal expansion of the telescope, in microsteps, for each memory point
  COORD_STYPE delta_pos_curr; // delta_pos value at the last goto memory point operation (used to determine when T is drifting away too much and mempoint # should flash)
  signed char current_point; // The index of the currently loaded memory point. Can be 0/3 for fore/background (macro mode), 0...3 for telescope mode. -1 means no point has been loaded/saved yet.
  TIME_TYPE t_status; // time variable used in generating memory point flashing
  byte status_flag; // Flag used to establish blinking of the current point when delta_pos becomes larger than DELTA_POS_MAX
  byte locked[N_REGS_TEL]; // locked (1) / unlocked (0) flags for N_REGS registers (telescope mode)
  byte n_regs; // The current value of number of memory registers (=N_REGS in macro mode and N_REGS_TEL in telescope mode)
#ifdef TEST_SWITCH
  // Number of tests to perform:
#define TEST_N_MAX 50
  float speed_test;
  short test_flag;
  short on_init;
  float test_sum[2];
  float test_sum2[2];
  short test_N;
  float test_pos0[2];
  float delta_min[2];
  float delta_max[2];
  float test_dev[2];
  float test_avr[2];
  float test_std[2];
  int count[2];
  byte test_limit_on[2];
  float pos_tmp;
  float pos_tmp2;
  float test_limit;
  char buf9[10];
  COORD_TYPE pos0_test;
#endif
#ifdef TEST_HALL
  byte hall_on = 0;
#endif
#ifdef BUZZER
  TIME_TYPE dt1_buzz_us = 1000; // Current half-period for the buzzer sound, us
#endif
#ifdef TEST_LIMITER
  int limiter_i; // counter for the false limiter readings
  byte limiter_ini; // initial limiter state
#endif
};

struct global g;

#endif


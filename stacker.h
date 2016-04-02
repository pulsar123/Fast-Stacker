/* Sergey Mashchenko 2015, 2016

   User header file. Contains user adjustable parameters (constants), and other stuff.

   To be used with automated macro rail for focus stacking
*/

#ifndef STACKER_H
#define STACKER_H

// Requires hardware version h1.2
#define VERSION "1.16"


//////// Debugging options ////////
// Integer type for all coordinates. Use "short" if the total number of microsteps for your rail is <32,000 (this is the case with my hardware - Velbon Super Mag Slider,
// 1.8 degrees stepper motor and 8 microsteps/step motor driver), and use "long" for larger numbers (will consume more memory)
#define COORD_TYPE short
// For timing the main loop:
//#define TIMING
// Motor debugging mode: limiters disabled (used for finetuning the motor alignment with the macro rail knob, finding the minimum motor current,
// and software debugging without the motor unit)
//#define MOTOR_DEBUG
// Uncomment this line when debugging the control unit without the motor unit:
//#define DISABLE_MOTOR
// Battery debugging mode (prints actual voltage per AA battery in the status line; needed to determine the lowest voltage parameter, V_LOW - see below)
//#define BATTERY_DEBUG
// If defined, do camera debugging:
//#define CAMERA_DEBUG
// If defined, software SPI emulation instead of the default harware SPI. Try this if your LCD doesn't work after upgrading to h1.1 or newer and s0.10 or newer
//#define SOFTWARE_SPI
// Uncomment this line to measure the BACKLASH parameter for your rail (you don't need this if you are using Velbon Super Mag Slider - just use my value of BACKLASH)
// When BL_DEBUG is defined, two keys get reassigned: keys "2" and "3" become "reduce BACKLASH" and "increase BACKLASH" functions
// Don't use BL_DEBUG together with either BL2_DEBUG or DELAY_DEBUG!
//#define BL_DEBUG
// Uncomment this line to measure the BACKLASH_2 parameter for your rail (you don't need this if you are using Velbon Super Mag Slider - just use my value of BACKLASH_2)
// When BL2_DEBUG is defined, two keys get reassigned: keys "2" and "3" become "reduce BACKLASH_2" and "increase BACKLASH_2" functions
// Don't use BL2_DEBUG together with either BL_DEBUG or DELAY_DEBUG!
//#define BL2_DEBUG
// Step for changing both BACKLASH and BACKLASH_2, in microsteps:
const COORD_TYPE BL_STEP = 1;
// Uncomment this line to measure SHUTTER_ON_DELAY2 (electronic shutter for Canon DSLRs; when mirror_lock=2).
// When DELAY_DEBUG is defined, two keys get reassigned: keys "2" and "3" become "reduce SHUTTER_ON_DELAY2" and "increase SHUTTER_ON_DELAY2" functions
// Don't use DELAY_DEBUG together with either BL_DEBUG or BL2_DEBUG!
//#define DELAY_DEBUG
// Step used durinmg DELAY_DEBUG (in us)
const long DELAY_STEP = 50000;
// Uncomment to disable shutter triggering:
//#define DISABLE_SHUTTER


//////// Camera related parameters: ////////
// Delay between triggering AF on and starting shooting in continuous stacking mode; microseconds
// (If your continuous focus stacking skips the very first shot, increase this parameter)
const unsigned long CONT_STACKING_DELAY = 100000;  // 100000
const unsigned long SHUTTER_TIME_US = 100000; // Time to keep the shutter button pressed (us) 100000
const unsigned long SHUTTER_ON_DELAY = 5000; // Delay in microseconds between setting AF on and shutter on  5000
const unsigned long SHUTTER_OFF_DELAY = 5000; // Delay in microseconds between setting shutter off and AF off  5000
// The mode of AF synching with the shutter:
//  0 (default): AF is synched with shutter (when shutter is on AF is on; when shutter is off AF is off) only
//      for non-continuous stacking (#0); during continuous stacking, AF is permanently on (this can increase the maximum FPS your camera can yield);
//  1: AF is always synched with shutter, even for continuous stacking. Use this feature only if your camera requires it.
const short AF_SYNC = 0;
#ifdef DELAY_DEBUG
// Initial values for the two electronic shutter delays during delay debugging:
// The SHUTTER_ON_DELAY2 value can be modified during debugging (keys 2/3); the SHUTTER_OFF_DELAY2 value is fixed
long SHUTTER_ON_DELAY2 = 1100000;
long SHUTTER_OFF_DELAY2 = 100000;
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
const unsigned long SHUTTER_ON_DELAY2 = 1100000;
const unsigned long SHUTTER_OFF_DELAY2 = 100000;
#endif

//////// Pin assignment ////////
// Pin 10 is left unused because it is used internally by hardware SPI.
// We are using the bare minimum of arduino pins for stepper driver:
const short PIN_STEP = 0;
const short PIN_DIR = 1;
const short PIN_ENABLE = 2;  // LOW: enable motor; HIGH: disable motor (to save energy)
// LCD pins (Nokia 5110): following resistor scenario in https://learn.sparkfun.com/tutorials/graphic-lcd-hookup-guide
const short PIN_LCD_DC = 5;  // Via 10 kOhm resistor
const short PIN_LCD_LED = 9;  // Via 330 Ohm resistor
const short PIN_LCD_DN_ = 11;  // Via 10 kOhm resistor
const short PIN_LCD_SCL = 13;  // Via 10 kOhm resistor
// Pin to read digital input from the two limiting switches (normally LOW; HIGH when limiters are triggered)
const short PIN_LIMITERS = 8;
// Pin to trigger camera shutter:
const short PIN_SHUTTER = 3;
// Hardware h1.2: pin 6 was reassigned from RST LCD to operate the AF relay:
const short PIN_AF = 6;
// Analogue pin for the battery life sensor:
#define PIN_BATTERY A0
// Hardware h1.1: the chip select LCD pin (SCE, CE) is now soldered to ground via 10k pulldown resistor, to save one Arduino pin; here assigning a bogus value
// (I modified the pcd8544 library to disable the use of this pin). Using a fake value:
const short PIN_LCD_SCE = 100;
// Hardware h1.2: Arduino is no longer needed, as the initial LCD reset is done with a delay RC circuit. Pin 6 can now be used to operate the AF relay
// (I modified the pcd8544 library to disable the use of this pin). Using a fake value:
const short PIN_LCD_RST = 100;


//////// Voltage parameters: ////////
// Scaling coefficient to derive the battery voltage (depends on the resistance of the two dividing resistors, R3 and R4.
// Assuming R3 is the one directly connected to "+" of the battery, the scaler is (R3+R4)/R4. R3+R4 should be ~0.5M)
// To reduce reading noise, a 0.1uF capacitor has to be soldered parallel to R4.
// The second factor is 5.0V/1024/8 (assumes 8 AA batteries) - don't change it.
const float VOLTAGE_SCALER = 2.7273 * 5.0 / 1024.0 / 8.0;
// Critically low voltage, per AA battery (when V becomes lower than this, the macro rail is disabled)
// Set it slightly above the value when the rail with camera starts skipping steps
const float V_LOW = 1.125;
// Highest voltage from a freshly charged AA battery:
const float V_HIGH = 1.4;
// The speed related critical voltage value; if the power voltage is above this value, we assume that we are
// running from AC power, and will be using the larger speed limit SPEED_LIMIT_MM_S; if it is below this value,
// we assume that we are using the battery power and will be using SPEED_LIMIT2_MM_S speed limit.
// The acceleration limit is always computed from SPEED_LIMIT_MM_S.
// This test is only done once, when you power up the device.
// We are dividing the value by 8 as that's how we compute the voltage in battery_status() (per AA battery)
const float SPEED_VOLTAGE = 11.5 / 8.0;


//////// Keypad stuff: ////////
const byte rows = 4; //four rows
const byte cols = 4; //three columns
char keys[rows][cols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
// Hardware v1.1: 4, 7, 12, A1 (was 4, 10, 12, A1; pin 10 was freed to be able to use hardware SPI for LCD)
byte rowPins[rows] = {4, 7, 12, A1}; //connect to the row pinouts of the keypad (6,7,8,9 for mine)
byte colPins[cols] = {A2, A3, A4, A5}; //connect to the column pinouts of the keypad (2,3,4,5 for mine)
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );


//////// Parameters related to the motor and the rail: ////////
// Number of full steps per rotation for the stepper motor:
const COORD_TYPE MOTOR_STEPS = 200;
// Number of microsteps in a step (default for EasyDriver is 8):
const COORD_TYPE N_MICROSTEPS = 8;
// Macro rail parameter: travel distance per one rotation, in mm (3.98mm for Velbon Mag Slider):
const float MM_PER_ROTATION = 3.98;
// Backlash compensation (in mm); positive direction (towards background) is assumed to be the good one (no BL compensation required);
// all motions moving in the bad (negative) direction at the end will need some BL compensation.
// Using the simplest BL model (assumption: rail physically doesn't move until rewinding the full g.backlash amount,
// and then instantly starts moving; same when moving to the positive direction after moving to the bad direction).
// The algorithm guarantees that every time the rail comes to rest, it is fully BL compensated (so the code coordinate = physical coordinate).
// Should be determined experimentally: too small values will produce visible backlash (two or more frames at the start of the stacking
// sequence will look alsmost identical). For my Velbon Super Mag Slide rail I measured the BL to be ~0.2 mm.
// Set it to zero to disable BL compensation.
const float BACKLASH_MM = 0.2; // 0.2mm for Velbon Super Mag Slider
// This is the second backlash related parameter you need to measure for you rail (or just use the value provided if your rail is Velbon Super Mag Slider)
// This parameter is only relevant for one operation - rail reversal (*1 function). Unlike the above parameter (BACKLASH_MM) which can be equal to or
// larger than the actual backlash value for the rail movements to be perfectly accurate, the BACKLASH_2 parameter has to have a specific value (not larger, no smaller); if
// your rail backlash changes as a function of the rail angle, position on the rail, camera weight etc., BACKLASH_2 would have to change as well.
// Because it is not practical, your value of BACKLASH_2 should be a compromise, giving reasonable results under normal usage scenario. In any case
// rail reversal (*1) wasn't meant to be a perfectly accurate operation, whereas the standard backlash compensation is.
// Use the BL2_DEBUG mode to find the good value of this parameter. You should convert the displayed value of BL2_DEBUG from microsteps
// to mm, by multiplying by MM_PER_ROTATION/(MOTOR_STEPS*N_MICROSTEPS).
// Adjust this parameter only after you found a good value for BACKLASH_MM parameter.
const float BACKLASH_2_MM = 0.3333; // 0.3333mm for Velbom Super Mag Slider
// Speed limiter, in mm/s. Higher values will result in lower torques and will necessitate larger travel distance
// between the limiting switches and the physical limits of the rail. In addition, too high values will result
// in Arduino loop becoming longer than inter-step time interval, which can screw up the algorithm.
// 5 mm/s seems to be a reasonable compromize, for my motor and rail.
// For an arbitrary rail and motor, make sure the following condition is met:
// 10^6 * MM_PER_ROTATION / (MOTOR_STEPS * N_MICROSTEPS * SPEED_LIMIT_MM_S) >~ 500 microseconds
// This speed limits is normally used only with AC power (which provides mor torque).
const float SPEED_LIMIT_MM_S = 5;
// The second (smaller) speed limit (used only with a battery power, which provides less torque):
const float SPEED_LIMIT2_MM_S = 2.5;
// Breaking distance (mm) for the rail when stopping while moving at the fastest speed (SPEED_LIMIT)
// This will determine the maximum acceleration/deceleration allowed for any rail movements - important
// for reducing the damage to the (mostly plastic) rail gears. Make sure that this distance is smaller
// than the smaller distance of the two limiting switches (between the switch actuation and the physical rail limits)
const float BREAKING_DISTANCE_MM = 2.0;
// Rewind/fast-forward acceleration factor: the acceleration when pressing "1" or "A" keys (rewind / fast forward) will be slower than the ACCEL_LIMIT (see below) by this factor
// Should be 1 or larger. If 1, we have the old behaviour - acceleration and deceleration are always the same, ACCEL_LIMIT
// This feature is to allow for more precise positioning of the rail, to find good fore/background points, but keep all other rail movements as fast as possible
// Set it to a larger value if you typically deal with high magnifications, and a lower value if you do low magnifications.
//const float ACCEL_FACTOR = 3.0;
// Padding (in microsteps) for a soft limit, before hitting the limiters:
const COORD_TYPE LIMITER_PAD = 400;
// A bit of extra padding (in microsteps) when calculating the breaking distance before hitting the limiters (to account for inaccuracies of go_to()):
const COORD_TYPE LIMITER_PAD2 = 100;
const COORD_TYPE DELTA_LIMITER = 1000; // In calibration, after hitting the first limiter, breaking, and moving in the opposite direction,
// travel this many microsteps after the limiter goes off again, before starting checking the limiter again
// Delay in microseconds between LOW and HIGH writes to PIN_STEP (should be >=1 for Easydriver; but arduino only guarantees delay accuracy for >=3)
const short STEP_LOW_DT = 3;
// Delay after writing to PIN_ENABLE, ms (only used in SAVE_ENERGY mode):
const short ENABLE_DELAY_MS = 3;


//////// User interface parameters: ////////
const unsigned long COMMENT_DELAY = 1000000; // time in us to keep the comment line visible
const unsigned long T_KEY_LAG = 500000; // time in us to keep a parameter change key pressed before it will start repeating
const unsigned long T_KEY_REPEAT = 200000; // time interval in us for repeating with parameter change keys
const unsigned long DISPLAY_REFRESH_TIME = 1000000; // time interval in us for refreshing the whole display (only when not moving). Mostly for updating the battery status


//////// INPUT PARAMETERS: ////////
// If defined, the smaller values (< 20 microsteps) in the MM_PER_FRAME table below will be rounded off to the nearest whole number of microsteps.
#define ROUND_OFF
// Number of values for the input parameters (mm_per_frame etc):
const short N_PARAMS = 25;
//  Mm per frame parameter (determined by DoF of the lens)
float MM_PER_FRAME[] = {0.0025, 0.005, 0.0075, 0.01, 0.015, 0.02, 0.025, 0.03, 0.04, 0.05, 0.06, 0.08, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.8, 1, 1.5, 2, 2.5};
// Frame per second parameter (Canon 50D can do up to 4 fps when Live View is not enabled, for 20 shots using 1000x Lexar card):
const float FPS[] = {0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.08, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.5, 0.6, 0.8, 1, 1.2, 1.5, 2, 2.5, 3, 3.5, 4};
// Number of shots parameter (to be used in 1-point stacking):
const short N_SHOTS[] = {2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 75, 100, 125, 150, 175, 200, 250, 300, 400, 500, 600};
// Two delay parameters for the non-continuous stacking mode (initiated with "#0"):
// The length of the first delay table:
const short N_FIRST_DELAY = 7;
// First delay in non-continuous stacking (from the moment rail stops until the shot is initiated), in seconds:
const float FIRST_DELAY[N_FIRST_DELAY] = {0.5, 1, 1.5, 2, 3, 4, 8};
// The length of the first delay table:
const short N_SECOND_DELAY = 7;
// Second delay in non-continuous stacking (from the shot initiation until the rail starts moving again), in seconds
// (This should be always longer than the camera exposure time)
const float SECOND_DELAY[N_SECOND_DELAY] = {0.5, 1, 1.5, 2, 3, 4, 8};
// Table of possible values for accel_factor parameter:
const byte N_ACCEL_FACTOR = 3;
const byte ACCEL_FACTOR[N_ACCEL_FACTOR] = {1, 3, 6};
// Table for N_timelapse parameter (number of stacking sequences in the timelapse mode); 1 means no timelapse (just one stack):
const byte N_N_TIMELAPSE = 7;
const short N_TIMELAPSE[N_N_TIMELAPSE] = {1, 3, 10, 30, 100, 300, 999};
// Table for dt_timelapse parameter (time in seconds between different stacks in timelapse mode; if it is shorter than a single stack time, the latter is used)
const byte N_DT_TIMELAPSE = 9;
const short DT_TIMELAPSE[N_DT_TIMELAPSE] = {1, 3, 10, 30, 100, 300, 1000, 3000, 9999};


//////////////////////////////////////////// Normally you shouldn't modify anything below this line ///////////////////////////////////////////////////

//////// LCD stuff: ////////
// Create a pcd8544 object.
// The LCD has 6 lines (rows) and 14 columns
// Pin 10 has to be unused (will be used internally)
#ifdef SOFTWARE_SPI
// Software SPI emulation:
pcd8544 lcd(PIN_LCD_DC, PIN_LCD_RST, PIN_LCD_SCE, PIN_LCD_DN_, PIN_LCD_SCL);
#else
// Hardware SPI
pcd8544 lcd(PIN_LCD_DC, PIN_LCD_RST, PIN_LCD_SCE);
#endif

// MM per microstep:
const float MM_PER_MICROSTEP = MM_PER_ROTATION / ((float)MOTOR_STEPS * (float)N_MICROSTEPS);
// Number of microsteps per rotation
const COORD_TYPE MICROSTEPS_PER_ROTATION = MOTOR_STEPS * N_MICROSTEPS;
// Breaking distance in internal units (microsteps):
const float BREAKING_DISTANCE = MICROSTEPS_PER_ROTATION * BREAKING_DISTANCE_MM / (1.0 * MM_PER_ROTATION);
const float SPEED_SCALE = MICROSTEPS_PER_ROTATION / (1.0e6 * MM_PER_ROTATION); // Conversion factor from mm/s to usteps/usecond
// Speed limit in internal units (microsteps per microsecond):
const float SPEED_LIMIT = SPEED_SCALE * SPEED_LIMIT_MM_S;
const float SPEED_LIMIT2 = SPEED_SCALE * SPEED_LIMIT2_MM_S;
// Maximum acceleration/deceleration allowed, in microsteps per microseconds^2 (a float)
// (This is a limiter, to minimize damage to the rail and motor)
const float ACCEL_LIMIT = SPEED_LIMIT * SPEED_LIMIT / (2.0 * BREAKING_DISTANCE);
// Speed small enough to allow instant stopping (such that stopping within one microstep is withing ACCEL_LIMIT):
// 2* - to make goto accurate, but with higher decelerations at the end
// Currently not used
const float SPEED_SMALL = 2 * sqrt(2.0 * ACCEL_LIMIT);
// A small float (to detect zero speed):
const float SPEED_TINY = 1e-4 * SPEED_LIMIT;
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
const float MAXIMUM_FPS = 1e6 / (float)(SHUTTER_TIME_US + SHUTTER_ON_DELAY + SHUTTER_OFF_DELAY + 2000);
// If defined, will be using my module to make sure that my physical microsteps always correspond to the program coordinates
// (this is needed to fix the problem when some Arduino loops are longer than the time interval between microsteps, which results in skipped steps)
// My solution: every time we detect a skipped microstep in motor_control, we backtrack a bit in time (by modifying variable g.dt_backlash) until the
// point when a single microstep was supposed to happen, and use this time lag correction until the moving has stopped. If more steps are skipped,
// this will keep increasing the time lag. As a result, my rail position will always be precise, but my timings might get slightly behind, and my actual
// speed might get slightly lower than what program thinks it is.
#define PRECISE_STEPPING
// Only matters if BACKLASH is non-zero. If defined, pressing the rewind key ("1") for a certain length of time will result in the travel by the same
// amount as when pressing fast-forward ("A") for the same period of time, with proper backlash compensation. This should result in smoother user experience.
// If undefined, to rewind by the same amount,
// one would have to press the rewind key longer (compared to pressing fast-forward key), to account for backlash compensation.
#define EXTENDED_REWIND


// Structure to have custom parameters saved to EEPROM
struct regist
{
  byte i_n_shots;
  byte i_mm_per_frame;
  byte i_fps;
  byte i_first_delay;
  byte i_second_delay;
  byte i_accel_factor;
  byte i_n_timelapse;
  byte i_dt_timelapse;
  byte mirror_lock;
  byte backlash_on;
  byte straight;
  byte save_energy;
  COORD_TYPE point1;
  COORD_TYPE point2;
};
  // Just in case adding a 1-byte if SIZE_REG is odd, to make the total regist size even (I suspect EEPROM wants data to have even number of bytes):
short SIZE_REG = sizeof(regist);

const short dA = sizeof(COORD_TYPE);

// EEPROM addresses: make sure they don't go beyong the Arduino Uno EEPROM size of 1024!
const int ADDR_POS = 0;  // Current position (float, 4 bytes)
const int ADDR_CALIBRATE = ADDR_POS + 4; // If =3, full limiter calibration will be done at the beginning (1 byte)
//!!! For some reason +1 doesn't work here, but +2 does, depsite the fact that the previous variable is 1-byte long:
const int ADDR_LIMIT1 = ADDR_CALIBRATE + 2; // pos_short for the foreground limiter (2 bytes)
const int ADDR_LIMIT2 = ADDR_LIMIT1 + dA; // pos_short for the background limiter (2 bytes)
const int ADDR_I_N_SHOTS = ADDR_LIMIT2 + dA;  // for the i_n_shots parameter
const int ADDR_I_MM_PER_FRAME = ADDR_I_N_SHOTS + 2; // for the i_mm_per_frame parameter;
const int ADDR_I_FPS = ADDR_I_MM_PER_FRAME + 2; // for the i_fps parameter;
const int ADDR_POINT1 = ADDR_I_FPS + 2; // Point 1 for 2-points stacking
const int ADDR_POINT2 = ADDR_POINT1 + dA; // Point 2 for 2-points stacking
const int ADDR_STRAIGHT = ADDR_POINT2 + dA; // g.straight value
const int ADDR_SAVE_ENERGY = ADDR_STRAIGHT + 2; // g.save_energy value
const int ADDR_BACKLIGHT = ADDR_SAVE_ENERGY + 2;  // backlight level
const int ADDR_REG1 = ADDR_BACKLIGHT + 2;  // register1
const int ADDR_REG2 = ADDR_REG1 + SIZE_REG;  // register2
const int ADDR_REG3 = ADDR_REG2 + SIZE_REG;  // register3
const int ADDR_REG4 = ADDR_REG3 + SIZE_REG;  // register4
const int ADDR_REG5 = ADDR_REG4 + SIZE_REG;  // register5
const int ADDR_I_FIRST_DELAY = ADDR_REG5 + SIZE_REG;  // for the FIRST_DELAY parameter
const int ADDR_I_SECOND_DELAY = ADDR_I_FIRST_DELAY + 2;  // for the SECOND_DELAY parameter
const int ADDR_MIRROR_LOCK = ADDR_I_SECOND_DELAY + 2;  // for g.mirror_lock
const int ADDR_BACKLASH_ON = ADDR_MIRROR_LOCK + 2; // for g.backlash_on
const int ADDR_I_ACCEL_FACTOR = ADDR_BACKLASH_ON + 2; // for g.i_accel_factor
const int ADDR_I_N_TIMELAPSE = ADDR_I_ACCEL_FACTOR + 2; // for g.i_n_timelaspe
const int ADDR_I_DT_TIMELAPSE = ADDR_I_N_TIMELAPSE + 2; // for g.i_dt_timelaspe

// 2-char bitmaps to display the battery status; 4 levels: 0 for empty, 3 for full:
const uint8_t battery_char [][12] = {
  {0xfe, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0xfe, 0x38}, // level 0 (empty)
  {0xfe, 0x82, 0xba, 0xb2, 0xa2, 0x82, 0x82, 0x82, 0x82, 0x82, 0xfe, 0x38}, // level 1 (1/3 charge)
  {0xfe, 0x82, 0xba, 0xba, 0xba, 0xba, 0xb2, 0xa2, 0x82, 0x82, 0xfe, 0x38}, // level 2 (2/3 charge)
  {0xfe, 0x82, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0x82, 0xfe, 0x38}  // level 3 (full charge)
};
// 2-char bitmaps to display rewind/fast-forward symbols:
const uint8_t rewind_char[] = {0x10, 0x38, 0x54, 0x92, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00};
const uint8_t forward_char[] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x92, 0x54, 0x38, 0x10, 0x00};

// All global variables belong to one structure - global:
struct global
{
  // Variables used to communicate between modules:
  unsigned long t;  // Time in us measured at the beginning of motor_control() module
  byte moving;  // 0 for stopped, 1 when moving; can only be set to 0 in motor_control()
  float speed1; // Target speed, in microsteps per microsecond
  float speed;  // Current speed (negative, 0 or positive)
  char accel; // Current acceleration index. Allowed values: -2,1,0,1,2 . +-2 correspond to ACCEL_LIMIT, +-1 correspond to ACCEL_SMALL
  
  byte i_accel_factor; // Index for accel_factor  
  float accel_v[5]; // Five possible floating point values for acceleration
  float pos;  // Current position (in microsteps). Should be stored in EEPROM before turning the controller off, and read from there when turned on
  float pos_old; // Last position, in the previous arduino loop
  COORD_TYPE pos_short_old;  // Previously computed position
  float pos0;  // Last position when accel changed
  unsigned long t0; // Last time when accel changed
  float speed0; // Last speed when accel changed
  float speed_old; // speed at the previous step
  float pos_stop; // Current stop position if breaked
  float pos_stop_old; // Previously computed stop position if breaked
  COORD_TYPE pos_limiter_off; // Position when after hitting a limiter, breaking, and moving in the opposite direction the limiter goes off
  unsigned long t_key_pressed; // Last time when a key was pressed
  unsigned long int t_last_repeat; // Last time when a key was repeated (for parameter change keys)
  short N_repeats; // Counter of key repeats
  unsigned long int t_display; // time since the last display refresh (only when not moving)
  unsigned char calibrate; // =3 when both limiters calibration is required (only the very first use); =1/2 when only the fore/background limiter (limit1/2) should be calibrated
  unsigned char calibrate_init; // Initial value of g.calibrate (matters only for the first calibration, calibrate=3)
  unsigned char calibrate_flag; // a flag for each leg of calibration: 0: no calibration; 1: breaking after hitting a limiter; 2: moving in the opposite direction (limiter still on);
  // 3: still moving, limiter off; 4: hit the second limiter; 5: rewinding to a safe area
  unsigned char calibrate_warning; // 1: pause calibration until any key is pressed, and display a warning
  COORD_TYPE limit1; // pos_short for the foreground limiter
  COORD_TYPE limit2; // pos_short for the background limiter
  COORD_TYPE limit_tmp; // temporary value of a new limit when rail hits a limiter
  unsigned char breaking;  // =1 when doing emergency breaking (e.g. to avoid hitting the limiting switch); disables the keypad
  unsigned char travel_flag; // =1 when travel was initiated
  float pos_goto; // position to go to
  byte moving_mode; // =0 when using speed_change, =1 when using go_to
  byte pos_stop_flag; // flag to detect when motor_control is run first time
  char key_old;  // peviously pressed key; used in keypad()
  COORD_TYPE point1;  // foreground point for 2-point focus stacking
  COORD_TYPE point2;  // background point for 2-point focus stacking
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
  unsigned long t_shutter; // Time when the camera shutter was triggered
  unsigned long t_shutter_off; // Time when the camera shutter was switched off
  unsigned long t_AF; // Time when the camera AF was triggered
  byte i_mm_per_frame; // counter for mm_per_frame parameter;
  byte i_fps; // counter for fps parameter;
  byte i_n_shots; // counter for n_shots parameter;
  byte i_first_delay; // counter for FIRST_DELAY parameter
  byte i_second_delay; // counter for SECOND_DELAY parameter
  byte i_n_timelapse; // counter for N_TIMELAPSE parameter
  byte i_dt_timelapse; // counter for DT_TIMELAPSE parameter
  char direction; // -1/1 for reverse/forward directions of moving
  char buffer[15];  // char buffer to be used for lcd print; 1 more element than the lcd width (14)
  unsigned long t_comment; // time when commment line was triggered
  byte comment_flag; // flag used to trigger the comment line briefly
  byte error; // error code (no error if 0); 1: initial limiter on or cable disconnected; 2: battery drained; non-zero value will disable the rail (with some exceptions)
  byte backlight; // backlight level; 0,1 for now
  struct regist reg; // Custom parameters register
  COORD_TYPE coords_change; // if >0, coordinates have to change (because we hit limit1, so we should set limit1=0 at some point)
  byte start_stacking; // =1 if we just initiated focus stacking, =2 when AF is triggered initially, =3 after CONT_STACKING_DELAY delay in continuous mode, =0 when no stacking
  byte make_shot; // =1 if we just initiated a shot; 0 otherwise
  unsigned long t_shot; // the time shot was initiated
  unsigned long int t0_stacking; // time when stacking was initiated;
  byte paused; // =1 when 2-point stacking was paused, after hitting any key; =0 otherwise
  COORD_TYPE BL_counter; // Counting microsteps made in the bad (negative) direction. Possible values 0...BACKLASH. Each step in the good (+) direction decreases it by 1.
  byte started_moving; // =1 when we just started moving (the first loop), 0 otherwise
  byte backlashing; // A flag to ensure that backlash compensation is uniterrupted (except for emergency breaking, #B); =1 when BL compensation is being done, 0 otherwise
  byte continuous_mode; // 2-point stacking mode: =0 for a non-continuous mode, =1 for a continuous mode
  byte noncont_flag; // flag for non-continuous mode of stacking; 0: no stacking; 1: initiated; 2: first shutter trigger; 3: second shutter; 4: go to the next frame
  unsigned long t_old;
  float speed_limit = SPEED_LIMIT;  // Current speed limit, in internal units. Determined once, when the device is powered up
  byte setup_flag; // Flag used to detect if we are in the setup section (then the value is 1; otherwise 0)
  byte alt_flag; // 0: normal display; 1: alternative display (when pressing *)
  byte straight;  // 0: reversed rail (PIN_DIR=LOW is positive); 1: straight rail (PIN_DIR=HIGH is positive)
  char* rev_char; // "R" if rail revered, " " otherwise
  byte backlash_init; // 1: initializing a full backlash loop; 2: initializing a rail reverse
  byte mirror_lock; // 1: mirror lock is used in non-continuous stacking; 0: not used; 2: similar to 0, but using SHUTTER_ON_DELAY2, SHUTTER_OFF_DELAY2 instead of SHUTTER_ON_DELAY, SHUTTER_OFF_DELAY
  byte disable_limiters; // 1: to temporarily disable limiters (not saved to EEPROM)
  char buf6[6]; // Buffer to store the stacking length for displaying
  char buf7[7];
  short timelapse_counter; // Counter for the time lapse feature
  unsigned long t_mil; // millisecond accuracy timer; used to set up timelapse stacks
  unsigned long t0_mil; // millisecond accuracy timer; used to set up timelapse stacks
  byte end_of_stacking; // =1 when we are done with stacking (might still be moving, in continuoius mode)  
  byte timelapse_mode; // =1 during timelapse mode, 0 otherwise
  COORD_TYPE backlash; // current value of backlash in microsteps (can be either 0 or BACKLASH)
  byte backlash_on; // =1 when g.backlash=BACKLASH; =0 when g.backlash=0.0
  byte save_energy; // =0: always using the motor's torque, even when not moving (should improve accuracy and holding torque); =1: save energy (only use torque during movements)
#ifdef PRECISE_STEPPING
  unsigned long dt_backlash;
#endif
#ifdef EXTENDED_REWIND
  byte no_extended_rewind;
#endif
#ifdef TIMING
  unsigned long i_timing;
  unsigned long t0_timing;
  short dt_max;
  short dt_min;
  short bad_timing_counter; // How many loops in the last movement were longer than the shortest microstep interval allowed
#endif
};

struct global g;

#endif


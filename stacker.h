/* Sergey Mashchenko 2015

   User header file

   To be used with automated macro rail for focus stacking

Basic functionality to implement:
 - Error message if the battery level is below acceptable.

Issues to address:
 - Position accuracy after turning off/on again: the motor will likely move to the
 nearest (or in a certain direction?) full stop, creating an error of that size.
 I probably should only use full stop positions when stopped (how to figure out
 which ones are full stop?)
 - Similar issue when using SAVE_ENERGY: I should use full stops, or the error will
 accumulate every time I stop.
 - Apparently stepper motors can't change direction at atarbitrary microsteps, perhaps not
 even at all full steps - needs to be figured out and implemented.

*/

#ifndef STACKER_H
#define STACKER_H

#define VERSION "0.09"

// For debugging with serial monitor:
//#define DEBUG

// For timing the main loop:
//#define TIMING
// Compute and display timing results every that many loops:
const unsigned long N_TIMING = 10000;

// Motor debugging mode: limiters disabled (used for finetuning the motor alignment with the macro rail knob, finding the minimum motor current etc.)
#define MOTOR_DEBUG

// Battery debugging mode (prints actual voltage per AA battery in the status line; needed to determine the lowest voltage parameter, V_LOW - see below)
//#define BATTERY_DEBUG

// If undefined, lcd will not be used
#define LCD

// Backlash compensation (in mm); positive direction (towards background) is assumed to be the good one (no BL compensation required);
// all motions moving in the bad (negative) direction at the end will need some BL compensation.
// All go_to() motions always get maximum BL compensation just in case (as it doesn't pose a problem); small rewinds (in negative direction) might
// end up doing a fractional BL compensation, using the simplest BL model (the model: rail physically doesn't move until rewinding the full BACKLASH amount,
// and then instantly starts moving; same when moving to the positive direction after moving to the bad direction).
// The algorithm guarantees that every time rail comes to rest, it is fully BL compensated (so the code coordinate = physical coordinate).
// Should be determined experimentally: too small values will produce visible backlash (two or more frames at the start of the stacking
// sequence will look alsmost identical).
// Set to zero to disable BL compensation.
const float BACKLASH_MM = 0.0;

// Options controlling compilation:

// If defined, motor will be parked when not moving (probably will affect the accuracy of positioning)
#define SAVE_ENERGY

// If defined, will be using my experimental module to make sure that my physical microsteps always correspond to the program coordinates
// (this is needed to fix the problem when some Arduino loops are longer than the time interval between microsteps, which results in skipped steps)
// My solution: every time we detect a skipped microstep in motor_control, we backtrack a bit in time (by modifying variable g.delta_t) until the
// point when a single microstep was supposed to happen, and use this time lag correction until the moving has stopped. If more steps are skipped,
// this will keep increasing the time lag. As a result, my rail position will always be precise, but my timings might get slightly behind, and my actual
// speed might get slightly lower than what program thinks it is.
// EDIT: It is on hold for now, as step skipping seems to be extremely rare now, after code profiling and hardware SPI implemented
#define PRECISE_STEPPING

//////// Parameters to be set only once //////////

//////// Pin assignment ////////
// We are using the bare minimum of arduino pins for stepper driver:
const short PIN_STEP = 0;
const short PIN_DIR = 1;
const short PIN_ENABLE = 2;  // LOW: enable motor; HIGH: disable motor (to save energy)
// LCD pins (Nokia 5110): following resistor scenario in https://learn.sparkfun.com/tutorials/graphic-lcd-hookup-guide
const short PIN_LCD_DC = 5;  // Via 10 kOhm resistor
const short PIN_LCD_RST = 6;  // Via 10 kOhm resistor
// Hardware v1.1: the chip select LCD pin (SCE, CE) is now soldered to ground via 10k pulldown resistor, to save one Arduino pin; here assigning a bogus value
// (I modified the pcd8544 library to disable the use of this pin)
const short PIN_LCD_SCE = 100;
const short PIN_LCD_LED = 9;  // Via 330 Ohm resistor
const short PIN_LCD_DN_ = 11;  // Via 10 kOhm resistor
const short PIN_LCD_SCL = 13;  // Via 10 kOhm resistor
// Pin to read digital input from the two limiting switches (normally LOW; HIGH when limiters are triggered)
const short PIN_LIMITERS = 8;
// Pin to trigger camera shutter:
const short PIN_SHUTTER = 3;
// Analogue pin for the battery life sensor:
#define PIN_BATTERY A0

// Scaling coefficient to derive the battery voltage (depends on the resistance of the two dividing resistors, R1 and R2.
// Assuming R2 is the one directly connected to "+" of the battery, the scaler is (R1+R2)/R2. R1+R2 should be ~0.5M)
// To reduce reading noise, a 0.1uF capacitor has to be soldered parallel to R1.
// The second factor is 5.0V/1024/8 (assumes 8 AA batteries) - don't change it.
const float VOLTAGE_SCALER = 2.7273 * 5.0/1024.0/8.0;
// Critically low voltage, per AA battery (when V becomes lower than this, the macro rail is disabled)
// Set it slightly above the value when the rail with camera starts skipping steps
const float V_LOW = 1.125;
// Highest voltage from a freshly charged AA battery:
const float V_HIGH = 1.4;

// Keypad stuff:
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

// LCD stuff
// Create a pcd8544 object.
// Hardware SPI will be used.
// sdin (MOSI) is on pin 11 and sclk on pin 13.
// The LCD has 6 lines (rows) and 14 columns
// Pin 10 has to be unused (will be used internally)
pcd8544 lcd(PIN_LCD_DC, PIN_LCD_RST, PIN_LCD_SCE);

// Number of full steps per rotation for the stepper motor:
const short MOTOR_STEPS = 200;
// Number of microsteps in a step (default for EasyDriver is 8):
const short N_MICROSTEPS = 8;
// Macro rail parameter: travel distance per one rotation, in mm (3.98mm for Velbon Mag Slider):
const float MM_PER_ROTATION = 3.98;

//////// Parameters which might need to be changed ////////
// Speed limiter, in mm/s. Higher values will result in lower torques and will necessitate larger travel distance
// between the limiting switches and the physical limits of the rail. In addition, too high values will result
// in Arduino loop becoming longer than inter-step time interval, which can screw up the algorithm.
// 5 mm/s seems to be a reasonable compromize, for my motor and rail.
const float SPEED_LIMIT_MM_S = 5;
// Breaking distance (mm) for the rail when stopping while moving at the fastest speed (SPEED_LIMIT)
// This will determine the maximum acceleration/deceleration allowed for any rail movements - important
// for reducing the damage to the (mostly plastic) rail gears. Make sure that this distance is smaller
// than the smaller distance of the two limiting switches (between the switch actuation and the physical rail limits)
const float BREAKING_DISTANCE_MM = 2.0;
// Padding (in microsteps) for a soft limit, before hitting the limiters:
const short LIMITER_PAD = 400;
// A bit of extra padding (in microsteps) when calculating the breaking distance before hitting the limiters (to account for inaccuracies of go_to()):
const short LIMITER_PAD2 = 100;
const unsigned long SHUTTER_TIME_US = 50000; // Time to keep the shutter button pressed (us)
const short DELTA_LIMITER = 400; // In calibration, after hitting the first limiter, breaking, and moving in the opposite direction, travel this many microsteps after the limiter goes off again, before starting checking the limiter again

// Delay in microseconds between LOW and HIGH writes to PIN_STEP (should be >=1 for Easydriver; but arduino only guarantees delay accuracy for >=3)
const short STEP_LOW_DT = 3;
// Delay after writing to PIN_ENABLE, ms (only used in SAVE_ENERGY mode):
const short ENABLE_DELAY_MS = 3;

const unsigned long COMMENT_DELAY = 1000000; // time in us to keep the comment line visible
const unsigned long T_KEY_LAG = 500000; // time in us to keep a parameter change key pressed before it will start repeating
const unsigned long T_KEY_REPEAT = 200000; // time interval in us for repeating with parameter change keys
const unsigned long DISPLAY_REFRESH_TIME = 1000000; // time interval in us for refreshing the whole display (only when not moving). Mostly for updating the battery status
// If you focus stacking skips the very first shot, increase this parameter; 200000 works for Canon 50D
const unsigned long STACKING_DELAY = 200000; // delay in us before initiating stacking/making first shot and starting the movement; also the shutter open time for the first shot

// INPUT PARAMETERS:
// Number of values for the input parameters (mm_per_frame etc):
const short N_PARAMS = 25;
//  Mm per frame parameter (determined by DoF of the lens)
const float MM_PER_FRAME[] = {0.005, 0.006, 0.008, 0.01, 0.015, 0.02, 0.025, 0.03, 0.04, 0.05, 0.06, 0.08, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.8, 1, 1.5, 2, 2.5};
// Frame per second parameter (Canon 50D can do up to 4 fps when Live View is not enabled, for 20 shots using 1000x Lexar card):
const float FPS[] = {0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.08, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.5, 0.6, 0.8, 1, 1.2, 1.5, 2, 2.5, 3, 3.5, 4};
// Number of shots parameter (to be used in 1-point stacking):
const short N_SHOTS[] = {2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 75, 100, 125, 150, 175, 200, 250, 300, 400, 500, 600};

//////// Don't modify these /////////
// MM per microstep:
const float MM_PER_MICROSTEP = MM_PER_ROTATION / ((float)MOTOR_STEPS * (float)N_MICROSTEPS);
// Number of microsteps per rotation
const short MICROSTEPS_PER_ROTATION = MOTOR_STEPS * N_MICROSTEPS;
// Breaking distance in internal units (microsteps):
const float BREAKING_DISTANCE = MICROSTEPS_PER_ROTATION * BREAKING_DISTANCE_MM / (1.0 * MM_PER_ROTATION);
const float SPEED_SCALE = MICROSTEPS_PER_ROTATION / (1.0e6 * MM_PER_ROTATION); // Conversion factor from mm/s to usteps/usecond
// Speed limit in internal units (microsteps per microsecond):
const float SPEED_LIMIT = SPEED_SCALE * SPEED_LIMIT_MM_S;
// Maximum acceleration/deceleration allowed, in microsteps per microseconds^2 (a float)
// This is a limiter, to minimize damage to the rail and motor
const float ACCEL_LIMIT = SPEED_LIMIT * SPEED_LIMIT / (2.0 * BREAKING_DISTANCE);
// Speed small enough to allow instant stopping (such that stopping within one microstep is withing ACCEL_LIMIT):
//!!! 2* - to make goto accurate, but with higher decelerations at the end
const float SPEED_SMALL = 2 * sqrt(2.0 * ACCEL_LIMIT);
// A small float (to detect zero speed):
const float SPEED_TINY = 1e-3 * SPEED_SMALL;
// Backlash in microsteps:
const short BACKLASH = (short)(BACKLASH_MM / MM_PER_MICROSTEP);
// Slightly smaller value of backlash, to be used in motor_control
const short BACKLASH1 = (short)(0.9 * BACKLASH_MM / MM_PER_MICROSTEP);

// Structure to have custom parameters saved to EEPROM
struct regist
{
  short i_n_shots;
  short i_mm_per_frame;
  short i_fps;
  short point1;
  short point2;
};
short SIZE_REG=sizeof(regist);

// EEPROM addresses:
const int ADDR_POS = 0;  // Current position (float, 4 bytes)
const int ADDR_CALIBRATE = ADDR_POS + 4; // If =3, full limiter calibration will be done at the beginning (1 byte)
//!!! For some reason +1 doesn't work here, but +2 does, depsite the fact that the previous variable is 1-byte long:
const int ADDR_LIMIT1 = ADDR_CALIBRATE + 2; // pos_short for the foreground limiter; should be 0? (2 bytes)
const int ADDR_LIMIT2 = ADDR_LIMIT1 + 2; // pos_short for the background limiter (2 bytes)
const int ADDR_I_N_SHOTS = ADDR_LIMIT2 + 2;  // for the i_n_shots parameter
const int ADDR_I_MM_PER_FRAME = ADDR_I_N_SHOTS + 2; // for the i_mm_per_frame parameter;
const int ADDR_I_FPS = ADDR_I_MM_PER_FRAME + 2; // for the i_fps parameter;
const int ADDR_POINT1 = ADDR_I_FPS + 2; // Point 1 for 2-points stacking
const int ADDR_POINT2 = ADDR_POINT1 + 2; // Point 2 for 2-points stacking
const int ADDR_POINTS_BYTE = ADDR_POINT2 + 2; // points_byte value
const int ADDR_BACKLIGHT = ADDR_POINTS_BYTE + 2;  // backlight level
const int ADDR_REG1 = ADDR_BACKLIGHT + 2;  // register1
const int ADDR_REG2 = ADDR_REG1 + SIZE_REG;  // register2
const int ADDR_REG3 = ADDR_REG2 + SIZE_REG;  // register3

// 2-char bitmaps to display the battery status; 4 levels: 0 for empty, 3 for full:
uint8_t battery_char [][12] = {
{0xfe, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0xfe, 0x38}, // level 0 (empty)
{0xfe, 0x82, 0xba, 0xb2, 0xa2, 0x82, 0x82, 0x82, 0x82, 0x82, 0xfe, 0x38}, // level 1 (1/3 charge)
{0xfe, 0x82, 0xba, 0xba, 0xba, 0xba, 0xb2, 0xa2, 0x82, 0x82, 0xfe, 0x38}, // level 2 (2/3 charge)
{0xfe, 0x82, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0xba, 0x82, 0xfe, 0x38}  // level 3 (full charge)
};
// 2-char bitmaps to display rewind/fast-forward symbols:
uint8_t rewind_char[] = {0x10, 0x38, 0x54, 0x92, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00};
uint8_t forward_char[] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x92, 0x54, 0x38, 0x10, 0x00};

// All global variables belong to one structure - global:
struct global
{
  // Variables used to communicate between modules:
  unsigned long t;  // Time in us measured at the beginning of motor_control() module
  short moving;  // 0 for stopped, 1 when moving; can only be set to 0 in motor_control()
  float speed1; // Target speed, in microsteps per microsecond (
  float speed;  // Current speed (negative, 0 or positive)
  short accel; // Current acceleration, in ACCEL_LIMIT units. Allowed values: -1,0,1
  float pos;  // Current position (in microsteps). Should be stored in EEPROM before turning the controller off, and read from there when turned on
  float pos_old; // Last position, in the previous arduino loop
  short pos_short_old;  // Previously computed position
  float pos0;  // Last position when accel changed
  unsigned long t0; // Last time when accel changed
  float speed0; // Last speed when accel changed
  float speed_old; // speed at the previous step
  float pos_stop; // Current stop position if breaked
  float pos_stop_old; // Previously computed stop position if breaked
  short pos_limiter_off; // Position when after hitting a limiter, breaking, and moving in the opposite direction the limiter goes off
  unsigned long t_key_pressed; // Last time when a key was pressed
  unsigned long int t_last_repeat; // Last time when a key was repeated (for parameter change keys)
  int N_repeats; // Counter of key repeats
  unsigned long int t_display; // time since the last display refresh (only when not moving)

  unsigned char calibrate; // =3 when both limiters calibration is required (only the very first use); =1/2 when only the fore/background limiter (limit1/2) should be calibrated
  unsigned char calibrate_init; // Initial value of g.calibrate (matters only for the first calibration, calibrate=3)
  unsigned char calibrate_flag; // a flag for each leg of calibration: 0: no calibration; 1: breaking after hitting a limiter; 2: moving in the opposite direction (limiter still on);
  // 3: still moving, limiter off; 4: hit the second limiter; 5: rewinding to a safe area
  unsigned char calibrate_warning; // 1: pause calibration until any key is pressed, and display a warning
  short limit1; // pos_short for the foreground limiter
  short limit2; // pos_short for the background limiter
  short limit_tmp; // temporary value of a new limit when rail hits a limiter
  unsigned char breaking;  // =1 when doing emergency breaking (to avoid hitting the limiting switch); disables the keypad
  unsigned char travel_flag; // =1 when travel was initiated
  float pos_goto; // position to go to
  short moving_mode; // =0 when using speed_change, =1 when using go_to
  short pos_stop_flag; // flag to detect when motor_control is run first time
  char key_old;  // peviously pressed key; used in keypad()
  short point1;  // foreground point for 2-point focus stacking
  short point2;  // background point for 2-point focus stacking
  short starting_point; // The starting point in the focus stacking with two points
  short destination_point; // The destination point in the focus stacking with two points
  short stacking_direction; // 1/-1 for direct/reverse stacking direction
  short stacker_mode;  // 0: default (rewind etc.); 1: pre-winding for focus stacking; 2: focus stacking itself; 3: single-point stacking
  float msteps_per_frame; // Microsteps per frame for focus stacking
  short Nframes; // Number of frames for 2-point focus stacking
  short frame_counter; // Counter for shots
  short pos_to_shoot; // Position to shoot the next shot during focus stacking
  short shutter_on; // flag for camera shutter state: 0/1 corresponds to off/on
  unsigned long t_shutter; // Time when the camera shutter was triggered
  short i_mm_per_frame; // counter for mm_per_frame parameter;
  short i_fps; // counter for fps parameter;
  short i_n_shots; // counter for n_shots parameter;
  short direction; // -1/1 for reverse/forward directions of moving
  char buffer[16];  // char buffer to be used for lcd print; 2 more elements than the lcd width (14)
  char p_buffer[16]; // keeps a copy of the buffer used to print position on display
  byte points_byte; // two-points status encoded: 0/1/2/3 when no / only fg / only bg / both points are defined
  unsigned long t_comment; // time when commment line was triggered
  byte comment_flag; // flag used to trigger the comment line briefly
  KeyState state_old;  // keeping old key[0] state
  KeyState state1_old;  // keeping old key[1] state
  short error; // error code (no error if 0); 1: initial limiter on or cable disconnected; 2: battery drained; non-zero value will disable the rail (with some exceptions)
  short backlight; // backlight level; 0,1,2 for now
  struct regist reg1; // Custom parameters saved in register1
  struct regist reg2; // Custom parameters saved in register2
  struct regist reg3; // Custom parameters saved in register3
  short coords_change; // if >0, coordinates have to change (because we hit limit1, so we should set limit1=0 at some point)
  short start_stacking; // =1 if we just initiated focust stacking, 0 otherwise; used to create an initial delay befor emoving, to ensure first shot is taken
  unsigned long int t0_stacking; // time when stacking was initiated;
  short paused; // =1 when 2-point stacking was paused, after hitting any key; =0 otherwise
  short just_paused; // a "just paused" state - before making any movements (step a single frame etc.)
  short BL_counter; // Counting microsteps mad in the bad (negative) direction. Possible values 0...BACKLASH. Each step in the good (+) direction decreases it by 1.
  short first_loop; // =1 during the first loop, 0 after that
  short started_moving; // =1 when we just started moving (the first loop), 0 otherwise
//  short display4_counter; // Loop counter, for displaying line 4
  short backlashing; // A flag to ensure that backlash compensation is uniterrupted (except for emergency breaking, #B); =1 when BL compensation is being done, 0 otherwise
  unsigned long t_old;
#ifdef PRECISE_STEPPING
  unsigned long dt_backlash;
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

#ifdef MOTOR_DEBUG
  short cplus1, cminus1, cplus2, cminus2, cmax, imax, istep, skipped_current, skipped_total;  
  short n_fixed, n_failed, n1, n2, n3, n4, k1, k2, k3;
#ifdef PRECISE_STEPPING
  unsigned long dt_backlash;
#endif
#endif
#ifdef DEBUG
  short flag=0;
#endif

#endif


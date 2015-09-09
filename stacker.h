/* Sergey Mashchenko 2015

   User header file

   To be used with automated macro rail for focus stacking

== Handling limiters (limiting switches) ==
There are two limiters - foreground (smaller pos) and background (larger pos).


Issues to address:
 - Position accuracy after turning off/on again: the motor will likely move to the
 nearest (or in a certain direction?) full stop, creating an error of that size.
 I probably should only use full stop positions when stopped (how to figure out
 which ones are full stop?)
 - Similar issue when using SAVE_ENERGY: I should use full stops, or the error will
 accumulate every time I stop.
 - Apparently stepper motors can't change direction at atarbitrary microsteps, perhaps not
 even at all full steps - needs to be figured out and implemented.
 - [DONE] Very good chance that go_to() will not be accurate to a microstep level (and that is
 required for correct stacking). Possible solution: always travel slightly shorter distance 
 (can be tricky if has to reverse direction), and switch to travelling at constant low
 speed (low enough that instant stopping - withing a single microstep - will be
 within ACCEL_LIMIT) when almost at the destionation. Then instantly stop when hitting the exact
 position.
 - [DONE] Change camera() to only trigger shutter between microsteps (to minimize vibrations).
   
*/

#ifndef STACKER_H
#define STACKER_H


#define VERSION "0.01"
// For debugging (motor doesn't work when debug is on!):
//#define DEBUG
// For timing the main loop:
//#define TIMING
// Compute and display timing results every that many loops:
const unsigned long N_TIMING = 10000;

// If undefined, lcd will not be used
//#define LCD

// Options controlling compilation:

// If defined, motor will be parked when not moving (probably will affect the accuracy of positioning)
// I think it makes sense to only use full stops when at rest in saving mode
#define SAVE_ENERGY
// If defined, each go_to() operation will move the rail slighly shorter distance (by DELTA_POS),
// and when at the end the low speed SPEED_SMALL is reached, deceleration stops, and motion proceeds
// at that low speed until the exact  target position is reached, at which point the rail stops
// instantly (SPEED_SMALL is small enough to ensure that deceleration stays within ACCEL_LIMIT)
//#define HIGH_ACCURACY

//////// Pin assignment ////////
// We are using the bare minimum of arduino pins for stepper driver:
const short PIN_STEP = 0;
const short PIN_DIR = 1;
const short PIN_ENABLE = 2;  // LOW: enable motor; HIGH: disable motor (to save energy)
// LCD pins (Nokia 5110): following resistor scenario in https://learn.sparkfun.com/tutorials/graphic-lcd-hookup-guide
const short PIN_LCD_DC = 5;  // Via 10 kOhm resistor
const short PIN_LCD_RST = 6;  // Via 10 kOhm resistor
const short PIN_LCD_SCE = 7;  // Via 1 kOhm resistor
const short PIN_LCD_LED = 9;  // Via 330 Ohm resistor
const short PIN_LCD_DN_ = 11;  // Via 10 kOhm resistor
const short PIN_LCD_SCL = 13;  // Via 10 kOhm resistor
// Pin to read digital input from the two limiting switches (normally LOW; HIGH when limiters are triggered)
const short PIN_LIMITERS = 8;
// Pin to trigger camera shutter:
const short PIN_SHUTTER = 3;
// Analogue pin for the battery life sensor:
#define PIN_BATTERY A0

//////// Parameters to be set only once //////////
// Number of full steps per rotation for the stepper motor:
const short MOTOR_STEPS = 200;
// Number of microsteps in a step (default for EasyDriver is 8):
const short N_MICROSTEPS = 8;
// Macro rail parameter: travel distance per one rotation, in mm:
const float MM_PER_ROTATION = 3.98;
// MM per microstep:
const float MM_PER_MICROSTEP = MM_PER_ROTATION / ((float)MOTOR_STEPS * (float)N_MICROSTEPS);

//////// Parameters which might need to be changed occasionally ////////
// Speed limiter, in mm/s
const float SPEED_LIMIT_MM_S = 5;
// Breaking distance (mm) for the rail when stopping while moving at the fastest speed (SPEED_LIMIT)
// This will determine the maximum acceleration/deceleration allowed for any rail movements - important
// for reducing the damage to the (mostly plastic) rail gears. Make sure that this distance is smaller
// than the smaller distance of the two limiting switches (between the switch actuation and the physical rail limits)
const float BREAKING_DISTANCE_MM = 2.0;
// Padding (in microsteps) for a soft limit, before hitting the limiters:
const short LIMITER_PAD = 400;
const unsigned long SHUTTER_TIME_US = 50000; // Time to keep the shutter button pressed (us)
const short DELTA_POS = 10; //In go_to, travel less than needed by this number of microsteps, to allow for precise positioning at the stop in motor_control()
const short DELTA_LIMITER = 400; // In calibration, after hitting the first limiter, breaking, and moving in the opposite direction, travel this many microsteps after the limiter goes off again, before starting checking the limiter again

// Delay in microseconds between LOW and HIGH writes to PIN_STEP (should be >=1 for Easydriver; but arduino only guarantees accuracy for >=3)
const short STEP_LOW_DT = 3;
// Delay after writing to PIN_ENABLE, ms (only used in SAVE_ENERGY mode):
const short ENABLE_DELAY_MS = 3;

const unsigned long COMMENT_DELAY = 1000000; // time in us to keep the comment line visible

// INPUT PARAMETERS:
// Number of values for the input parameters (mm_per_frame etc):
const short N_PARAMS = 16;
//  Mm per frame parameter (determined by DoF of the lens)
const float MM_PER_FRAME[] = {0.005, 0.0075, 0.01, 0.015, 0.02, 0.03, 0.04, 0.06, 0.08, 0.12, 0.16, 0.25, 0.38, 0.5, 0.75, 1.0};
//const float MM_PER_FRAME[N_PARAMS];
// Frame per second parameter:
const float FPS[] = {0.038, 0.05, 0.075, 0.1, 0.15, 0.2, 0.3, 0.4, 0.6, 0.8, 1.2, 1.6, 2.5, 3.8, 5.0, 6.3};
//const float FPS[N_PARAMS];
// Number of shots parameter (to be used in 1-point stacking):
const short N_SHOTS[] = {2, 3, 4, 5, 6, 8, 12, 16, 25, 38, 50, 75, 100, 150, 200, 300};
//const short N_SHOTS[N_PARAMS];

//////// Don't modify these /////////
// Number of microsteps per rotation
const short MICROSTEPS_PER_ROTATION = MOTOR_STEPS*N_MICROSTEPS;
// Breaking distance in internal units (microsteps):
const float BREAKING_DISTANCE = MICROSTEPS_PER_ROTATION*BREAKING_DISTANCE_MM/(1.0*MM_PER_ROTATION);
const float SPEED_SCALE = MICROSTEPS_PER_ROTATION/(1.0e6*MM_PER_ROTATION);  // Conversion factor from mm/s to usteps/usecond
// Speed limit in internal units (microsteps per microsecond):
const float SPEED_LIMIT = SPEED_SCALE*SPEED_LIMIT_MM_S;
// Maximum acceleration/deceleration allowed, in microsteps per microseconds^2 (a float)
// This is a limiter, to minimize damage to the rail and motor
const float ACCEL_LIMIT = SPEED_LIMIT*SPEED_LIMIT/(2.0*BREAKING_DISTANCE);
// Speed small enough to allow instant stopping (such that stopping within one microstep is withing ACCEL_LIMIT):
//!!! 2* - to make goto accurate, but with higher decelerations at the end
const float SPEED_SMALL = 2*sqrt(2.0*ACCEL_LIMIT);
// A small float (to detect zero speed):
const float SPEED_TINY = 1e-3*SPEED_SMALL;
const float SPEED1 = SPEED_LIMIT/sqrt(2.0);

// EEPROM addresses:
const short ADDR_POS = 0;  // Current position (float, 4 bytes)
const short ADDR_CALIBRATE = ADDR_POS+4;  // If =1, limiter calibration will be done at the beginning (1 byte)
const short ADDR_LIMIT1 = ADDR_CALIBRATE+1; // pos_short for the foreground limiter; should be 0? (2 bytes)
const short ADDR_LIMIT2 = ADDR_LIMIT1+2; // pos_short for the background limiter (2 bytes)
const short ADDR_I_N_SHOTS = ADDR_LIMIT2 + 2;  // for the i_n_shots parameter
const short ADDR_I_MM_PER_FRAME = ADDR_I_N_SHOTS + 2; // for the i_mm_per_frame parameter;
const short ADDR_I_FPS = ADDR_I_MM_PER_FRAME + 2; // for the i_fps parameter;
const short ADDR_POINT1 = ADDR_I_FPS + 2; // Point 1 for 2-points stacking
const short ADDR_POINT2 = ADDR_POINT1 + 2; // Point 2 for 2-points stacking
const short ADDR_POINTS_BYTE = ADDR_POINT2 + 2; // points_byte value

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
short pos_short_old;  // Previously computed position
float pos0;  // Last position when accel changed
unsigned long t0; // Last time when accel changed
float speed0; // Last speed when accel changed
float speed_old; // speed at the previous step
float pos_stop; // Current stop position if breaked
float pos_stop_old; // Previously computed stop position if breaked
short pos_limiter_off; // Position when after hitting a limiter, breaking, and moving in the opposite direction the limiter goes off

unsigned char abortMy=0; // immediately abort the loop if >0 (only if direction=0 - rail not moving)
unsigned char calibrate=0; // =3 when both limiters calibration is required; =1/2 when only the fore/background limiter (limit1/2) should be calibrated
unsigned char calibrate_init=0; // a copy of calibrate
unsigned char calibrate_flag=0; // a flag for each leg of calibration: 0: no calibration; 1: breaking after hitting a limiter; 2: moving in the opposite direction (limiter still on); 
// 3: still moving, limiter off; 4: hit the second limiter; 5: rewinding to a safe area
unsigned char limit_on; // =0/1 when the limiting switches are off/on
short limit1; // pos_short for the foreground limiter
short limit2; // pos_short for the background limiter
short limit_tmp; // temporary value of a new limit when rail activates a limiter
unsigned char breaking;  // =1 when doing emergency breaking (to avoid hitting the limiting switch)
unsigned char travel_flag; // =1 when travle was initiated
short pos_goto_short; // position to go to
short moving_mode; // =0 when using speed_change, =1 when using go_to
short pos_stop_flag; // flag to detect when motor_control is run first time
char key_old;  // peviously pressd key (can be NO_KEY); used in keypad()
short point1;  // foreground point for 2-point focus stacking
short point2;  // background point for 2-point focus stacking
short first_point; // The first point in the focus stacking with two points
short second_point; // The second point in the focus stacking with two points
short stacking_direction; // 1/-1 for direct/reverse stacking direction
short stacker_mode;  // 0: default (rewind etc.); 1: pre-winding for focus stacking; 2: focus stacking itself
//float fps;  // Frames per second parameter
//float mm_per_frame;  // Mm per shot parameter
float msteps_per_frame; // Microsteps per frame for focus stacking
short Nframes; // Number of frames for 2-point focus stacking
short frame_counter; // Counter for shots
short pos_to_shoot; // Position to shoot the next shot during focus stacking
short shutter_on; // flag for camera shutter: 0/1 corresponds to off/on
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
KeyState state_old;  // keeping old keypas state
//short limiter_engaged;  // =1 when hitting a limiter
#ifdef TIMING
unsigned long t_old;
unsigned long i_timing;
#endif

unsigned char flag; // for testing
};

struct global g;

// Keypad stuff:
const byte rows = 4; //four rows
const byte cols = 4; //three columns
char keys[rows][cols] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[rows] = {4, 10, 12, A1}; //connect to the row pinouts of the keypad (6,7,8,9 for mine)
byte colPins[cols] = {A2, A3, A4, A5}; //connect to the column pinouts of the keypad (2,3,4,5 for mine)
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );

// LCD stuff
// Create a pcd8544 object.
// Hardware SPI will be used.
// sdin (MOSI) is on pin 11 and sclk on pin 13.
// The LCD has 6 lines (rows) and 14 columns
pcd8544 lcd(PIN_LCD_DC, PIN_LCD_RST, PIN_LCD_SCE);

#endif


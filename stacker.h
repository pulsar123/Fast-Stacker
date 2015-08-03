/* Sergey Mashchenko 2015

   User header file

   To be used with automated macro rail for focus stacking
*/

#ifndef STACKER_H
#define STACKER_H

//#define DEBUG

// If defined, motor will be parked when not moving (probabli will affect the accuracy of positioning)
#define SAVE_ENERGY

//////// Pin assignment ////////
// We are using the bare minimum of arduino pins for stepper driver:
const short PIN_STEP = 0;
const short PIN_DIR = 1;
const short PIN_ENABLE = 2;  // LOW: enable motor; HIGH: disable motor (to save energy)
// LCD pins (Nokia 5110):
const short PIN_LCD_D_C = 5;
const short PIN_LCD_RST = 6;
const short PIN_LCD_SCE = 7;
const short PIN_LCD_LED = 9;
const short PIN_LCD_DN_ = 11;
const short PIN_LCD_SCL = 13;
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

//////// Parameters which might need to be changed occasionally ////////
// Speed limiter, in mm/s
const float SPEED_LIMIT_MM_S = 5;
// Breaking distance (mm) for the rail when stopping while moving at the fastest speed (SPEED_LIMIT)
// This will determine the maximum acceleration/deceleration allowed for any rail movements - important
// for reducing the damage to the (mostly plastic) rail gears. Make sure that this distance is smaller
// than the smaller distance of the two limiting switches (between the switch actuation and the physical rail limits)
const float BREAKING_DISTANCE_MM = 2.0;
// Padding (in microsteps) before hitting the limiters:
const short LIMITER_PAD = 400;
const unsigned long SHUTTER_TIME_US = 100000; // Time to keep the shutter button pressed (us)

// Delay in microseconds between LOW and HIGH writes to PIN_STEP (should be >=1 for Easydriver; but arduino only guarantees accuracy for >=3)
const short STEP_LOW_DT = 3;
// Delay after writing to PIN_ENABLE, ms (only used in SAVE_ENERGY mode):
const short ENABLE_DELAY_MS = 3;
// A small float (to detect zero speed):
const float SMALL = 1e-8;

//////// Don't modify these /////////
// Number of microsteps per rotation
const short MICROSTEPS_PER_ROTATION = MOTOR_STEPS*N_MICROSTEPS;
const float SPEED_SCALE = MICROSTEPS_PER_ROTATION/(1.0e6*MM_PER_ROTATION);  // Conversion factor from mm/s to usteps/usecond
// Speed limit in internal units (microsteps per microsecond):
const float SPEED_LIMIT = SPEED_SCALE*SPEED_LIMIT_MM_S;
// Speed small enough to allow instant stopping:
const float SPEED_SMALL = 0.01*SPEED_LIMIT;
const float SPEED1 = SPEED_LIMIT/sqrt(2.0);
// Breaking distance in internal units (microsteps):
const float BREAKING_DISTANCE = MICROSTEPS_PER_ROTATION*BREAKING_DISTANCE_MM/(1.0*MM_PER_ROTATION);
// Maximum acceleration/deceleration allowed, in microsteps per microseconds^2 (a float)
// This is a limiter, to minimize damage to the rail and motor
const float ACCEL_LIMIT = SPEED_LIMIT*SPEED_LIMIT/(2.0*BREAKING_DISTANCE);

// EEPROM addresses:
const short ADDR_POS = 0;  // Current position (float, 4 bytes)
const short ADDR_CALIBRATE = ADDR_POS+4;  // If =1, limiter calibration will be done at the beginning (1 byte)
const short ADDR_LIMIT1 = ADDR_CALIBRATE+1; // pos_short for the foreground limiter; should be 0? (2 bytes)
const short ADDR_LIMIT2 = ADDR_LIMIT1+2; // pos_short for the background limiter (2 bytes)

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

unsigned char abortMy=0; // immediately abort the loop if >0 (only if direction=0 - rail not moving)
unsigned char calibrate=0; // =3 when both limiters calibration is required; =1/2 when only the fore/background limiter (limit1/2) should be calibrated
unsigned char calibrate_init=0; // a copy of calibrate
unsigned char calibrate_flag=0; // a flag for each leg of calibration
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
float fps;  // Frames per second parameter
float mm_per_frame;  // Mm per shot parameter
float msteps_per_frame; // Microsteps per frame for focus stacking
short Nframes; // Number of frames for 2-point focus stacking
short frame_counter; // Counter for shots
short pos_to_shoot; // Position to shoot the next shot during focus stacking
short shutter_on; // flag for camera shutter: 0/1 corresponds to off/on
unsigned long t_shutter; // Time when the camera shutter was triggered

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
byte rowPins[rows] = {4, 10, 12, A1}; //connect to the row pinouts of the keypad
byte colPins[cols] = {A2, A3, A4, A5}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );

#endif


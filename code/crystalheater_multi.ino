//-------------------------------------------------------------------
// Crystalheater Controller
// (c) N. Trapp @ Small Molecule Crystallography Center
// ETH Zürich, 2018
//
// derived from:
// "Sous Vide Controller", (c) Bill Earl - for Adafruit Industries
// based on Arduino PID and PID AutoTune Libs, (c) Brett Beauregard
// Usage:
//        right         : start or next
//        left          : stop or previous,
//                        during ramping: stop and hold current temp
//        up/down       : during ramping&heating: show info screens
//        shift         : hold for larger increments in settings
//        shift & +     : start autotune (temp must be within 1°C
//                        of setpoint)
//        shift & -     : PID parameter settings
//        shift & right : start ramp
//
// Log format: Current T(°C), Setpoint(°C), Power(%), Runtime(min)
//------------------------------------------------------------------

// PID Library
#include <PID_v1.h>
#include <PID_AutoTune_v0.h>

// Libraries for the Adafruit RGB/LCD Shield
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>

// include the SmoothThermistor library
#include <SmoothThermistor.h>

// So we can save and retrieve settings
#include <EEPROM.h>

// ************************************************
// Pin definitions
// ************************************************

// Output for SSR DC/DC Relay
#define RelayPin 13

// Output Beeper (Sensor Errors and Ramp finished)
#define BuzzerPin 2

// ************************************************
// PID Variables and constants
// ************************************************

//Min&Max Temps/Ramps:
const int Tmax = 60;
const int Tmin = -40;
const double Rmax = 20.0;
const double Rmin = 0.05;  // also sets the ramp increment
const float Pi = 3.14159;

//Define Variables we'll be connecting to
double Setpoint;
double Ramprate;
double Endpoint;
double Timescale;
double Oruntime;
double Operiod;
double Oamp;
int Swdt;
double Shgh;
double Input;
double Output;
unsigned long Starttime;
double Starttemp;
double Rrmillis;
int Modei;
int Nsteps;
char *Modes[] = { "LINEAR", "STEPS", "OSCILLATE", "SAWTOOTH"};
char buffer[10]; // BUFFER FOR FORMATTING OUTPUT

volatile long onTime = 0;

// pid tuning parameters
double Kp;
double Ki;
double Kd;

// EEPROM addresses for persisted data
const int SpAddress = 0;
const int KpAddress = 8;
const int KiAddress = 16;
const int KdAddress = 24;
const int EpAddress = 32;
const int RrAddress = 40;
const int MdAddress = 48;
const int OrAddress = 56;
const int OpAddress = 64;
const int OaAddress = 72;
const int SwAddress = 80;
const int ShAddress = 88;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// 1 second Time Proportional Output window
int WindowSize = 1000; 
unsigned long windowStartTime;

// ************************************************
// Auto Tune Variables and constants
// ************************************************
byte ATuneModeRemember=2;

double aTuneStep=500;
double aTuneNoise=1;
unsigned int aTuneLookBack=20;

boolean tuning = false;

PID_ATune aTune(&Input, &Output);

// ************************************************
// Display Variables and constants
// ************************************************

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

#define BUTTON_SHIFT BUTTON_SELECT

unsigned long lastInput = 0; // last button press

byte degree[8] = // define the degree symbol 
{ 
 B00110, 
 B01001, 
 B01001, 
 B00110, 
 B00000,
 B00000, 
 B00000, 
 B00000 
}; 

unsigned long adjInterval; // ramp temp stepping
const long logInterval = 5000; // log every 5 seconds
unsigned long lastAdjTime = 0;
unsigned long lastLogTime = 0;

// ************************************************
// States for state machine
// ************************************************
enum operatingState { OFF = 0, SETM, SETP, SETOR, SETOA, SETOP, SETSW, SETSH, SETE, SETR, RUN, RMP, ERR, TUNE_P, TUNE_I, TUNE_D, AUTO};
operatingState opState = OFF;

// ************************************************
// Sensor Variables and constants
// Temp Data wire is plugged into port A0 on the Arduino

// create a SmoothThermistor instance, reading from analog pin 0
// using a common 100K NTC thermistor.
SmoothThermistor sensors(A0,ADC_SIZE_10_BIT,100000,100000);

// ************************************************
// Setup and display initial screen
// ************************************************
void setup()
{
   Serial.begin(9600);

   // Initialize Relay Control:

   pinMode(RelayPin, OUTPUT);    // Output mode to drive relay
   digitalWrite(RelayPin, LOW);  // make sure it is off to start

   // Thermistor setup:
   // use the AREF pin, so we can measure on 3.3v, which has less noise on an Arduino
   // make sure your thermistor is fed using 3.3v, along with the AREF pin
   // so the 3.3v output pin goes to the AREF pin and the thermistor
   // see "the advanced circuit" on top of this sketch
   sensors.useAREF(true);

   // Initialize LCD DiSplay 

   lcd.begin(16, 2);
   lcd.createChar(1, degree); // create degree symbol from the binary
   
   lcd.setBacklight(VIOLET);
   lcd.print(F("     SMoCC"));
   lcd.setCursor(0, 1);
   lcd.print(F(" CrystalHeater"));

//   if (!sensors.temperature()) 
//   {
//      lcd.setCursor(0, 1);
//      lcd.print(F("Sensor Error"));
//   }

   delay(3000);  // Splash screen

   // Initialize the PID and related variables
   LoadParameters();
   myPID.SetTunings(Kp,Ki,Kd);

   myPID.SetSampleTime(1000);
   myPID.SetOutputLimits(0, WindowSize);

  // Run timer2 interrupt every 15 ms 
  TCCR2A = 0;
  TCCR2B = 1<<CS22 | 1<<CS21 | 1<<CS20;

  //Timer2 Overflow Interrupt Enable
  TIMSK2 |= 1<<TOIE2;
}

// ************************************************
// Timer Interrupt Handler
// ************************************************
SIGNAL(TIMER2_OVF_vect) 
{
  if (opState == OFF)
  {
    digitalWrite(RelayPin, LOW);  // make sure relay is off
  }
  else
  {
    DriveOutput();
  }
}

// ************************************************
// Main Control Loop
//
// All state changes pass through here
// ************************************************
void loop()
{
   // wait for button release before changing state
   while(ReadButtons() != 0) {}

   lcd.clear();

   switch (opState)
   {
   case OFF:
      Off();
      break;
   case SETM:
      Tune_Md();
      break;
   case SETOR:
      Tune_Or();
      break;
   case SETOP:
      Tune_Op();
      break;
   case SETOA:
      Tune_Oa();
      break;     
   case SETP:
      Tune_Sp();
      break;
   case SETE:
      Tune_Ep();
      break;
   case SETR:
      Tune_Rr();
      break;
   case SETSW:
      Tune_Sw();
      break;
   case SETSH:
      Tune_Sh();
      break;
   case RUN:
      Run();
      break;
   case RMP:
      Ramp();
      break;
   case ERR:
      Error();
      break;
   case TUNE_P:
      TuneP();
      break;
   case TUNE_I:
      TuneI();
      break;
   case TUNE_D:
      TuneD();
      break;
   }
}

// ************************************************
// Initial State - press RIGHT to start heating
// ************************************************
void Off()
{
   myPID.SetMode(MANUAL);
   lcd.setBacklight(0);
   digitalWrite(RelayPin, LOW);  // make sure it is off
   lcd.print(F("     SMoCC"));
   lcd.setCursor(0, 1);
   lcd.print(F(" CrystalHeater"));
   uint8_t buttons = 0;
   
   while(!(buttons & (BUTTON_RIGHT)))
   {
      buttons = ReadButtons();
   }
   // Prepare to transition to the RUN state
   // turn the PID on:
   myPID.SetMode(AUTOMATIC);
   windowStartTime = millis();
   opState = RUN; // start control
}

// ************************************************
// Mode Entry State
// UP/DOWN to change ramp mode
// RIGHT to change Start Temp
// LEFT for Run mode
// SHIFT for 0.1x tuning
// ************************************************
void Tune_Md()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("Ramp Mode:"));
   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      if (buttons & BUTTON_LEFT)
      {
         opState = RUN;
         return;
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = SETP;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Modei += 1;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Modei -= 1;
         Setpoint = int(Setpoint);
         delay(200);
      }
      if (Modei > 3)
      {
         Modei = 0;
      }
      else if (Modei < 0)
      {
         Modei = 3;         
      }
      else
      {
         if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
         {
            opState = RUN;
            return;
         }
         
         lcd.setCursor(0,1);
         lcd.print(Modes[Modei]);
         lcd.print("      ");
         DoControl();
      }
   }
}

// ************************************************
// Setpoint / Start Temp Entry State
// UP/DOWN to change setpoint
// RIGHT for tuning parameters
// LEFT for set mode
// SHIFT for 0.1x tuning
// ************************************************
void Tune_Sp()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("Start Temp ["));
   lcd.write(1);
   lcd.print(F("C]:"));
   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      float increment = 1;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = SETM;
         return;
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = SETE;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Setpoint += increment;
         Setpoint = int(Setpoint);
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Setpoint -= increment;
         Setpoint = int(Setpoint);
         delay(200);
      }
      if (Setpoint > Tmax)
      {
         Setpoint = int(Tmin);
      }
      else if (Setpoint < Tmin)
      {
         Setpoint = int(Tmax);         
      }
      else
      {
         if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
         {
            opState = RUN;
            return;
         }
      if (Modei == 1) // STEPS
      {
         // The weird multiplication is necessary to prevent typecasting errors...
         Nsteps = int((10*(abs(Setpoint - Endpoint)))/int(10*Shgh));
         Timescale = double(Nsteps*Swdt)/60;   
      }
      else
      {
         Timescale = (abs(Setpoint - Endpoint))/Ramprate;
      }   
         lcd.setCursor(0,1);
         lcd.print(int(Setpoint));
         lcd.print("   ");
         lcd.setCursor(10,1);
         if (Modei != 2)   // show runtime where it makes sense
         {
           lcd.print(Timescale,1);    
           lcd.print(F("h  "));
         }
         DoControl();
      }
   }
}

// ************************************************
// Target Temp Entry State
// UP/DOWN to change setpoint
// RIGHT for tuning parameters
// LEFT for BACK
// SHIFT for 0.1x tuning
// ************************************************
void Tune_Ep()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("End Temp ["));
   lcd.write(1);
   lcd.print(F("C]:"));
   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      float increment = 1;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = SETP;
         return;
      }
      // FORK THE MENU HERE DEPENDING ON SELECTED RAMP MODE:
      if (buttons & BUTTON_RIGHT && ((Modei == 0) || (Modei == 3))) // LINEAR & SAWTOOTH
      {
         opState = SETR;
         return;
      }
      if (buttons & BUTTON_RIGHT && (Modei == 1)) // STEPS
      {
         opState = SETSW;
         return;
      }
      if (buttons & BUTTON_RIGHT && (Modei == 2)) // OSCILLATE
      {
         opState = SETOR;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Endpoint += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Endpoint -= increment;
         delay(200);
      }
      if (Endpoint > Tmax)
      {
         Endpoint = int(Tmin);
      }
      else if (Endpoint < Tmin)
      {
         Endpoint = int(Tmax);         
      }
      else
      {
         if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
         {
            opState = RUN;
            return;
         }

      if (Modei == 1) // STEPS
      {
         // The weird multiplication is necessary to prevent typecasting errors...        
         Nsteps = int((10*(abs(Setpoint - Endpoint)))/int(10*Shgh));
         Timescale = double(Nsteps*Swdt)/60;   
      }
      else
      {
         Timescale = (abs(Setpoint - Endpoint))/Ramprate;
      }   
         lcd.setCursor(0,1);
         lcd.print(int(Endpoint));
         lcd.print("   ");
         lcd.setCursor(10,1);
         if (Modei != 2)   // show runtime where it makes sense
         {
           lcd.print(Timescale,1);    
           lcd.print(F("h  "));
         }
         DoControl();
      }
   }
}

// ************************************************
// Ramp Rate Entry State
// UP/DOWN to change setpoint
// RIGHT for tuning parameters
// LEFT for BACK
// SHIFT for 0.1x tuning
// ************************************************
void Tune_Rr()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("Ramp ["));
   lcd.write(1);
   lcd.print(F("C/h]:"));
   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      float increment = Rmin;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = SETE;
         return;
      }
      if (buttons & BUTTON_RIGHT) 
      {
        if (Modei == 3) // SAWTOOTH
        {
          opState = SETOP; // set period
        }
        else
        {
          opState = RUN;  
        }
        return;
      }
      if (buttons & BUTTON_UP)
      {
         Ramprate += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Ramprate -= increment;
         delay(200);
      }
      if (Ramprate > Rmax)
      {
         Ramprate = Rmin;
      }
      else if (Ramprate < Rmin)
      {
         Ramprate = Rmax;         
      }
      else
      {    
         if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
         {
            opState = RUN;
            return;
         }
         Timescale = (abs(Setpoint - Endpoint))/Ramprate;
         
         lcd.setCursor(0,1);
         lcd.print(Ramprate,2);
         lcd.print("    ");
         lcd.setCursor(10,1);
         lcd.print(Timescale,1);    
         lcd.print(F("h  "));
         DoControl();
      }
   }
}

// ************************************************
// Step Width Entry State
// UP/DOWN to change setpoint
// RIGHT for Step Height Entry
// LEFT for BACK
// SHIFT for 10x tuning
// ************************************************
void Tune_Sw()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("Step Width[min]:"));

   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      int increment = 1;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = SETE;
         return;
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = SETSH;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Swdt += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Swdt -= increment;
         delay(200);
      }
      if (Swdt > 600)
      {
         Swdt = 1;
      }
      else if (Swdt < 1)
      {
         Swdt = 600;         
      }
      else
      {    
         if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
         {
            opState = RUN;
            return;
         }
         Nsteps = int((10*(abs(Setpoint - Endpoint)))/int(10*Shgh));
         Timescale = double(Nsteps*Swdt)/60;   
                 
         lcd.setCursor(0,1);
         lcd.print(Swdt);
         lcd.print("      ");
         lcd.setCursor(10,1);
         lcd.print(Timescale,1);    
         lcd.print(F("h  "));         
         DoControl();
      }
   }
}

// ************************************************
// Step Height Entry State
// UP/DOWN to change setpoint
// RIGHT for Run state
// LEFT for BACK
// SHIFT for 10x tuning
// ************************************************
void Tune_Sh()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("Step Height["));
   lcd.write(1);
   lcd.print(F("C]:"));
   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      float increment = 0.1;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = SETSW;
         return;
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = RUN;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Shgh += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Shgh -= increment;
         delay(200);
      }
      if (Shgh > abs(Setpoint - Endpoint))
      {
         Shgh = 0.1;
      }
      else if (Shgh < 0.1)
      {
         Shgh = int(abs(Setpoint - Endpoint));         
      }
      else
      {    
         if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
         {
            opState = RUN;
            return;
         }
         Nsteps = int((10*(abs(Setpoint - Endpoint)))/int(10*Shgh));
         Timescale = double(Nsteps*Swdt)/60;    
                 
         lcd.setCursor(0,1);
         lcd.print(Shgh,1);
         lcd.print("      ");
         lcd.setCursor(10,1);
         lcd.print(Timescale,1);    
         lcd.print(F("h  "));
         DoControl();
      }
   }
}

// ************************************************
// Oscillation Runtime Entry State
// UP/DOWN to change setpoint
// RIGHT for tuning parameters
// LEFT for BACK
// SHIFT for 10x tuning
// ************************************************
void Tune_Or()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("Runtime [h]:"));

   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      float increment = 0.5;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = SETE;
         return;
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = SETOP;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Oruntime += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Oruntime -= increment;
         delay(200);
      }
      if (Oruntime < 0.5)
      {
         Oruntime = 0.5;
      }
      else
      {    
         if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
         {
            opState = RUN;
            return;
         }
                 
         lcd.setCursor(0,1);
         lcd.print(Oruntime,1);
         lcd.print(" ");
         lcd.setCursor(10,1);
         DoControl();
      }
   }
}

// ************************************************
// Oscillation Period Entry State
// UP/DOWN to change setpoint
// RIGHT for tuning parameters
// LEFT for BACK
// SHIFT for 10x tuning
// ************************************************
void Tune_Op()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("Period [min]:"));

   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      float increment = 1;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
        if (Modei == 3)  // SAWTOOTH
        {
          opState = SETR; 
        }
        else
        {
          opState = SETOR;
        }
        return;
      }
      if (buttons & BUTTON_RIGHT)
      {
        if (Modei == 3)  // SAWTOOTH
        {
          opState = SETOA; 
        }
        else
        {
          opState = RUN;
        }
        return;
       }
      if (buttons & BUTTON_UP)
      {
         Operiod += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Operiod -= increment;
         delay(200);
      }
      if (Operiod < 1)
      {
         Operiod = 1;
      }
      else if (Operiod > Oruntime*60)
      {
         Operiod = Oruntime*60;
      }
      else
      {    
         if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
         {
            opState = RUN;
            return;
         }
                 
         lcd.setCursor(0,1);
         lcd.print(int(Operiod));
         lcd.print(" ");
         lcd.setCursor(10,1);
         DoControl();
      }
   }
}

// ************************************************
// Oscillation Amplitude Entry State (Sawtooth)
// UP/DOWN to change setpoint
// RIGHT for tuning parameters
// LEFT for BACK
// SHIFT for 10x tuning
// ************************************************
void Tune_Oa()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("Amplitude ["));
   lcd.write(1);
   lcd.print(F("C]:"));

   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      float increment = 0.1;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = SETOP;
         return;
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = RUN;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Oamp += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Oamp -= increment;
         delay(200);
      }
      if (Oamp > 0.5*abs(Setpoint-Endpoint) && 0.5*abs(Setpoint-Endpoint) > 1)
      {
         Oamp = 0.5*abs(Setpoint-Endpoint);
      }
      else if (Oamp < 0)
      {
         Oamp = 0.1;
      }
      else
      {    
         if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
         {
            opState = RUN;
            return;
         }
                 
         lcd.setCursor(0,1);
         lcd.print(Oamp,1);
         lcd.print(" ");
         lcd.setCursor(10,1);
         DoControl();
      }
   }
}

// ************************************************
// Proportional Tuning State
// UP/DOWN to change Kp
// RIGHT for Ki
// LEFT for setpoint
// SHIFT for 10x tuning
// ************************************************
void TuneP()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("Set Kp"));

   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      float increment = 1.0;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
       if (buttons & BUTTON_LEFT) 
      {
         opState = RUN;
         return; 
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = TUNE_I;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Kp += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Kp -= increment;
         delay(200);
      }
      if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
      {
         opState = RUN;
         return;
      }
      lcd.setCursor(0,1);
      lcd.print(Kp);
      lcd.print(" ");
      DoControl();
   }
}

// ************************************************
// Integral Tuning State
// UP/DOWN to change Ki
// RIGHT for Kd
// LEFT for Kp
// SHIFT for 10x tuning
// ************************************************
void TuneI()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("Set Ki"));

   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();

      float increment = 0.01;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = TUNE_P;
         return;
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = TUNE_D;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Ki += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Ki -= increment;
         delay(200);
      }
      if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
      {
         opState = RUN;
         return;
      }
      lcd.setCursor(0,1);
      lcd.print(Ki);
      lcd.print(" ");
      DoControl();
   }
}

// ************************************************
// Derivative Tuning State
// UP/DOWN to change Kd
// RIGHT for setpoint
// LEFT for Ki
// SHIFT for 10x tuning
// ************************************************
void TuneD()
{
   lcd.setBacklight(VIOLET);
   lcd.print(F("Set Kd"));

   uint8_t buttons = 0;
   while(true)
   {
      buttons = ReadButtons();
      float increment = 0.01;
      if (buttons & BUTTON_SHIFT)
      {
        increment *= 10;
      }
      if (buttons & BUTTON_LEFT)
      {
         opState = TUNE_I;
         return;
      }
      if (buttons & BUTTON_RIGHT)
      {
         opState = RUN;
         return;
      }
      if (buttons & BUTTON_UP)
      {
         Kd += increment;
         delay(200);
      }
      if (buttons & BUTTON_DOWN)
      {
         Kd -= increment;
         delay(200);
      }
      if ((millis() - lastInput) > 10000)  // return to RUN after 10 seconds idle
      {
         opState = RUN;
         return;
      }
      lcd.setCursor(0,1);
      lcd.print(Kd);
      lcd.print(" ");
      DoControl();
   }
}

// ************************************************
// PID Control State
// SHIFT and UP for autotune
// SHIFT and DOWN to set PID parameters
// SHIFT and RIGHT to start RAMP
// RIGHT - Set Mode
// LEFT - OFF
// UP - SHOW INFO
// DOWN - SHOW INFO
// ************************************************
void Run()
{
   SaveParameters();
   Starttemp = Setpoint;
   myPID.SetTunings(Kp,Ki,Kd);

   uint8_t buttons = 0;
   while(true)
   {
      // set up the LCD's number of rows and columns: 
      if (buttons & BUTTON_UP)
      {
        InfoUp();
      }
      else if (buttons & BUTTON_DOWN)
      {
        InfoDown();
      }
      else
      {
        lcd.setCursor(0,0);
        lcd.print(F(">"));
        lcd.print(int(Setpoint));
        lcd.print(F(" ... "));
        lcd.print(int(Endpoint));
        lcd.write(1);
        lcd.print(F("C"));
        lcd.print(F("    "));
      }
      
      setBacklight();  // set backlight based on state

      buttons = ReadButtons();
      // check for sensor errors or overheat:
      if ((Input >= Tmax+20)
         || (Input <= -100))
      {
         opState = ERR; 
         return;  
      }
      else if ((buttons & BUTTON_SHIFT) 
         && (buttons & BUTTON_UP) 
         && (abs(Input - Setpoint) < 1.0))  // Should be at steady-state
      {
         StartAutoTune();
      }
      else if ((buttons & BUTTON_SHIFT) 
         && (buttons & BUTTON_DOWN)) 
      {
        opState = TUNE_P; 
        return;  
      }
      else if ((buttons & BUTTON_SHIFT) 
         && (buttons & BUTTON_RIGHT))
      {
        opState = RMP; 
        return;  
      }
      else if (buttons & BUTTON_RIGHT)
      {
        opState = SETM;
        return;
      }
      else if (buttons & BUTTON_LEFT)
      {
        opState = OFF;
        return;
      }
    
      DoControl();
      float pct = map(Output, 0, WindowSize, 0, 1000);

      if (!(buttons & BUTTON_UP) && !(buttons & BUTTON_DOWN))
      {
        lcd.setCursor(0,1);
        lcd.print(Input);
        lcd.write(1);
        lcd.print(F("C : "));
        lcd.setCursor(10,1);
        lcd.print(F("      "));
        lcd.setCursor(10,1);
        lcd.print(pct/10,1);
        lcd.print("%");
      }

      lcd.setCursor(15,0);  //blink T while tuning, not mess up info screens!
      if (tuning && !(buttons & BUTTON_UP) && !(buttons & BUTTON_DOWN))
      {
        lcd.print("T");
      }
      else if (!(buttons & BUTTON_UP) && !(buttons & BUTTON_DOWN))
      {
        lcd.print(" ");
      }
      
      // periodically log to serial port in csv format
      if (millis() - lastLogTime > logInterval)  
      {
        lastLogTime = millis();
        Serial.print(Input,2);
        Serial.print(",");
        Serial.print(Setpoint,2);
        Serial.print(",");
        Serial.print(pct/10);
        Serial.print(",");
        Serial.println("0.00");
      }

      delay(100);
   }
}

// ************************************************
// PID RAMP State
// LEFT - HOLD
// UP - SHOW INFO
// DOWN - SHOW INFO
// ************************************************
void Ramp()
{
   myPID.SetTunings(Kp,Ki,Kd);
   Starttemp = Setpoint;
   // init local vars:
   uint8_t buttons = 0;
   int beepcount = 5;
   Starttime = millis();
   // convert Ramprate from °C/h to °C/msec:
   Rrmillis = Ramprate / 3600000;
   
   while(true)
   {
      // set up the LCD's number of rows and columns: 
      if (buttons & BUTTON_UP)
      {
        InfoUp();
      }
      else if (buttons & BUTTON_DOWN)
      {
        InfoDown();
      }
      else
      {
        lcd.setCursor(0,0);
        lcd.print(F(" "));
        lcd.print(int(Starttemp));
        lcd.print(F(" >>> "));
        lcd.print(int(Endpoint));
        lcd.write(1);
        lcd.print(F("C"));
        lcd.print(F("    "));
      }
      
      setBacklight();  // set backlight based on state
 
      buttons = ReadButtons();
      // check for sensor errors or overheat:
      if ((Input >= Tmax+20)
         || (Input <= -100))
      {
         opState = ERR; 
         return;  
      }
      else if (buttons & BUTTON_LEFT)
      {
        // set current temp as new setpoint here ("hold"):
        Setpoint = int(Input);
        opState = RUN;
        return;  
      }
      // beep & bail to RUN state as soon as target is reached (different conditions for each ramp program):
      else if (((abs(Input - Endpoint) < 0.2) && (Modei != 2) && (Modei != 3) && (Modei != 1)) 
          || ((abs(Input - Endpoint) < 0.2) && (double((millis()) - Starttime) > Oruntime*3600000) && (Modei == 2))
          || ((double((millis()) - Starttime) > Timescale*3600000) && (Modei == 3))
          || ((Setpoint == Endpoint) && (abs(Input - Endpoint) < 0.2) && (Modei == 1))
          )
      {
        while(beepcount > 0)
        {
        tone(BuzzerPin, 300, 2000);
        delay(3000);
        noTone(BuzzerPin);
        beepcount = beepcount - 1;
        }
        // set Endtemp as setpoint here ("hold"):
        Setpoint = Endpoint;
        opState = RUN;
        return;
      }

      // call the Ramp function here:
      switch (Modei)
      {
        case 0:
        Flinear();
        break;
        case 1:
        Fsteps();
        break;
        case 2:
        Foscillate();
        break;
        case 3:
        Fsawtooth();
        break;
      }
     
      DoControl();
      float pct = map(Output, 0, WindowSize, 0, 1000);      

      if (!(buttons & BUTTON_UP) && !(buttons & BUTTON_DOWN))
      {      
        lcd.setCursor(0,1);
        lcd.print(Input);
        lcd.write(1);
        lcd.print(F("C : "));
        lcd.setCursor(10,1);
        lcd.print(F("      "));
        lcd.setCursor(10,1);
        lcd.print(pct/10,1);
        lcd.print("%");
      }
      
      // periodically log to serial port in csv format
      if (millis() - lastLogTime > logInterval)  
      {
        lastLogTime = millis();
        Serial.print(Input,2);
        Serial.print(",");
        Serial.print(Setpoint,2);
        Serial.print(",");
        Serial.print(pct/10);
        Serial.print(",");
        Serial.println(((double(millis()) - Starttime)/60000),2);
        // FOR DIAGNOSTICS:
        //Serial.print(",");
        //Serial.println(Rcurrent, 4);
      }

      delay(100);
   }
}

// ************************************************
// PID ERROR State
// DEAD END UNTIL SENSOR BECOMES OPERATIONAL
// AND TEMP IS WITHIN LIMITS
// ************************************************
void Error()
{
   setBacklight();  // set backlight based on state
   while(true)
   {
      if (sensors.temperature() >= Tmax+20)
      {
         lcd.setCursor(5, 0);
         lcd.print(F("OVERHEAT"));
         tone(BuzzerPin, 800, 200);
      }
      else if (sensors.temperature() <= -100)
      {
      // Too cold, sensor not connected
         lcd.setCursor(1, 0);
         lcd.print(F("SENSOR DISCONN"));
         tone(BuzzerPin, 800, 200);
      }
      else
      {
        softReset();
        return;
      }
      delay(500);
      noTone(BuzzerPin);
      lcd.clear();
   }
}

// ************************************************
// WORK FUNCTIONS CALLED IN RAMP STATE
// ************************************************
void Flinear()
{
   adjInterval = 60000; // adjustment interval: 1min constant
   // periodically adjust the Setpoint towards the target (Endpoint)
   if (millis() - lastAdjTime > adjInterval)  
   {
     lastAdjTime = millis();
     // the actual adjustment procedure:
     if (Starttemp > Endpoint)      // ramp down
     {
       Setpoint = Starttemp - (double((millis() - Starttime)) * Rrmillis);
     }
     else                          // ramp up
     {
       Setpoint = Starttemp + (double((millis() - Starttime)) * Rrmillis);
     }
   }
}

void Fsteps()
{
   adjInterval = Swdt*60000; // adjustment interval = Step Width
   // periodically adjust the Setpoint towards the target (Endpoint)
   if (millis() - lastAdjTime > adjInterval)  
   {
     lastAdjTime = millis();
     // the actual adjustment procedure:
     if (Starttemp > Endpoint)      // ramp down
     {
       if (Setpoint - Shgh > Endpoint)
       {
         Setpoint = Setpoint - Shgh;
       }
       else
       {
         Setpoint = Endpoint;
       }
     }
     else                          // ramp up
     {
       if (Setpoint + Shgh < Endpoint)
       {
         Setpoint = Setpoint + Shgh;
       }
       else
       {
         Setpoint = Endpoint;
       }
     }
   }  
}

void Foscillate()
{
   adjInterval = Operiod*1000; // adjustment depending on period length (switch 60 times per period)
   if (millis() - lastAdjTime > adjInterval)  
   {
     lastAdjTime = millis();
     // the actual adjustment procedure:
     Setpoint = 0.5 * (Starttemp - Endpoint) * cos(double((millis() - Starttime)/(Operiod*60000))* 2 * Pi) + 0.5 * Starttemp + 0.5 * Endpoint;
   }
}

void Fsawtooth()
{
   adjInterval = Operiod*1000; // adjustment depending on period length (switch 60 times per period)
   if (millis() - lastAdjTime > adjInterval)  
   {
     lastAdjTime = millis();
      // the actual adjustment procedure:
     if (Starttemp > Endpoint)      // ramp down
     {
       Setpoint = Starttemp - Oamp - (double((millis() - Starttime)) * Rrmillis) + 0.5 * Oamp * cos(double((millis() - Starttime)/(Operiod*60000))* 2 * Pi);
     }
     else                          // ramp up
     {
       Setpoint = Starttemp - Oamp + (double((millis() - Starttime)) * Rrmillis) + 0.5 * Oamp * cos(double((millis() - Starttime)/(Operiod*60000))* 2 * Pi);
     }
   }
}
 
// ************************************************
// Soft Reset after missing Sensor is reattached
// ************************************************
void softReset()
{
   asm volatile ("  jmp 0");
}

// ************************************************
// Show Extra Info while running
// ************************************************
void InfoUp()
{
   switch(Modei) {
     case 0:  // LINEAR
       lcd.setCursor(0,0);
       lcd.print(F("LIN"));
       lcd.print(F("     "));
       dtostrf(Ramprate, 4, 2, buffer);
       lcd.setCursor(12-strlen(buffer),0);
       lcd.print(Ramprate,2);
       lcd.write(1);
       lcd.print(F("C/h"));
       lcd.setCursor(0,1);
       lcd.print(F("                "));
       break;
     case 1:  // STEPS
       double SRamprate;
       Nsteps = int((10*(abs(Starttemp - Endpoint)))/int(10*Shgh));
       SRamprate = abs(Starttemp - Endpoint)*60/double(Nsteps*Swdt);
       lcd.setCursor(0,0);
       lcd.print(F("STP("));
       lcd.print(Nsteps);
       lcd.print(F(")"));
       lcd.print(F("     "));
       dtostrf(SRamprate, 4, 1, buffer);
       lcd.setCursor(12-strlen(buffer),0);
       lcd.print(SRamprate,1);
       lcd.write(1);
       lcd.print(F("C/h"));
       lcd.setCursor(0,1);
       lcd.print(F("W:"));
       lcd.print(Swdt);
       lcd.print(F("m "));
       lcd.print(F("H:"));
       lcd.print(Shgh,1);
       lcd.write(1);
       lcd.print(F("C             "));        
       break;
     case 2:  // OSCILLATE
       lcd.setCursor(0,0);
       lcd.print(F("OSC"));
       lcd.print(F("         "));
       dtostrf(Oruntime, 4, 2, buffer);
       lcd.setCursor(16-strlen(buffer),0);
       lcd.print(Oruntime,1);
       lcd.print(F("h"));
       lcd.setCursor(0,1);
       lcd.print(F("P:"));
       lcd.print(Operiod,0);
       lcd.print(F("min            "));
       break;
     case 3:  // SAWTOOTH
       lcd.setCursor(0,0);
       lcd.print(F("SAW"));
       lcd.print(F("     "));
       dtostrf(Ramprate, 4, 2, buffer);
       lcd.setCursor(12-strlen(buffer),0);
       lcd.print(Ramprate,2);
       lcd.write(1);
       lcd.print(F("C/h"));
       lcd.setCursor(0,1);
       lcd.print(F("P:"));
       lcd.print(Operiod,0);
       lcd.print(F("min "));
       lcd.print(F("A:"));
       lcd.print(Oamp,1);
       lcd.write(1);
       lcd.print(F("C             "));       
       break;              
   }
}
void InfoDown()
{
  // calculate timescale if not yet set here:
  if (!Timescale)
  {
    if (Modei != 1)  // STEPS
    {
       Timescale = (abs(Setpoint - Endpoint))/Ramprate;
    }
    else
    {
       Nsteps = int((10*(abs(Starttemp - Endpoint)))/int(10*Shgh));
       Timescale = double(Nsteps*Swdt)/60;      
    }
  }
  if (opState == RMP)
  {
    lcd.setCursor(0,0);
    lcd.print(F("ELAPSED"));
    lcd.print(F("    "));
    lcd.setCursor(10,0);
    lcd.print(((double(millis())-Starttime)/3600000),1);
    lcd.print(F("  "));
    lcd.setCursor(15,0);
    lcd.print(F("h"));
    lcd.setCursor(0,1);
    lcd.print(F("REMAIN"));
    lcd.print(F("    "));
    lcd.setCursor(10,1);
    if (Modei != 2)
    {
      lcd.print(Timescale-((double(millis())-Starttime)/3600000),1);
      lcd.print(F("    "));
    }
    else
    {
      lcd.print(Oruntime-((double(millis())-Starttime)/3600000),1);
      lcd.print(F("    "));
    }
      lcd.setCursor(15,1);
      lcd.print(F("h"));
  }
  else  // before starting ramp program (=RUN state)
  {
    lcd.setCursor(0,0);
    lcd.print(F("ELAPSED"));
    lcd.print(F("    "));
    lcd.setCursor(10,0);
    lcd.print(F("---"));
    lcd.print(F("  "));
    lcd.setCursor(15,0);
    lcd.print(F("h"));
    lcd.setCursor(0,1);
    lcd.print(F("REMAIN"));
    lcd.print(F("    "));
    lcd.setCursor(10,1);
    if (Modei == 2) {
      lcd.print(Oruntime,1);
    }
    else
    {
      lcd.print(Timescale,1);
    }
    lcd.print(F("  "));
    lcd.setCursor(15,1);
    lcd.print(F("h"));
  }
}

// ************************************************
// Execute the control loop
// ************************************************
void DoControl()
{
  // Read the input:
  Input = sensors.temperature();
  
  if (tuning) // run the auto-tuner
  {
     if (aTune.Runtime()) // returns 'true' when done
     {
        FinishAutoTune();
     }
  }
  else // Execute control algorithm
  {
     myPID.Compute();
  }
  
  // Time Proportional relay state is updated regularly via timer interrupt.
  onTime = Output; 
}

// ************************************************
// Called by ISR every 15ms to drive the output
// ************************************************
void DriveOutput()
{  
  long now = millis();
  // Set the output
  // "on time" is proportional to the PID output
  if(now - windowStartTime>WindowSize)
  { //time to shift the Relay Window
     windowStartTime += WindowSize;
  }
  if((onTime > 100) && (onTime > (now - windowStartTime)))
  {
     digitalWrite(RelayPin,HIGH);
  }
  else
  {
     digitalWrite(RelayPin,LOW);
  }
}

// ************************************************
// Set Backlight based on the state of control
// ************************************************
void setBacklight()
{
   if (tuning)
   {
      lcd.setBacklight(VIOLET); // Tuning Mode
   }
   else if (opState == ERR)
   {
      lcd.setBacklight(VIOLET); // Error Mode
   }
   else if (abs(Input - Setpoint) > 5.0)  
   {
      lcd.setBacklight(RED);  // High Alarm - off by more than 1 degree
   }
   else if (abs(Input - Setpoint) > 1.0)  
   {
      lcd.setBacklight(YELLOW);  // Low Alarm - off by more than 0.2 degrees
   }
   else
   {
      lcd.setBacklight(GREEN);  // We're on target!
   }
}

// ************************************************
// Start the Auto-Tuning cycle
// ************************************************

void StartAutoTune()
{
   // REmember the mode we were in
   ATuneModeRemember = myPID.GetMode();

   // set up the auto-tune parameters
   aTune.SetNoiseBand(aTuneNoise);
   aTune.SetOutputStep(aTuneStep);
   aTune.SetLookbackSec((int)aTuneLookBack);
   tuning = true;
}

// ************************************************
// Return to normal control
// ************************************************
void FinishAutoTune()
{
   tuning = false;

   // Extract the auto-tune calculated parameters
   Kp = aTune.GetKp();
   Ki = aTune.GetKi();
   Kd = aTune.GetKd();

   // Re-tune the PID and revert to normal control mode
   myPID.SetTunings(Kp,Ki,Kd);
   myPID.SetMode(ATuneModeRemember);
   
   // Persist any changed parameters to EEPROM
   SaveParameters();
}

// ************************************************
// Check buttons and time-stamp the last press
// ************************************************
uint8_t ReadButtons()
{
  uint8_t buttons = lcd.readButtons();
  if (buttons != 0)
  {
    lastInput = millis();
  }
  return buttons;
}

// ************************************************
// Save any parameter changes to EEPROM
// ************************************************
void SaveParameters()
{
     if (Modei != EEPROM_readDouble(MdAddress))
   {
      EEPROM_writeDouble(MdAddress, Modei);
   }
   if (Setpoint != EEPROM_readDouble(SpAddress))
   {
      EEPROM_writeDouble(SpAddress, int(Setpoint));
   }
   if (Endpoint != EEPROM_readDouble(EpAddress))
   {
      EEPROM_writeDouble(EpAddress, Endpoint);
   }
   if (Ramprate != EEPROM_readDouble(RrAddress))
   {
      EEPROM_writeDouble(RrAddress, Ramprate);
   }
   if (Kp != EEPROM_readDouble(KpAddress))
   {
      EEPROM_writeDouble(KpAddress, Kp);
   }
   if (Ki != EEPROM_readDouble(KiAddress))
   {
      EEPROM_writeDouble(KiAddress, Ki);
   }
   if (Kd != EEPROM_readDouble(KdAddress))
   {
      EEPROM_writeDouble(KdAddress, Kd);
   }
      if (Oruntime != EEPROM_readDouble(OrAddress))
   {
      EEPROM_writeDouble(OrAddress, Oruntime);
   }
      if (Operiod != EEPROM_readDouble(OpAddress))
   {
      EEPROM_writeDouble(OpAddress, Operiod);
   }
   if (Oamp != EEPROM_readDouble(OaAddress))
   {
      EEPROM_writeDouble(OaAddress, Oamp);
   }
   if (Swdt != EEPROM_readDouble(SwAddress))
   {
      EEPROM_writeDouble(SwAddress, Swdt);
   }
   if (Shgh != EEPROM_readDouble(ShAddress))
   {
      EEPROM_writeDouble(ShAddress, Shgh);
   }
}

// ************************************************
// Load parameters from EEPROM
// ************************************************
void LoadParameters()
{
  // Load from EEPROM
   Modei = EEPROM_readDouble(MdAddress);
   Setpoint = int(EEPROM_readDouble(SpAddress));
   Endpoint = int(EEPROM_readDouble(EpAddress));
   Ramprate = EEPROM_readDouble(RrAddress);
   Oruntime = EEPROM_readDouble(OrAddress);
   Operiod = EEPROM_readDouble(OpAddress);
   Oamp = EEPROM_readDouble(OaAddress);
   Swdt = EEPROM_readDouble(SwAddress);
   Shgh = EEPROM_readDouble(ShAddress);
   Kp = EEPROM_readDouble(KpAddress);
   Ki = EEPROM_readDouble(KiAddress);
   Kd = EEPROM_readDouble(KdAddress);
   
   // Use defaults if EEPROM values are invalid
   if (isnan(Modei))
   {
     Modei = 0;
   }
   if (isnan(Setpoint))
   {
     Setpoint = 60;
   }
   if (isnan(Endpoint))
   {
     Endpoint = int(Setpoint - 20);
   }
   if (isnan(Ramprate))
   {
     Ramprate = 0.5;
   }
   if (isnan(Kp))
   {
     Kp = 850;
   }
   if (isnan(Ki))
   {
     Ki = 0.5;
   }
   if (isnan(Kd))
   {
     Kd = 0.1;
   }  
  if (isnan(Oruntime))
   {
     Oruntime = 2;
   }
   if (isnan(Operiod))
   {
     Operiod = 10;
   }
   if (isnan(Oamp))
   {
     Oamp = 1;
   } 
   if (isnan(Swdt))
   {
     Swdt = 1;
   }    
   if (isnan(Shgh))
   {
     Shgh = 1;
   }    
}


// ************************************************
// Write floating point values to EEPROM
// ************************************************
void EEPROM_writeDouble(int address, double value)
{
   byte* p = (byte*)(void*)&value;
   for (int i = 0; i < sizeof(value); i++)
   {
      EEPROM.write(address++, *p++);
   }
}

// ************************************************
// Read floating point values from EEPROM
// ************************************************
double EEPROM_readDouble(int address)
{
   double value = 0.0;
   byte* p = (byte*)(void*)&value;
   for (int i = 0; i < sizeof(value); i++)
   {
      *p++ = EEPROM.read(address++);
   }
   return value;
}

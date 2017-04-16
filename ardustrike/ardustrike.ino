#include <Adafruit_TiCoServo.h>
#include <known_16bit_timers.h>

#include <Adafruit_NeoPixel.h>

//This code utilizes TiCoServo to prevent interference between the Neopixels and the ESCs as documented here https://learn.adafruit.com/neopixels-and-servos/overview
//NOTE: for triggers, HIGH means the trigger is NOT pressed and LOW means it is pressed.
//TODO:  Write Select fire logic, Retraction logic



#define PIXEL_COUNT 17

#define PIXEL_PIN 16

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_TiCoServo pusherESC;
Adafruit_TiCoServo spinESC;


//declare servo objects that will be used throughout the rest of the project.
// constants won't change. They're used here to set pin numbers:

const int ledPin = 13;      // the number of the LED pin

//define pins for Digital switches
const int revSw = 12;
const int feedSw = 11;

//define pins for ESCs
const int pusherPin = 9;
const int flywheelPin = 10;

const int burstPin = 5;
const int fullPin = 6;

int spinAverage = -10;
int feedAverage = -10;

const int ESCMinSpeed = 1000;   //this varies by ESC.  Min speed must be low enough to arm.  Refer to speec controller's documentation for this
const int feedESCMinSpeed = 250;   //this varies by ESC.  Min speed must be low enough to arm.  Refer to speec controller's documentation for this
const int feedESCMaxSpeed = 2000;   //this varies by ESC.  Max speed here is defined seperately to allow for controlling fire rates.  Refer to speec controller's documentation for this
const int ESCMaxSpeed = 1860;   //this varies by ESC.  You may want to set it slightly lower than the max supported to avoid overheating issues

//variables to determine if we need to make changes to esc spinning states.  Eliminates unnecessary servo.writemicrosecond() calls
bool spinchange = false;
bool feedchange = false;

// Variables will change:
int ledState = LOW;         // the current state of the output pin
int spinButtonState;             // the current reading from the spin input pin
int feedButtonState;            // the current reading from the feed input pin
int burstReading;
int fullReading;
int spinReading;
int feedReading;

//track if the flywheels are currently spinning.  Used to prevent pusher from being run when flywheels are stationary
bool spinning = false;
//track if pusher is running
bool pushing = false;
//track if there has a request for the pusher to be retracted.
bool retract = false;

int rage = 0;
int rageColor=0;
//variables used for settging neopixel colors
int redVal, greenVal, blueVal;

void setup() {

  pinMode(ledPin, OUTPUT);
  pinMode(revSw, INPUT_PULLUP);
  pinMode(feedSw, INPUT_PULLUP);
  pinMode(burstPin, INPUT_PULLUP);
  pinMode(fullPin, INPUT_PULLUP);

  // set initial LED state
  digitalWrite(ledPin, ledState);

  pusherESC.attach(pusherPin, feedESCMinSpeed, feedESCMaxSpeed);
  spinESC.attach(flywheelPin,  ESCMinSpeed, ESCMaxSpeed);

  //write minimum spped to ESCs so they arm
  pusherESC.write(feedESCMinSpeed);
  spinESC.write(ESCMinSpeed);
  delay(1000);                  // waits for a second while the ESCs arm

  strip.begin();
  for (int i = 0; i < PIXEL_COUNT; i++) {
    //pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  // read the state of the switchs into local variables:
  spinReading = digitalRead(revSw);
  feedReading = digitalRead(feedSw);
  
  burstReading = digitalRead(burstPin);
  fullReading = digitalRead(fullPin);


//Firing Mode Handling
//If both both burstReading and fullReading are HIGH, then mode is Semi Auto




//Rev Trigger Handling
  if (spinReading == HIGH) {
    if (spinAverage < 9) {
      spinAverage++;
      spinAverage++; //we are incrementing spinAverage twice here since it is more important to stop the action quickly than start it quickly for safety sake
      spinchange = true; //set the spin change flag so that we can evaluate if the flywheels need to be stopped
      
    }
    if (rage > 0) {
      rage--;
    }
   
  } else {
    if (spinAverage > -10) {
      spinAverage--;
      spinchange = true; //set the spin change flag so that we can evaluate if the flywheels need to be started
    }
    
     if (rage < 25500) {
      rage++;
      rage++;
    }
  }


//Feed Trigger Handling

  if (feedReading == HIGH) {
    if (feedAverage < 9) {
      feedAverage++;
      feedAverage++; //we are incrementing feedAverage twice here since it is more important to stop the action quickly than start it quickly for safety sake
      feedchange=true; //set the feed change flag so that we can evaluate if the pusher needs to be started
    }
  } else {
    if (feedAverage > -10) {
      feedAverage--;
      feedchange=true; //set the feed change flag so that we can evaluate if the pusher needs to be started
    }
  }




  if(spinchange == true){
  if (spinAverage < 0) {
    spinESC.write(ESCMaxSpeed);
    spinning = true;
    spinchange = false; //reset spin change flag since the change has been processed
  } else {
    spinESC.write(ESCMinSpeed);
    spinning = false; //set spin state to false.
    spinchange = false; //reset spin change flag since the change has been processed

  }
  }

 

 if (spinning == true){ //check that flywheels are running before allowing pusher to engage. 
  if(feedchange == true){ //only update pusher speed if the trigger state has changed
    if (feedAverage < 0) {
      ledState = HIGH;
      pusherESC.write(feedESCMaxSpeed);
      feedchange=false; //reset feed change flag since the change has been processed
      pushing=true; //set flag that the pusher is running
    } else {
      //stop the pusher
      pusherESC.write(feedESCMinSpeed);
      ledState = LOW;
      feedchange = false; //reset feed change flag since the change has been processed
      pushing=false;
    }
  }
 }
 if (!spinning && pushing ){ //pusher is running and the flywheels are not.  THIS IS BAD! stop the pusher
  pusherESC.write(feedESCMinSpeed);
  ledState = LOW;
  pushing=false;
 }
 
//Retract code will go here.  Instead of there code for handling the feed trigger stopping the pusher, it will just set the retract variable which will then use this function to wait until the position switch to read low at which time it will stop the pusher motor



  //set rage lighting colors
   rageColor = rage/100;

   redVal = rageColor;
   greenVal = 0;
   blueVal = 255 - rageColor;


   for (int i = 0; i < PIXEL_COUNT; i++) {
      strip.setPixelColor(i, strip.Color( redVal, greenVal, blueVal )); 
    }
    
  // set the LED state:
  digitalWrite(ledPin, ledState);
  //write the neopixels
strip.show(); // This sends the updated pixel color to the hardware.

}


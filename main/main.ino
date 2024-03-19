/////////////////////////////////////////////////////////////////

#include "Rotary.h";
#include <TM1637Display.h>
#define SEG_DP 0x80
#include "Button2.h";
#include <arduino-timer.h>


/////////////////////////////////////////////////////////////////
// PINN ASSIGNMENTS

// KY-040
#define ROTARY_PIN1	3
#define ROTARY_PIN2	2
#define ROTARY_BTN 4

// TM1637
#define DISP_CLK A0
#define DISP_DIO A1

// Generic
#define FORCE_SWITCH  0
#define TRIGGER_BTN 1
#define OUT A2

/////////////////////////////////////
// CONSTANTS
#define CLICKS_PER_STEP 4
#define MIN_POS 0
#define MAX_POS 9990
#define START_POS 0
#define INCREMENT 50
#define INCREMENT_HOLD 5
#define MILLIS_PER_STEP 100
#define DP  (SEG_DP  >> 2) // decimal to 3rd position(from left) because MILLIS_PER_STEP is 100

#define DEBUG 0;

/////////////////////////
// Calculated build time variables

#define HOLD_FOR_FINE INCREMENT > INCREMENT_HOLD
#define COUNTDOWN_INCREMENT (HOLD_FOR_FINE ? INCREMENT_HOLD : INCREMENT) * MILLIS_PER_STEP

/////////////////////////////////////////////////////////////////

const uint8_t SEG_ON[] = {
  0,
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,  // O
  SEG_C | SEG_E | SEG_G,                          // n
};

/////////////////////////////////////////////////////////////////

TM1637Display display(DISP_CLK, DISP_DIO);
Rotary r;
Button2 rotaryButton;
Button2 triggerButton;
Button2 forceSwitch;
Timer<2> timer;

unsigned long endTime;
bool isActive;
unsigned int currentPosition;

/////////////////////////////////////////////////////////////////

void setup() {
  pinMode(TRIGGER_BTN, INPUT);
  pinMode(FORCE_SWITCH, INPUT);
  pinMode(OUT, OUTPUT);
  
  toggleSwitch(false);
  
  display.clear();
  display.setBrightness(0);

  rotaryButton.begin(ROTARY_BTN);
  rotaryButton.setPressedHandler(rotaryButtonPressed);
  rotaryButton.setReleasedHandler(rotaryButtonReleased);
  rotaryButton.setDoubleClickHandler(resetPosition);

  triggerButton.begin(TRIGGER_BTN);
  triggerButton.setTapHandler(trigger);

  forceSwitch.begin(FORCE_SWITCH);
  forceSwitch.setPressedHandler(forceSwitchOn);
  forceSwitch.setReleasedHandler(forceSwitchOff);

  // if (DEBUG){
  //   Serial.begin(9600);
  // }

  r.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP, MIN_POS, MAX_POS, START_POS, INCREMENT);
  r.setChangedHandler(rotate);

  display.showNumberDecEx(currentPosition, DP, false); 
}

void loop() {
  r.loop();
  rotaryButton.loop();
  triggerButton.loop();
  forceSwitch.loop();
  timer.tick();
}

/////////////////////////////////////////////////////////////////


// on change
void rotate(Rotary& r) {
    if(isActive){
      return;
    }
    currentPosition = r.getPosition();
    display.showNumberDecEx(currentPosition, DP, false);
}

void rotaryButtonPressed(Button2& b) {
    r.setIncrement(INCREMENT_HOLD);
    fixPositionToIncrement();
}

void rotaryButtonReleased(Button2& b) {
    fixPositionToIncrement();
    r.setIncrement(INCREMENT);

}

void fixPositionToIncrement(){
    r.resetPosition(currentPosition);
}

void resetPosition(){
  r.resetPosition(START_POS);
}

void trigger(Button2& b) {
    if(isActive){
      return;
    }

    unsigned long pos = currentPosition;
    unsigned long duration = pos * MILLIS_PER_STEP;

    if(duration == 0){
      return;
    }

    endTime = millis() + duration;

    toggleSwitch(true);
    
    timer.in(duration, timerFinish);
    timer.every(COUNTDOWN_INCREMENT, timerInterval);
}

void timerFinish(){
  toggleSwitch(false);
}


bool timerInterval(){
  unsigned long remainingTime = divRoundClosest(endTime - millis(), MILLIS_PER_STEP);   // we do not care the exact time, we want to see something consistent and pretty
  display.showNumberDecEx(remainingTime, DP, false);
  return true;
}


void forceSwitchOn(){
  toggleSwitch(true);
  display.setSegments(SEG_ON);
}

void forceSwitchOff(){
  toggleSwitch(false);
}

void toggleSwitch(bool turnon){
  timer.cancel();
  isActive = turnon;
  digitalWrite(OUT, !turnon);

  if(!turnon){
    display.showNumberDecEx(currentPosition, DP, false);
      r.setIncrement(INCREMENT);
  }else{
      r.setIncrement(0);
  }

}



unsigned long divRoundClosest(const unsigned long n, const unsigned long d)
{
  return ((n < 0) == (d < 0)) ? ((n + d/2)/d) : ((n - d/2)/d);
}

/////////////////////////////////////////////////////////////////
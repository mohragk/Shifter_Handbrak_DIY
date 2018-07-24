#include <Joystick.h>

// Last state of the buttons
const int pinToButtonOffset = 9; //offset from pin to button
int lastButtonState[2] = {0,0};

int lastOverrideButtonState = 0;
int overrideButtonNum = 6;

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_JOYSTICK,
  8, 0,                  // Button Count, Hat Switch Count
  true, false, false,    // X axis, but no Y and, Z
  false, false, false,   // No Rx, Ry, or Rz
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering


void setup() {
  // Initialize Pins
  pinMode(A0, INPUT);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  
  // Initialize Joystick Library
  Joystick.begin();
}


void loop() {

  //update handbrake axis
  int pot = analogRead(A0);
  pot = constrain(pot, 50, 750);
  int mapped = map(pot,50,750,0,255);
  Joystick.setXAxis(mapped);

  int currentOverrideButtonState = 0;

  //if more than half way along travel, set buttonState to 1.
  if (mapped >= 127) {
    currentOverrideButtonState = 1;
  } 

  // make sure we only change once
  if (lastOverrideButtonState != currentOverrideButtonState) {
  	Joystick.setButton(overrideButtonNum, currentOverrideButtonState);
  	lastOverrideButtonState = currentOverrideButtonState;
  }
  


  // Read pin values and update buttons
  for (int index = 0; index < 4; index++)
  {
    int currentButtonState = !digitalRead(index + pinToButtonMap);
    if (currentButtonState != lastButtonState[index])
    {
      Joystick.setButton(index, currentButtonState);
      lastButtonState[index] = currentButtonState;
    }
  }


}


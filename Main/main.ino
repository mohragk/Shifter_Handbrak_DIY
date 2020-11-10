//------------------------------
// - Connect microswitches from shifter to pins 9 and 10
// - Connect  10k potentiometer from handbrake to pin A0
//
// Serial command protocol for reading: <N,0000>
// Serial command protocol for sending: [N,0000]
//-----------------------------


#include "Joystick.h"
#include <EEPROM.h>
#include <Timer.h>


#define MAX_SHIFTER_BTNS 2
#define PIN_BUTTON_OFFSET 9

// only compile relevant code when not using handbrake (0)
#define USE_HANDBRAKE 1
#define USE_SERIAL 1
#define SINE_TEST 0
#define USE_EEPROM 1


// Last state of the buttons
int lastButtonState[MAX_SHIFTER_BTNS];

int lastHandbrakeButtonState = 0;
int handbrakeButtonNum = 6;

float skewFactor = 1.0f;
int deadZone = 0;


#if USE_HANDBRAKE
    Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_JOYSTICK,
        8, 0,                  // Button Count, Hat Switch Count
        true, false, false,    // X axis, but no Y and, Z
        false, false, false,   // No Rx, Ry, or Rz
        false, false,          // No rudder or throttle
        false, false, false);  // No accelerator, brake, or steering
#else
    Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_JOYSTICK,
        MAX_SHIFTER_BTNS, 0,                  // Button Count, Hat Switch Count
        false, false, false,   // no axis
        false, false, false,   // No Rx, Ry, or Rz
        false, false,          // No rudder or throttle
        false, false, false);  // No accelerator, brake, or steering
#endif

#if USE_SERIAL
    // serial variables
    const int numChars = 32;
    char receivedChars[numChars];
    char tempChars[numChars];

    char messageFromGUI[numChars] = {0};
    int valueFromGUI = 0;
    
    bool stringComplete = false;  // whether the string is complete
 

    
    void serialEventRun()
    {
        static bool inProgress = false;
        static int idx = 0;
        char beginMarker = '<';
        char endMarker = '>';
        char inChar;
        
        while (Serial.available()) 
        {
          inChar = Serial.read();
          
          if (inProgress == true)
          {
              if (inChar == endMarker)
              {
                  receivedChars[idx] = '\0';
                  inProgress = false;
                  idx = 0;
                  
                  stringComplete = true;
                  
              } else {
                  receivedChars[idx] = inChar;
                  idx++;
                  if (idx > numChars) {
                      idx = numChars - 1;
                  }
              }
          }
          else if ( inChar == beginMarker )
          {
              inProgress = true;
          }
          
      }
    }

    void parseData()
    {
        char* pchar;

         pchar = strtok(tempChars, ",");  // get first part
         strcpy(messageFromGUI, pchar);   // copy first part to messageFromGUI
  
         pchar = strtok(NULL, ",");
         valueFromGUI = atoi(pchar);
    }
    
    void updateValuesFromCommand()
    {
        if (stringComplete)
        {
            strcpy (tempChars, receivedChars);
            parseData();

            if      ( messageFromGUI[0] == 'S' )
            {
               skewFactor = static_cast<float>( valueFromGUI ) / 1024.0f;
               Serial.print("Skew command received: ");
               Serial.print(skewFactor);
               Serial.println();
            }
            else if ( messageFromGUI[0] == 'Z' )
            {
              deadZone = valueFromGUI;
              Serial.print("Deadzone command received: ");
              Serial.print(deadZone);
              Serial.println();
            }
            
           stringComplete = false;
             
        }
    }
#endif //USE_SERIAL


#if USE_EEPROM
    
    float eeSkew = 1.0;
    int   eeDeadzone = 0;

    Timer t;

    void getEEPROMData()
    {
       int eeAddress = 0;
       EEPROM.get(eeAddress, eeSkew);
       skewFactor = eeSkew;

       eeAddress = sizeof(float);
       EEPROM.get(eeAddress, eeDeadzone);
       deadZone = eeDeadzone;

    }

    void sendEEPROMData()
    {
       int eeAddress = 0;
       float sk = 1.0f;
       int dz = 0;
       
       EEPROM.get(eeAddress, sk);
       int skew = round(sk * 1024);
       Serial.print("[S,");       // prefix and name
       Serial.print(skew);        // value
       Serial.println("]");         // suffix

       eeAddress = sizeof(float);
       EEPROM.get(eeAddress, dz);
       Serial.print("[Z,");       // prefix and name
       Serial.print(dz);          // value
       Serial.println("]");         // suffix
    }

   

    void updateEEPROMData()
    {
        int eeAddress = 0;

        if (eeSkew != skewFactor)
        {
          EEPROM.put(eeAddress, skewFactor);
          eeSkew = skewFactor;
          Serial.println("skewFactor saved to EEPROM!");
        }

        if (eeDeadzone != deadZone)
        {
          eeAddress = sizeof(float);
          EEPROM.put(eeAddress, deadZone);
          eeDeadzone = deadZone;
          Serial.println("Deadzone saved to EEPROM!");
        }
    } 
#endif // USE_EEPROM


#if USE_HANDBRAKE
    int getSkewedValue(int value, float skew)
    {
        float norm = (float)value / 1024.0f;
        float skewed = pow( norm, 2.0f - skew ); // invert skew..
    
        return static_cast<int> (round( skewed * 1024.0f) );
    }
#endif //USE_HANDBRAKE



void setup() 
{
	// Initialize Pins
    pinMode(9, INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);
    
#if USE_HANDBRAKE
    pinMode(A0, INPUT);
#endif

#if USE_SERIAL
    Serial.begin(9600);
#endif

    memset( lastButtonState, 0, sizeof(lastButtonState) );

#if USE_EEPROM
    getEEPROMData();
    t.every(2000, sendEEPROMData); // send EEPROM data every 2 second over serial connection to GUI
#endif

    // Initialize Joystick Library
    Joystick.begin();
   
}


void loop() {

#if USE_SERIAL
   updateValuesFromCommand();
#endif

    

#if USE_HANDBRAKE
    //update handbrake axis
    int pot    = analogRead( A0 );

    #if SINE_TEST
      float mod =  ( sin( timer * PI ) + 1.0f ) / 2.0f;
      pot =static_cast<int>( mod * 1023.0f ); //TEST!
      timer += 0.001f;
      if (timer > 2.0f) {  timer = 0.0f; };
    #endif

    int skewed = getSkewedValue(pot, skewFactor);
    skewed     = constrain(skewed, deadZone, 1023);
    Joystick.setXAxis(skewed);
    
    //if more than half way along travel, set buttonState to 1.
    int currentHandbrakeButtonState = 0;
    if ( skewed > 512 ) currentHandbrakeButtonState = 1;

    if (lastHandbrakeButtonState != currentHandbrakeButtonState) 
    {
        Joystick.setButton(handbrakeButtonNum, currentHandbrakeButtonState);
        lastHandbrakeButtonState = currentHandbrakeButtonState;
    }
#endif //USE_HANDBRAKE 

    // Read pin values and update shifter buttons
    for (int i = 0; i < MAX_SHIFTER_BTNS; i++)
    {
        int currentButtonState = !digitalRead(i + PIN_BUTTON_OFFSET);

        if (currentButtonState != lastButtonState[i])
        {
            Joystick.setButton(i, currentButtonState);
            lastButtonState[i] = currentButtonState;
        }
    }

#if USE_EEPROM
    updateEEPROMData();
    t.update();
#endif
}


/*********************************************************************
Turns counter for 3D printed Rubber Motor Winder

Uses two hall effect devices to count turns on the output shaft in 
either direction and display total count on an OLED display. A reset 
button zeros the count. Future: hold button while turning the crank
to set turns limit. Display shows a progress bar and flashes a warning 
at 80% and 90% of set point. On/Off switch and sleep mode to preserve
LiPo batter. Rechargeable from a USB cable

*********************************************************************/

#include "U8glib.h"
#include <avr/eeprom.h>

#define ledPin        13   // onboard LED

#define encoder0PinB  A1    // Port 2 Interrupt 1
#define encoder0PinA  A2    // P2 interrupt 2
#define buttonPin     A3    // P2 interrupt 3

#define OLED_RESET    5

U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI 

#define SETMODETIME  2000
#define DEFAULTMAX   1000

volatile unsigned int encoderPos = 0;
volatile int buttonState;
volatile bool refreshDisplay = true;
volatile long buttonDownMillis;

volatile int prev_buttonPin;  
volatile int prev_encoder0PinA;

int _current_count = 0;

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(encoder0PinA, INPUT_PULLUP); 
  pinMode(encoder0PinB, INPUT_PULLUP); 
  prev_buttonPin = digitalRead(buttonPin);
  prev_encoder0PinA = digitalRead(encoder0PinA);
  
  InitializeInterrupt();
  
  Serial.begin(115200);
  Serial.println("TurnsCounter");
}


void loop() {
  int turnsCount = 0;
  int maxTurns = DEFAULTMAX; // TODO: save last value in  NVRAM
  int inc = 1;
  int prevButtonState = 0;
  int prevEncoderPos = 0;
  bool setMode = false;
  
  buttonState = digitalRead(buttonPin);
  prevButtonState = buttonState;
  buttonDownMillis = 0;

  Serial.print("Button State ");
  Serial.println(buttonState);
  
  // Print any new count values and toggle the output LED
  while (true) {

    // State Machine with two states (setMode=[true,false])
    if (buttonState == LOW) {
      if (millis() - buttonDownMillis > SETMODETIME) {
        if (setMode == false) {
          setMode = true;
          //maxTurns = DEFAULTMAX;
          refreshDisplay = true;
          Serial.println("goto set mode");
        }
      }
    }
    
    if (prevButtonState == HIGH && buttonState == LOW) {
      turnsCount = 0;
      refreshDisplay = true;
      Serial.println("State change Low to High");
      if (setMode == true) {
        setMode = false;
        Serial.println("leave set mode");
      }
    }
    prevButtonState = buttonState;

    // Something changeD, so update values and displap
    if (prevEncoderPos != encoderPos) {
      refreshDisplay = true;
      if (setMode) {
        maxTurns += 10 *(encoderPos - prevEncoderPos);
      }
      else {
        turnsCount += encoderPos - prevEncoderPos;
      }
      prevEncoderPos = encoderPos;
    }
    
    // LED indicates setMode
    digitalWrite(ledPin, setMode);
    
    // u8glib picture loop
    u8g.firstPage();  
    do {
      UpdateCountDisplay_u8g(turnsCount, maxTurns, setMode);
    } while( u8g.nextPage() );
    // rebuild the picture after some delay
    delay(50);
    // u8glib picture loop
    
  } //end while(true)
} // end loop()


void InitializeInterrupt(){
  cli();		// switch interrupts off while messing with their settings  
  PCICR =0x02;          // Enable PCINT1 interrupt
  PCMSK1 = 0b00001110;  // Mask lower all but bits 1, 2,  & 3
  sei();		// turn interrupts back on
}


ISR(PCINT1_vect) {
  
  //Emulate AttachInterrupt (..., CHANGE)
  int bp = digitalRead(buttonPin);
  if (bp != prev_buttonPin) {
    doButton(bp);
  }

  //Emulate AttachInterrupt (..., RISING)
  if (digitalRead(encoder0PinA)==0 && prev_encoder0PinA == 1) {
    doEncoder();
  }
  prev_buttonPin = digitalRead(buttonPin);
  prev_encoder0PinA = digitalRead(encoder0PinA);

}


void doButton(int pinState) {
  refreshDisplay = true;
  buttonState = pinState;
  if (buttonState == LOW) {
    buttonDownMillis = millis();
    Serial.println("Button Down");
  }
  else {
    Serial.println("Button Up");
  }
}


void doEncoder() {
  /* If pinA and pinB are both high or both low, it is spinning
   * forward. If they're different, it's going backward.
   */
  if (digitalRead(encoder0PinA) == digitalRead(encoder0PinB)) {
    encoderPos += 1;
  } else {
    encoderPos += -1;
  }
}


void UpdateCountDisplay_u8g(int turnsCount, int maxCount, bool setMode)
{
  // show the new value    
  if (setMode)  {
    u8g.setFont(u8g_font_helvR12);  
    u8g.setPrintPos(0, 12);
    u8g.print("Set MAX");
    u8g.setPrintPos(0, 30);
    u8g.print(maxCount);
  }
  else {
    u8g.setFont(u8g_font_helvR24);  
    u8g.setPrintPos(0, 30);
    u8g.print(turnsCount);
  }
}


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

#define LEDPIN        13   // onboard LED
#define ENCODER_PINB  A1    // Port 2 Interrupt 1
#define ENCODER_PINA  A2    // P2 interrupt 2
#define BUTTON_PIN    A3    // P2 interrupt 3
#define OLED_RESET    5

U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI 

#define INACTIVE_LIMIT  30000
#define SETMODETIME  3000
#define DEFAULTMAX   1600

volatile unsigned int gEncoderPos = 0;
volatile int gButtonState;
volatile long gInactiveMillis;
volatile long gButtonDownMillis;
volatile int gPrevButtonPin;  
volatile int gPrevEncoderPinA;


void setup()
{
  pinMode(LEDPIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER_PINA, INPUT_PULLUP); 
  pinMode(ENCODER_PINB, INPUT_PULLUP); 
  gPrevButtonPin = digitalRead(BUTTON_PIN);
  gPrevEncoderPinA = digitalRead(ENCODER_PINA);
  
  InitializeInterrupt();
  
  //Serial.begin(115200);
  //Serial.println("TurnsCounter");
}


void loop() {
  int turnsCount = 0;
  int maxTurns = DEFAULTMAX; // TODO: save last value in  NVRAM
  int inc = 1;
  int prevButtonState = 0;
  int prevEncoderPos = 0;
  bool setMode = false;

  gInactiveMillis = 0;
  
  gButtonState = digitalRead(BUTTON_PIN);
  prevButtonState = gButtonState;
  gButtonDownMillis = 0;

  //Serial.print("Button State ");
  //Serial.println(gButtonState);
  
  // Print any new count values and toggle the output LED
  while (true) {

    // State Machine with two states (setMode=[true,false])
    if (gButtonState == LOW) {
      if (millis() - gButtonDownMillis > SETMODETIME) {
        if (setMode == false) {
          setMode = true;
          //maxTurns = DEFAULTMAX;
          //gRefreshDisplay = true;
          //Serial.println("goto set mode");
        }
      }
    }
    
    if (prevButtonState == HIGH && gButtonState == LOW) {
      turnsCount = 0;
      //gRefreshDisplay = true;
      //Serial.println("State change Low to High");
      if (setMode == true) {
        setMode = false;
        //Serial.println("leave set mode");
      }
    }
    prevButtonState = gButtonState;

    // Something changeD, so update values and displap
    if (prevEncoderPos != gEncoderPos) {
      //gRefreshDisplay = true;
      if (setMode) {
        maxTurns += 10 *(gEncoderPos - prevEncoderPos);
      }
      else {
        turnsCount += gEncoderPos - prevEncoderPos;
      }
      prevEncoderPos = gEncoderPos;
    }
    
    // LED indicates setMode
    digitalWrite(LEDPIN, setMode);
    
    // u8glib picture loop
    u8g.firstPage();
    do {
      if (millis() - gInactiveMillis < INACTIVE_LIMIT) {
        UpdateCountDisplay_u8g(turnsCount, maxTurns, setMode);
      }
      else {
        // Turn off the display if nothing happends for a while
        u8g.setFont(u8g_font_helvR12);  
        u8g.setPrintPos(48, 24);
        u8g.print("Sleep");
      }
    } while (u8g.nextPage());
    delay(50);     // rebuild the picture after some delay
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
  
  gInactiveMillis = millis();
  
  //Emulate AttachInterrupt (..., CHANGE)
  int bp = digitalRead(BUTTON_PIN);
  if (bp != gPrevButtonPin) {
    doButton(bp);
  }

  //Emulate AttachInterrupt (..., RISING)
  if (digitalRead(ENCODER_PINA)==0 && gPrevEncoderPinA == 1) {
    doEncoder();
  }
  gPrevButtonPin = digitalRead(BUTTON_PIN);
  gPrevEncoderPinA = digitalRead(ENCODER_PINA);
}


void doButton(int pinState) {
  //gRefreshDisplay = true;
  gButtonState = pinState;
  if (gButtonState == LOW) {
    gButtonDownMillis = millis();
    //Serial.println("Button Down");
  }
//  else {
//    Serial.println("Button Up");
//  }
}


void doEncoder() {
  /* If pinA and pinB are both high or both low, it is spinning
   * forward. If they're different, it's going backward.
   */
  if (digitalRead(ENCODER_PINA) == digitalRead(ENCODER_PINB)) {
    gEncoderPos += 1;
  } else {
    gEncoderPos += -1;
  }
}


int flash_count = 0;
#define FLASH_CYCLE     19
#define FLASH_ON_COUNT  15
void UpdateCountDisplay_u8g(int turns, int max_turns, bool set_mode)
{
  flash_count += 1;
  if (flash_count > FLASH_CYCLE) {
    flash_count = 0;
  }
  
  // show the new value    
  if (set_mode)  {
    u8g.setFont(u8g_font_helvR12);  
    u8g.setPrintPos(0, 12);
    u8g.print("Set Max Turns");
    u8g.setPrintPos(88, 30);
    u8g.print(max_turns);
  }
  else {
    long pct_max = 100L * abs(turns) / max_turns;
    
    u8g.setFont(u8g_font_helvR24);  
    u8g.setPrintPos(0, 30);
    u8g.print(turns);

    u8g.setFont(u8g_font_helvR12);  
    u8g.setPrintPos(88, 30);
    u8g.print(max_turns);
    
    if (pct_max < 80 || flash_count < FLASH_ON_COUNT) {
      u8g.setPrintPos(88, 12);
      u8g.print(pct_max);
      u8g.print(" %");
    }
  }
}


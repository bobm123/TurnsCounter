/*********************************************************************
Turns counter for 3D printed Rubber Motor Winder

Uses two hall effect devices to count turns on the output shaft in 
either direction and display total count on an OLED display. A reset 
button zeros the count. Future: hold button while turning the crank
to set turns limit. Display shows a progress bar and flashes a warning 
at 80% and 90% of set point. On/Off switch and sleep mode to preserve
LiPo batter. Rechargeable from a USB cable

*********************************************************************/

//#include <SPI.h>
//#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include "U8glib.h"
#include <avr/eeprom.h>

#define ledPin        13   // onboard LED
/* Usiong standard Arduino pins
#define encoder0PinA  2    // interrupt 0
#define buttonPin     3    // interrupt 1
#define encoder0PinB  4
*/
#define encoder0PinB  A1    // Port 2 Interrupt 1
#define encoder0PinA  A2    // P2 interrupt 2
#define buttonPin     A3    // P2 interrupt 3

#define OLED_RESET    5

//Adafruit_SSD1306 display(OLED_RESET);
U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI 

#define SETMODETIME  2000
#define DEFAULTMAX   1000

volatile unsigned int encoderPos = 0;
volatile int buttonState;
volatile bool refreshDisplay = true;
volatile long buttonDownMillis;

// TODO: Set these by actually reading pins in init
volatile int prev_buttonPin;  
volatile int prev_encoder0PinA;


//bool ledToggle = false;
int _current_count = 0;

//#if (SSD1306_LCDHEIGHT != 32)
//#error("Height incorrect, please fix Adafruit_SSD1306.h!");
//#endif

void setup()   {
  
  pinMode(ledPin, OUTPUT);

  pinMode(buttonPin, INPUT_PULLUP);
  //pinMode(buttonPin, INPUT);
  //attachInterrupt(1, doButton, CHANGE);  // encoder pin on interrupt 0 - pin 4
  
  pinMode(encoder0PinA, INPUT_PULLUP); 
  //digitalWrite(encoder0PinA, HIGH);       // turn on pullup resistor
  pinMode(encoder0PinB, INPUT_PULLUP); 
  //digitalWrite(encoder0PinB, HIGH);       // turn on pullup resistor
  
  prev_buttonPin = digitalRead(buttonPin);
  prev_encoder0PinA = digitalRead(encoder0PinA);
  
  //attachInterrupt(0, doEncoder, RISING);  // encoder pin on interrupt 0 - pin 2

  InitializeInterrupt();
  
  Serial.begin(115200);
  Serial.println("TurnsCounter");

/*
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();

  // Clear the buffer.
  display.clearDisplay();

  // It could go as high as 5, but then it cuts off the bottom line)
  display.setTextColor(WHITE); 
  display.setTextSize(4);
*/

  u8g.setColorIndex(1);         // pixel on

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
    
    // Update Adafruit Display Drivers
    //if (refreshDisplay) {
    //  UpdateCountDisplay(turnsCount, maxTurns, setMode);
    //  refreshDisplay = false;
    //}
    // Update Adafruit Display Drivers

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



/*
void InitializeIO(){
  //pinMode(A0, INPUT_PULLUP);   // Pin A0 is input with internal pull-up resistor
  pinMode(A1, INPUT);	   // Pin A1 is input to which a switch is connected
  pinMode(A2, INPUT);	   // Pin A2 is input to which a switch is connected
  pinMode(A3, INPUT_PULLUP);   // Pin A2 is input with internal pull-up resistor
}
*/

void InitializeInterrupt(){
  cli();		// switch interrupts off while messing with their settings  
  PCICR =0x02;          // Enable PCINT1 interrupt
  PCMSK1 = 0b00001110;  // Mask lower all but bits 1, 2,  & 3
  sei();		// turn interrupts back on
}


ISR(PCINT1_vect) {
  // Interrupt service routine. Every single PCINT8..14 (=ADC0..5) change
  // will generate an interrupt: but this will always be the same interrupt routine
/*
  if (digitalRead(A1)==0)  Serial.println("A1");
  if (digitalRead(A2)==0)  Serial.println("A2");
  if (digitalRead(A3)==0)  Serial.println("A3 Low");
  if (digitalRead(A3)==1)  Serial.println("A3 High");
*/  
  //Emulate AttachInterrupt (..., CHANGE)
  int bp = digitalRead(buttonPin);
  if (bp != prev_buttonPin) {
    doButton(bp);
  }

  //Emulate AttachInterrupt (..., RISING)
  if (digitalRead(encoder0PinA)==0 && prev_encoder0PinA == 1) {
    doEncoder();
//    Serial.println("Encoder Update");
  }
  prev_buttonPin = digitalRead(buttonPin);
  prev_encoder0PinA = digitalRead(encoder0PinA);

/*  
  if (digitalRead(buttonPin)==0)  Serial.println("buttonPin 0");
  if (digitalRead(encoder0PinA)==0)  Serial.println("encoder0PinA 0");
  if (digitalRead(encoder0PinA)==1)  Serial.println("encoder0PinA 1");
  if (digitalRead(encoder0PinB)==0)  Serial.println("encoder0PinB 0");
  if (digitalRead(encoder0PinB)==1)  Serial.println("encoder0PinB 1");
*/
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




/*
void UpdateCountDisplay(int turnsCount, int maxCount, bool setMode)
{
  // erase the old value
  display.clearDisplay();

  // show the new value    
  if (setMode)  {
    display.setCursor(0,0);
    display.setTextSize(2);
    display.println("Set Max");
    display.println(maxCount);
  }
  else {
    display.setTextSize(4);
    display.setCursor(0,0);
    display.println(turnsCount);
  }
  display.display();
  
  Serial.print(turnsCount);
  Serial.print(", ");
  Serial.print(maxCount);
  Serial.print(", ");
  Serial.println(setMode);
}
*/


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
  //display.display();
  
  //Serial.print(turnsCount);
  //Serial.print(", ");
  //Serial.print(maxCount);
  //Serial.print(", ");
  //Serial.println(setMode);
}


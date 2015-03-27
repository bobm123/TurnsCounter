/*********************************************************************
Turns counter for 3D printed Rubber Motor Winder

Uses two hall effect devices to count turns on the output shaft in 
either direction and display total count on an OLED display. A reset 
button zeros the count. Future: hold button while turning the crank
to set turns limit. Display shows a progress bar and flashes a warning 
at 80% and 90% of set point. On/Off switch and sleep mode to preserve
LiPo batter. Rechargeable from a USB cable

*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/eeprom.h>

#define ledPin        13   // onboard LED
#define encoder0PinA  2    // interrrupt 0
#define buttonPin     3    // interrrupt 1
#define encoder0PinB  4
#define OLED_RESET    5

Adafruit_SSD1306 display(OLED_RESET);

#define SETMODETIME  2000

volatile unsigned int encoderPos = 0;
volatile int buttonState;
volatile bool refreshDisplay = true;
volatile long buttonUpMillis;
volatile long buttonDownMillis;

//bool ledToggle = false;
int _current_count = 0;

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup()   {
  
  pinMode(ledPin, OUTPUT);

  pinMode(buttonPin, INPUT);
  attachInterrupt(1, doButton, CHANGE);  // encoder pin on interrupt 0 - pin 4
  
  pinMode(encoder0PinA, INPUT); 
  digitalWrite(encoder0PinA, HIGH);       // turn on pullup resistor
  pinMode(encoder0PinB, INPUT); 
  digitalWrite(encoder0PinB, HIGH);       // turn on pullup resistor
  
  attachInterrupt(0, doEncoder, RISING);  // encoder pin on interrupt 0 - pin 2

  Serial.begin(115200);
  Serial.println("TurnsCounter");

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
}


void loop() {
  int turnsCount = 0;
  int maxTurns = 1600; // TODO: save last value in  NVRAM
  int inc = 1;
  int prevButtonState = 0;
  int prevEncoderPos = 0;
  bool setMode = false;
  
  buttonUpMillis = 0;
  buttonDownMillis = 0;
  
  // Print any new count values and toggle the output LED
  while (true) {

    if (prevEncoderPos != encoderPos) {
      refreshDisplay = true;
      prevEncoderPos = encoderPos;
    }
    
    if (buttonState == HIGH) {
      if (millis() - buttonDownMillis > SETMODETIME) {
        if (setMode == false) {
          setMode = true;
          refreshDisplay = true;
          Serial.println("goto set mode");
        }
      }
    }
    
    if (prevButtonState == LOW && buttonState == HIGH) {
      encoderPos = 0;
      refreshDisplay = true;
      Serial.println("State change low to high");
      if (setMode == true) {
        setMode = false;
        Serial.println("leave set mode");
      }
    }
//    if (prevButtonState == HIGH && buttonState == LOW) {
//      Serial.println("State change high to low");
//    }

    prevButtonState = buttonState;

    // Turn LED On while in set mode, 
    if (setMode) {
      maxTurns = encoderPos;
      digitalWrite(ledPin, HIGH);
    }
    else {
      turnsCount = encoderPos;
      digitalWrite(ledPin, LOW);
    }
    
    if (refreshDisplay) {
      UpdateCountDisplay(turnsCount, maxTurns, setMode);
      refreshDisplay = false;
    }
  }
}


void doButton() {
  buttonState = digitalRead(buttonPin);
  refreshDisplay = true;
  if (buttonState) {
    buttonDownMillis = millis();
  }
  else {
    buttonUpMillis = millis();
  }
}


void doEncoder() {
  /* If pinA and pinB are both high or both low, it is spinning
   * forward. If they're different, it's going backward.
   *
   * For more information on speeding up this process, see
   * [Reference/PortManipulation], specifically the PIND register.
   */
  if (digitalRead(encoder0PinA) == digitalRead(encoder0PinB)) {
    encoderPos += 1;
  } else {
    encoderPos += -1;
  }
}


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


void testscrolltext(void) {
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10,0);
  display.clearDisplay();
  display.println("scroll");
  display.display();
 
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);    
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
}


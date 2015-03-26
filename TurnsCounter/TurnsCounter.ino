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

volatile unsigned int encoder0Pos = 0;
volatile int buttonState;
volatile bool new_value = true;
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
  display.setTextSize(4);
}


void loop() {
  int turns = 0;
  int inc = 1;
  int prevButtonState = 0;
  bool setMode = false;
  
  buttonUpMillis = 0;
  buttonDownMillis = 0;
  
  // Print any new count values and toggle the output LED
  while (true) {

    if (buttonState == HIGH) {
      if (millis() - buttonDownMillis > 3000) {
        if (setMode == false) {
          setMode = true;
          Serial.println("goto set mode");
        }
      }
    }
    
    if (prevButtonState == LOW && buttonState == HIGH) {
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

    // Turn LED On while in set mode
    if (setMode) {
      digitalWrite(ledPin, HIGH);
    }
    else {
      digitalWrite(ledPin, LOW);
    }
    
    if (new_value) {
      
      if (buttonState == LOW) {
        UpdateCountDisplay(encoder0Pos);
      }
      else {
        encoder0Pos = 0;
      }
      new_value = false;
    }
  }
  
/*  
  while (1) {
    UpdateCountDisplay(turns);

    // simmulate encoder inputs
    delay(20);
    turns += inc;
    if (turns > 1600) {
      inc = -1;
    }
    if (turns <= 0) {
      inc = 1;
    }
  }  
*/
}


void doButton() {
  buttonState = digitalRead(buttonPin);
  new_value = true;
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
    encoder0Pos += 1;
    new_value = true;
  } else {
    encoder0Pos += -1;
    new_value = true;
  }
}


void UpdateCountDisplay(int count)
{
  // erase the old value
  display.setCursor(32,2);
  display.setTextColor(BLACK); 
  display.println(_current_count);

  // show the new value    
  display.setCursor(32,2);
  display.setTextColor(WHITE); 
  display.println(count);
  display.display();

  _current_count = count;
  Serial.println(_current_count);
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


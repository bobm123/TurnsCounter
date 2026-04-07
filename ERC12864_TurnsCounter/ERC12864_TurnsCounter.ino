#include <U8g2lib.h>
#include <SPI.h>

// QT Py ESP32-C3 display pin mappings
U8G2_ST7565_ERC12864_ALT_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 10, /* data=*/ 7, /* cs=*/ 6, /* dc=*/ 8, /* reset=*/ 5);
#define BACKLIGHT_LED 0

// Hall effect sensor pins (KY-003 modules)
// Left sensor triggers the interrupt; right sensor is read for direction
#define HALL_LEFT   1   // A2 = GPIO1 - interrupt pin
#define HALL_RIGHT  3   // A1 = GPIO3 - direction pin

// Button pin (BOOT button on QT Py ESP32-C3)
#define BUTTON_PIN  9   // GPIO9 - active LOW with internal pull-up

// Debounce time in milliseconds
#define DEBOUNCE_MS 10

// Inactivity timeout for backlight (milliseconds)
#define INACTIVE_TIMEOUT 60000

// Volatile state shared between ISR and main loop
volatile int gEncoderPos = 0;
volatile unsigned long gLastInterruptTime = 0;
volatile unsigned long gLastActivityTime = 0;

// ISR: called on FALLING edge of left hall sensor (magnet detected)
void IRAM_ATTR hallSensorISR() {
  unsigned long now = millis();
  if (now - gLastInterruptTime < DEBOUNCE_MS) {
    return;
  }
  gLastInterruptTime = now;

  gLastActivityTime = now;

  // Same state = winding (left closed first), different = unwinding
  if (digitalRead(HALL_LEFT) == digitalRead(HALL_RIGHT)) {
    gEncoderPos++;
  } else {
    gEncoderPos--;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Turns Counter - Hall Effect");

  // Backlight on
  pinMode(BACKLIGHT_LED, OUTPUT);
  digitalWrite(BACKLIGHT_LED, HIGH);

  // Hall effect sensor inputs
  pinMode(HALL_LEFT, INPUT_PULLUP);
  pinMode(HALL_RIGHT, INPUT_PULLUP);

  // Button input (active LOW, has internal pull-up on the board)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Interrupt on left sensor: FALLING = magnet detected (KY-003 goes LOW)
  attachInterrupt(HALL_LEFT, hallSensorISR, FALLING);

  // Initialize display
  u8g2.begin();
}

int prevCount = -99999;
char buffer[12];

void loop() {
  // Snapshot volatile value (atomic read on 32-bit platform)
  int currentCount = gEncoderPos;

  // Reset count on button press
  if (digitalRead(BUTTON_PIN) == LOW) {
    gEncoderPos = 0;
    currentCount = 0;
    gLastActivityTime = millis();
    Serial.println("Count reset");
    delay(200);  // Simple debounce — wait for release
  }

  // Serial debug on change
  if (currentCount != prevCount) {
    prevCount = currentCount;
    Serial.print("Turns: ");
    Serial.println(currentCount);
  }

  // Backlight: off after inactivity, on when activity resumes
  bool active = (millis() - gLastActivityTime) < INACTIVE_TIMEOUT;
  digitalWrite(BACKLIGHT_LED, active ? HIGH : LOW);

  // Update display
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvR12_tr);
    u8g2.drawStr(0, 15, "Turns:");
    itoa(currentCount, buffer, 10);
    u8g2.setFont(u8g2_font_ncenB24_tr);
    int swidth = u8g2.getStrWidth(buffer);
    u8g2.drawStr(128 - swidth, 27, buffer);
  } while (u8g2.nextPage());

  delay(50);
}

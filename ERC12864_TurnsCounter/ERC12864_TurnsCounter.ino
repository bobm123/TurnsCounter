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

// Long press threshold to enter set-mode (milliseconds)
#define SETMODE_TIME 3000

// Default max turns
#define DEFAULT_MAX 1600

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

int turnsCount = 0;
int prevTurns = 0;       // saved count from last reset
int maxTurns = DEFAULT_MAX;
int prevEncoderPos = 0;
int prevDisplayedCount = -99999;
bool setMode = false;
bool buttonWasUp = true;
unsigned long buttonDownTime = 0;
bool longPressHandled = false;
char buffer[12];

void loop() {
  unsigned long now = millis();

  // --- Encoder delta ---
  int encoderPos = gEncoderPos;
  int delta = encoderPos - prevEncoderPos;
  if (delta != 0) {
    prevEncoderPos = encoderPos;
    if (setMode) {
      maxTurns += 10 * delta;
      if (maxTurns < 0) maxTurns = 0;
    } else {
      turnsCount += delta;
    }
  }

  // --- Button state machine ---
  bool buttonDown = (digitalRead(BUTTON_PIN) == LOW);

  if (buttonDown && buttonWasUp) {
    // Button just pressed
    buttonDownTime = now;
    longPressHandled = false;
    gLastActivityTime = now;
  }

  if (buttonDown && !buttonWasUp) {
    // Button held — check for long press
    if (!longPressHandled && (now - buttonDownTime > SETMODE_TIME)) {
      longPressHandled = true;
      if (!setMode) {
        setMode = true;
        Serial.println("Set mode ON");
      }
    }
  }

  if (!buttonDown && !buttonWasUp) {
    // Button just released
    if (!longPressHandled) {
      // Short press
      if (setMode) {
        setMode = false;
        Serial.print("Set mode OFF, max=");
        Serial.println(maxTurns);
      } else {
        prevTurns = turnsCount;
        turnsCount = 0;
        gEncoderPos = 0;
        prevEncoderPos = 0;
        Serial.print("Reset. Prev=");
        Serial.println(prevTurns);
      }
    }
    gLastActivityTime = now;
  }

  buttonWasUp = !buttonDown;

  // --- Serial debug on count change ---
  if (turnsCount != prevDisplayedCount) {
    prevDisplayedCount = turnsCount;
    Serial.print("Turns: ");
    Serial.println(turnsCount);
  }

  // --- Backlight: off after inactivity ---
  bool active = (now - gLastActivityTime) < INACTIVE_TIMEOUT;
  digitalWrite(BACKLIGHT_LED, active ? HIGH : LOW);

  // --- Update display ---
  // Flash effect for set-mode: hide max value briefly
  bool flashOn = setMode && ((now / 300) % 2 == 0);

  u8g2.firstPage();
  do {
    // Row 1: Current count (large)
    u8g2.setFont(u8g2_font_helvB12_tr);
    u8g2.drawStr(0, 15, "Turns:");
    itoa(turnsCount, buffer, 10);
    u8g2.setFont(u8g2_font_ncenB24_tr);
    int swidth = u8g2.getStrWidth(buffer);
    u8g2.drawStr(128 - swidth, 27, buffer);

    // Row 2: Previous count
    u8g2.setFont(u8g2_font_helvB10_tr);
    u8g2.drawStr(0, 46, "Prev:");
    itoa(prevTurns, buffer, 10);
    swidth = u8g2.getStrWidth(buffer);
    u8g2.drawStr(128 - swidth, 46, buffer);

    // Row 3: Max turns (flashes in set-mode)
    u8g2.drawStr(0, 61, "Max:");
    if (!flashOn) {
      itoa(maxTurns, buffer, 10);
      swidth = u8g2.getStrWidth(buffer);
      u8g2.drawStr(128 - swidth, 61, buffer);
    }
  } while (u8g2.nextPage());

  delay(50);
}

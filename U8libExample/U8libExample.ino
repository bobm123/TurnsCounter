#include "U8glib.h"

U8GLIB_SSD1306_128X64 u8g(12, 11, 8, 9);	// SW SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9

void setup(void) {
  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }
  
  u8g.setFont(u8g_font_fub30);
  //u8g.setFont(u8g_font_profont11);
  //u8g.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
  u8g.drawStr( 0, 32, "u8g library");
}

void loop(void) {
  static unsigned long thisMicros = 0;
  static unsigned long lastMicros = 0;
  
  // picture loop
  u8g.firstPage();  
  do {
    u8g.setPrintPos( 0, 36);
    u8g.print(thisMicros - lastMicros);
//    u8g.drawStr( 0, 60, "u8g library");
  } while( u8g.nextPage() );
  
  lastMicros = thisMicros;
  thisMicros = micros();
}
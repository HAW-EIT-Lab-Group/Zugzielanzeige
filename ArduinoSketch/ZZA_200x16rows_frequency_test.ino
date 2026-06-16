#include <digitalWriteFast.h>

#define PIN_G 22  // Data Green => neu!, statt Stecker Pin 1, Pin 2
#define PIN_R 24  // Data Red => neu!, statt am Stecker Pin 2, Pin 1
#define PIN_A0 30
#define PIN_A1 32
#define PIN_A2 34
#define PIN_CS 36  // Chip Select low->0-7; high->8-15
//Gemeinsam für alle Flachbandkabel?
#define PIN_CLK 26  // Clock
#define PIN_STR 28  // Latch
//Dauerhaft auf GND -> Hardwareseitig?
#define PIN_EN1_R 38  //Brightness -> PWM
#define PIN_EN2_G 39  //Brightness -> PWM

#define WIDTH 200
#define HEIGHT 16

void setup() {
  pinModeFast(PIN_G, OUTPUT);
  pinModeFast(PIN_R, OUTPUT);
  pinModeFast(PIN_CLK, OUTPUT);
  pinModeFast(PIN_STR, OUTPUT);
  pinModeFast(PIN_A0, OUTPUT);
  pinModeFast(PIN_A1, OUTPUT);
  pinModeFast(PIN_A2, OUTPUT);
  pinModeFast(PIN_CS, OUTPUT);
  pinModeFast(PIN_EN1_R, OUTPUT);
  pinModeFast(PIN_EN2_G, OUTPUT);

  digitalWriteFast(PIN_EN1_R, false);
  digitalWriteFast(PIN_EN2_G, false);
}


void loop() {
  for (int y = 0; y < HEIGHT; y++) {
    setRow(y);
    for (int x = 0; x < WIDTH; x++) {


      //Eine ganze Reihe in grün
      digitalWriteFast(PIN_R, true);
      digitalWriteFast(PIN_G, true);


/*
      //Nur ein Pixel in grün
      if (x == 50 && y == 8) {
        digitalWriteFast(PIN_R, true);
        digitalWriteFast(PIN_G, false);
      } else {
        digitalWriteFast(PIN_R, false);
        digitalWriteFast(PIN_G, false);
      }
*/

      digitalWriteFast(PIN_CLK, true);
      digitalWriteFast(PIN_CLK, false);
    }
    digitalWriteFast(PIN_STR, true);
    digitalWriteFast(PIN_STR, false);
  }
}


void setRow(int row) {
  digitalWriteFast(PIN_A0, row & 0x01);
  digitalWriteFast(PIN_A1, row & 0x02);
  digitalWriteFast(PIN_A2, row & 0x04);
  digitalWriteFast(PIN_CS, row >= 8);
    }

#include <digitalWriteFast.h>
#include <string.h>

// Pin 01   D_G         red
// Pin 03   D_R         green
// Pin 05   CLK         orange
// Pin 07   STR         yellow
// Pin 09   A0          white
// Pin 11   A1          grey
// Pin 13   A2          black
// Pin 15   CS          brown
// Pin 17   EN_R        blue
// Pin 18   EN_G        purple
// Pin 19   not used
// sonst.   GND

// Section 1
#define PIN_1_R 22
#define PIN_1_G 24

// Section 2
#define PIN_2_R 26
#define PIN_2_G 28

// Section 3
#define PIN_3_R 30
#define PIN_3_G 32

// Section 4
#define PIN_4_R 34
#define PIN_4_G 36

// Global for all Sections
#define PIN_CLK 38
#define PIN_STR 40
#define PIN_EN_R 42
#define PIN_EN_G 44

#define PIN_A0 46
#define PIN_A1 48
#define PIN_A2 50
#define PIN_CS 52

// Panel Measurements
#define WIDTH 200
#define HEIGHT 16
#define HEIGHT_ALL 64

// Matrices
uint8_t red_matrix[HEIGHT_ALL][WIDTH / 8];  // 64 rows * 25 bytes => 64 * (25 * 8 bits bzw. 8 cols) => 64 rows * 200 cols
uint8_t green_matrix[HEIGHT_ALL][WIDTH / 8];

// Inital setup
void setup() {
  // GPIO
  pinModeFast(PIN_1_R, OUTPUT);
  pinModeFast(PIN_1_G, OUTPUT);

  pinModeFast(PIN_2_R, OUTPUT);
  pinModeFast(PIN_2_G, OUTPUT);

  pinModeFast(PIN_3_R, OUTPUT);
  pinModeFast(PIN_3_G, OUTPUT);

  pinModeFast(PIN_4_R, OUTPUT);
  pinModeFast(PIN_4_G, OUTPUT);

  pinModeFast(PIN_CLK, OUTPUT);
  pinModeFast(PIN_STR, OUTPUT);
  pinModeFast(PIN_EN_R, OUTPUT);
  pinModeFast(PIN_EN_G, OUTPUT);

  pinModeFast(PIN_A0, OUTPUT);
  pinModeFast(PIN_A1, OUTPUT);
  pinModeFast(PIN_A2, OUTPUT);
  pinModeFast(PIN_CS, OUTPUT);

  // Inital Values
  analogWrite(PIN_EN_R, 0);  // active LOW  -> 100% Brightness = 0
  analogWrite(PIN_EN_G, 0);  //             -> 0% Brightness = 255

  memset(red_matrix, 0xFF, sizeof(red_matrix));  // Fill complete matrix with the same default value
  memset(green_matrix, 0xFF, sizeof(green_matrix));
}

// Main Loop
void loop() {
  
  // Set whole Diplay like in the matrices defined
  for (uint8_t y = 0; y < HEIGHT; y++) {
    //Row selection
    digitalWriteFast(PIN_A0, (y & 0x01) != 0);
    digitalWriteFast(PIN_A1, (y & 0x02) != 0);
    digitalWriteFast(PIN_A2, (y & 0x04) != 0);
    digitalWriteFast(PIN_CS, y >= 8);

    // for every byte of the row
    for (uint8_t xb = 0; xb < WIDTH / 8; xb++) {
      //select the byte for each of the 4 section
      uint8_t r1 = red_matrix[y][xb];
      uint8_t g1 = green_matrix[y][xb];

      uint8_t r2 = red_matrix[y + 16][xb];
      uint8_t g2 = green_matrix[y + 16][xb];

      uint8_t r3 = red_matrix[y + 32][xb];
      uint8_t g3 = green_matrix[y + 32][xb];

      uint8_t r4 = red_matrix[y + 48][xb];
      uint8_t g4 = green_matrix[y + 48][xb];

      //for every bit of the byte
      for (uint8_t bit = 0; bit < 8; bit++)  // if mirrored: for (int8_t bit = 7; bit >= 0; bit--)
      {
        uint8_t mask = 1 << bit;  //set a bitmask for a single bit, bit = 3: 0b00001000

        // set output bit:
        digitalWriteFast(PIN_1_R, (r1 & mask) != 0);
        digitalWriteFast(PIN_1_G, (g1 & mask) != 0);

        digitalWriteFast(PIN_2_R, (r2 & mask) != 0);
        digitalWriteFast(PIN_2_G, (g2 & mask) != 0);

        digitalWriteFast(PIN_3_R, (r3 & mask) != 0);
        digitalWriteFast(PIN_3_G, (g3 & mask) != 0);

        digitalWriteFast(PIN_4_R, (r4 & mask) != 0);
        digitalWriteFast(PIN_4_G, (g4 & mask) != 0);

        // Clocking
        digitalWriteFast(PIN_CLK, true);
        digitalWriteFast(PIN_CLK, false);
      }
    }

    // Latch the whole display
    digitalWriteFast(PIN_STR, true);
    digitalWriteFast(PIN_STR, false);
  }
}

/*
  Example for bit = 3:
  r1                 => 10110010
  mask               => 00001000
  r1 & mask          => 00000000 => 0
  (r1 & mask) != 0   => false


  Example for bit = 1:
  r1                 => 10110010
  mask               => 00010000
  r1 & mask          => 00000010 => 16
  (r1 & mask) != 0   => true
*/
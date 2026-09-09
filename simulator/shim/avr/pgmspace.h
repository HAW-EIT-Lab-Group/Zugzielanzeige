#pragma once
// PROGMEM-Ersatz fuer den PC: auf dem PC liegt alles im normalen RAM,
// die Makros werden also einfach zu ganz normalen Zugriffen aufgeloest.
// Damit laesst sich Game.cpp/MazeData.h/imageData.h unveraendert uebersetzen.
#include <stdint.h>
#include <string.h>

#define PROGMEM
#define PGM_P const char *
#define PSTR(s) (s)

#define pgm_read_byte(addr)      (*(const uint8_t *)(addr))
#define pgm_read_byte_near(addr) (*(const uint8_t *)(addr))
#define pgm_read_word(addr)      (*(const uint16_t *)(addr))
#define pgm_read_dword(addr)     (*(const uint32_t *)(addr))
#define pgm_read_ptr(addr)       (*(void * const *)(addr))

#define memcpy_P  memcpy
#define strcpy_P  strcpy
#define strlen_P  strlen
#define strcmp_P  strcmp

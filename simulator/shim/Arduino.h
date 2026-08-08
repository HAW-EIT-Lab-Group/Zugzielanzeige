#pragma once
// ---------------------------------------------------------------------------
// Minimaler Arduino-Ersatz fuer den PC-Simulator.
//
// Nur so viel, wie Game.cpp und Graphics.cpp tatsaechlich brauchen:
//   millis(), Serial (begin/available/read/print/println), F(), memset/memcpy.
// Die eigentliche Implementierung steht in src/sim_arduino.cpp; die Uhrzeit
// kommt aus der virtuellen Uhr des Simulators (pausierbar, verlangsambar).
// ---------------------------------------------------------------------------
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

typedef uint8_t byte;
typedef bool    boolean;

// --- Zeit ------------------------------------------------------------------
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

// --- GPIO-Attrappen (werden vom Spiel selbst nicht benutzt, nur von Display.cpp,
//     das im Simulator NICHT mitkompiliert wird) ------------------------------
#define HIGH   1
#define LOW    0
#define INPUT  0
#define OUTPUT 1
#define INPUT_PULLUP 2
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int  digitalRead(uint8_t pin);

// Analogeingaenge des Mega2560 (A0 = Pin 54)
#define A0 54
#define A1 55
#define A2 56
#define A3 57
#define A4 58
#define A5 59
#define A6 60
#define A7 61
#define A8 62
#define A9 63
#define A10 64
#define A11 65
#define A12 66
#define A13 67
#define A14 68
#define A15 69

// Liefert wie der echte ADC 0..1023. Ein offener Eingang rauscht auf der
// Hardware - im Simulator ist das Rauschen zwar auch bei jedem Aufruf anders,
// aber ueber Programmlaeufe hinweg IMMER dieselbe Folge. Damit bleibt ein
// "randomSeed(analogRead(A0))" reproduzierbar; mit "--seed N" laesst sich
// bewusst ein anderes Spiel erzeugen.
int  analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int value);
void analogReference(uint8_t mode);

// --- Zufall ----------------------------------------------------------------
// Gleiche Bedeutung wie auf dem Arduino: random(n) liefert 0..n-1,
// random(a,b) liefert a..b-1.
//
// Wichtig fuers Debuggen: der Simulator startet immer mit demselben Startwert,
// ein Fehler laesst sich also mit "--ms <zeit> --keys <tasten>" beliebig oft
// exakt gleich nachstellen. randomSeed() aendert das wie gewohnt.
long random(long howbig);
long random(long howsmall, long howbig);
void randomSeed(unsigned long seed);

// --- Rechen-Helfer aus der Arduino-Bibliothek -------------------------------
// (min/max/abs sind bewusst nicht definiert: als Makros zerlegen sie auf dem
//  PC die Standardbibliothek-Header)
long map(long x, long inMin, long inMax, long outMin, long outMax);

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define sq(x) ((x) * (x))

#define bit(b)              (1UL << (b))
#define bitRead(value, b)   (((value) >> (b)) & 0x01)
#define bitSet(value, b)    ((value) |= (1UL << (b)))
#define bitClear(value, b)  ((value) &= ~(1UL << (b)))
#define bitWrite(value, b, bitvalue) ((bitvalue) ? bitSet(value, b) : bitClear(value, b))

#define lowByte(w)  ((uint8_t)((w) & 0xFF))
#define highByte(w) ((uint8_t)((w) >> 8))

// --- Serial ----------------------------------------------------------------
// Eingaben kommen aus dem Simulator-Fenster (Tastendruck -> Serial.read()),
// Ausgaben landen in der Konsole und im Log-Fenster des Simulators.
class SimSerial
{
public:
    void begin(unsigned long baud);
    void end();
    int  available();
    int  read();
    int  peek();
    void flush();

    size_t print(const char *s);
    size_t print(char c);
    size_t print(int v);
    size_t print(unsigned int v);
    size_t print(long v);
    size_t print(unsigned long v);
    size_t print(double v);

    size_t println();
    size_t println(const char *s);
    size_t println(char c);
    size_t println(int v);
    size_t println(unsigned int v);
    size_t println(long v);
    size_t println(unsigned long v);
    size_t println(double v);

    size_t write(uint8_t c);
};

extern SimSerial Serial;

// Auf dem AVR legt F() den String ins Flash - auf dem PC unnoetig.
#define F(string_literal) (string_literal)

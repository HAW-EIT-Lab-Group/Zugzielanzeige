// ---------------------------------------------------------------------------
// Arduino-Ersatz: virtuelle Uhr + Serial-Umleitung auf Konsole/Fenster.
// ---------------------------------------------------------------------------
#include <Arduino.h>
#include <stdio.h>
#include <deque>
#include <string>

#include "sim_runtime.h"

namespace
{
    unsigned long          g_virtMs = 0;      // virtuelle Zeit in ms
    std::deque<char>       g_rxPuffer;        // Tastendruecke -> Serial.read()
    std::string            g_txZeile;         // noch nicht abgeschlossene Ausgabe
    std::deque<std::string> g_txLog;          // fertige Ausgabezeilen
    const size_t           LOG_MAX = 200;

    void logZeileAbschliessen()
    {
        printf("[Serial] %s\n", g_txZeile.c_str());
        fflush(stdout);
        g_txLog.push_back(g_txZeile);
        if (g_txLog.size() > LOG_MAX) g_txLog.pop_front();
        g_txZeile.clear();
    }
}

// --- SimRuntime -------------------------------------------------------------
void SimRuntime::uhrSetzen(unsigned long ms) { g_virtMs = ms; }
unsigned long SimRuntime::uhrLesen()         { return g_virtMs; }
void SimRuntime::tasteSenden(char c)         { g_rxPuffer.push_back(c); }
int  SimRuntime::logAnzahl()                 { return (int)g_txLog.size(); }

const char *SimRuntime::logZeile(int i)
{
    if (i < 0 || i >= (int)g_txLog.size()) return "";
    return g_txLog[(size_t)i].c_str();
}

// --- Zeit -------------------------------------------------------------------
unsigned long millis() { return g_virtMs; }
unsigned long micros() { return g_virtMs * 1000UL; }

// Das Spiel benutzt bewusst kein delay(); falls doch, wird nur die virtuelle
// Uhr weitergestellt - der Simulator bleibt fluessig.
void delay(unsigned long ms)              { g_virtMs += ms; }
void delayMicroseconds(unsigned int us)   { g_virtMs += us / 1000; }

// --- Zufall -----------------------------------------------------------------
// Fester Startwert: derselbe Simulatorlauf liefert immer dasselbe Spiel, damit
// sich ein Fehler reproduzieren laesst. randomSeed() setzt ihn wie auf dem AVR.
namespace
{
    uint32_t g_zufallsZustand = 1;

    uint32_t zufallsSchritt()
    {
        // xorshift32 - schnell und ohne Abhaengigkeit von der C-Bibliothek,
        // damit jeder Rechner dieselbe Folge erzeugt
        uint32_t x = g_zufallsZustand;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        g_zufallsZustand = x;
        return x;
    }
}

void randomSeed(unsigned long seed)
{
    if (seed != 0) g_zufallsZustand = (uint32_t)seed;
}

long random(long howbig)
{
    if (howbig <= 0) return 0;
    return (long)(zufallsSchritt() % (uint32_t)howbig);
}

long random(long howsmall, long howbig)
{
    if (howsmall >= howbig) return howsmall;
    return howsmall + random(howbig - howsmall);
}

long map(long x, long inMin, long inMax, long outMin, long outMax)
{
    if (inMax == inMin) return outMin;
    return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

// --- GPIO-Attrappen ---------------------------------------------------------
void pinMode(uint8_t, uint8_t)      {}
void digitalWrite(uint8_t, uint8_t) {}
int  digitalRead(uint8_t)           { return 0; }

// ADC-Attrappe: eigener Rauschgenerator, unabhaengig von random(), damit ein
// "randomSeed(analogRead(A0))" im Spiel nicht die Folge von random() stoert.
namespace
{
    uint32_t g_rauschZustand = 0x2BAD1DEA;
}

void SimRuntime::analogRauschenSetzen(uint32_t start)
{
    g_rauschZustand = start ? start : 0x2BAD1DEA;
}

int analogRead(uint8_t)
{
    uint32_t x = g_rauschZustand;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_rauschZustand = x;
    return (int)(x & 0x3FF); // 10 Bit wie der AVR-ADC
}

void analogWrite(uint8_t, int)   {}
void analogReference(uint8_t)    {}

// --- Serial -----------------------------------------------------------------
SimSerial Serial;

void SimSerial::begin(unsigned long baud) { printf("[Serial] begin(%lu)\n", baud); }
void SimSerial::end()                     {}
void SimSerial::flush()                   {}

int SimSerial::available() { return (int)g_rxPuffer.size(); }

int SimSerial::read()
{
    if (g_rxPuffer.empty()) return -1;
    char c = g_rxPuffer.front();
    g_rxPuffer.pop_front();
    return (unsigned char)c;
}

int SimSerial::peek()
{
    if (g_rxPuffer.empty()) return -1;
    return (unsigned char)g_rxPuffer.front();
}

size_t SimSerial::write(uint8_t c)
{
    if (c == '\n')      logZeileAbschliessen();
    else if (c != '\r') g_txZeile.push_back((char)c);
    return 1;
}

size_t SimSerial::print(const char *s)
{
    size_t n = 0;
    for (; s && *s; s++, n++) write((uint8_t)*s);
    return n;
}

size_t SimSerial::print(char c)          { return write((uint8_t)c); }
size_t SimSerial::print(int v)           { char b[32]; snprintf(b, sizeof b, "%d", v);   return print(b); }
size_t SimSerial::print(unsigned int v)  { char b[32]; snprintf(b, sizeof b, "%u", v);   return print(b); }
size_t SimSerial::print(long v)          { char b[32]; snprintf(b, sizeof b, "%ld", v);  return print(b); }
size_t SimSerial::print(unsigned long v) { char b[32]; snprintf(b, sizeof b, "%lu", v);  return print(b); }
size_t SimSerial::print(double v)        { char b[64]; snprintf(b, sizeof b, "%.2f", v); return print(b); }

size_t SimSerial::println()                  { return write((uint8_t)'\n'); }
size_t SimSerial::println(const char *s)     { return print(s) + println(); }
size_t SimSerial::println(char c)            { return print(c) + println(); }
size_t SimSerial::println(int v)             { return print(v) + println(); }
size_t SimSerial::println(unsigned int v)    { return print(v) + println(); }
size_t SimSerial::println(long v)            { return print(v) + println(); }
size_t SimSerial::println(unsigned long v)   { return print(v) + println(); }
size_t SimSerial::println(double v)          { return print(v) + println(); }

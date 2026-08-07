#pragma once
#include <stdint.h>

// Schnittstelle zwischen der Simulator-Oberflaeche (sim_main.cpp) und dem
// Arduino-Ersatz (sim_arduino.cpp).
namespace SimRuntime
{
    // virtuelle Uhr: millis() im Spiel liefert genau diesen Wert.
    // Die Oberflaeche stellt sie weiter - pausiert, in Zeitlupe oder im Zeitraffer.
    void          uhrSetzen(unsigned long ms);
    unsigned long uhrLesen();

    // Tastendruck in den Serial-Empfangspuffer legen (Serial.available()/read())
    void tasteSenden(char c);

    // Serial-Ausgaben des Spiels (Serial.println(...)), 0 = aelteste
    int         logAnzahl();
    const char *logZeile(int i);
}

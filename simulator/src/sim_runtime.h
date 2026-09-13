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

    // Startwert des ADC-Rauschens (analogRead). Steuert ueber das
    // randomSeed(analogRead(...)) des Spiels, welches Spiel gewuerfelt wird.
    void analogRauschenSetzen(uint32_t start);

    // Serial-Ausgaben des Spiels (Serial.println(...)), 0 = aelteste
    int         logAnzahl();
    const char *logZeile(int i);

    // --- emulierter SNES-Controller -------------------------------------
    // Der Simulator baut das Schieberegister eines echten SNES-Pads nach
    // (siehe sim_arduino.cpp). Die Tastatur des Simulators laeuft damit durch
    // genau den snespad.poll()-Pfad, den auch die Hardware nimmt - Game.cpp
    // bleibt unveraendert, auch bei EINGABEMODUS CONTROLLER.
    // bits sind die SNES_*-Konstanten aus SNESpad.h.
    void     padSetzen(uint16_t bits, bool gedrueckt);
    void     padAlleLoesen();
    uint16_t padZustand();

    // Controller an- oder abstecken (Standard: angesteckt)
    void padAnstecken(bool angesteckt);
}

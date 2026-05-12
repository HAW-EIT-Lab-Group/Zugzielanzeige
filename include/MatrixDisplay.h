/**
 * MatrixDisplay.h
 *
 * Treiber für die 64×200-Pixel-Bi-Color-LED-Matrix (Zugzielanzeige).
 *
 * Architektur
 * ───────────
 * Die vier Flachbandkabel-Segmente werden parallel betrieben: Ein gemeinsamer
 * CLK- und LATCH-Impuls steuert alle Schieberegister gleichzeitig. Nur die
 * Datenleitungen (Green + Red) sind pro Segment getrennt. Damit werden in einem
 * Scan-Durchlauf (Zeile 0–15) alle 64 physikalischen Zeilen des Displays
 * gleichzeitig aktualisiert (Segment 0 = Zeile 0+rowAddr, Segment 1 = 16+rowAddr usw.).
 *
 * Interrupt-Betrieb (optional)
 * ────────────────────────────
 * enableInterruptDriven(true) aktiviert einen Timer1-CTC-ISR, der automatisch
 * alle ~1,25 ms die nächste Scan-Zeile ausgibt → 50 Hz flimmerfreie Anzeige
 * ohne Blockierung der Hauptschleife. Ideal für Animationen und Spiele.
 *
 * Speicherbedarf
 * ──────────────
 * 2 × 64 × 25 = 3200 Bytes RAM für den Framebuffer (grün + rot).
 * Arduino Mega 1280: 8 kB RAM → ~39 % Auslastung durch den Treiber allein.
 */

#pragma once
#include <Arduino.h>
#include "matrix_config.h"

// ─── Farben ───────────────────────────────────────────────────────────────────

enum class Color : uint8_t {
    OFF    = 0b00, // aus
    GREEN  = 0b01, // grün
    RED    = 0b10, // rot
    ORANGE = 0b11, // grün + rot gleichzeitig = orange/gelb
};

// ─── Klasse ───────────────────────────────────────────────────────────────────

class MatrixDisplay {
public:
    MatrixDisplay() = default;

    /**
     * Hardware initialisieren: DDR-Register, Ports und Framebuffer auf 0 setzen.
     * Muss vor allen anderen Methoden aufgerufen werden.
     */
    void begin();

    // ── Pixelzugriff ──────────────────────────────────────────────────────────

    /** Einen Pixel setzen (x = Spalte 0…199, y = Zeile 0…63). */
    void setPixel(uint16_t x, uint8_t y, Color color);

    /** Einen Pixel lesen. Gibt Color::OFF zurück, wenn außerhalb des Displays. */
    Color getPixel(uint16_t x, uint8_t y) const;

    // ── Grafik-Primitiven ─────────────────────────────────────────────────────

    /** Gesamten Framebuffer auf eine Farbe setzen (Standard: aus). */
    void clear(Color color = Color::OFF);

    /** Horizontale Linie von x1 bis x2 in Zeile y. */
    void drawHLine(uint16_t x1, uint16_t x2, uint8_t y, Color color);

    /** Vertikale Linie von y1 bis y2 in Spalte x. */
    void drawVLine(uint8_t y1, uint8_t y2, uint16_t x, Color color);

    /** Rechteck (nur Rahmen). */
    void drawRect(uint16_t x, uint8_t y, uint16_t w, uint8_t h, Color color);

    /** Ausgefülltes Rechteck. */
    void fillRect(uint16_t x, uint8_t y, uint16_t w, uint8_t h, Color color);

    /**
     * Bitmap aus PROGMEM ausgeben.
     * @param bitmap  Zeiger auf Bilddaten: 1 Bit pro Pixel, MSB zuerst,
     *                Breite auf Bytes aufgerundet (stride = (w+7)/8).
     * @param x,y     Zielposition (obere linke Ecke).
     * @param w,h     Breite und Höhe in Pixeln.
     * @param color   Vordergrundfarbe (0-Bits = transparent).
     */
    void drawBitmap(const uint8_t* bitmap, uint16_t x, uint8_t y,
                    uint16_t w, uint8_t h, Color color);

    // ── Anzeige-Refresh ───────────────────────────────────────────────────────

    /**
     * Eine Scan-Zeile ausgeben und zur nächsten weiterschalten (0…15).
     * Aufruf in schneller Schleife oder ISR für persistente Anzeige.
     */
    void scanRow();

    /**
     * Alle 16 Scan-Zeilen einmal komplett ausgeben (blockierend, ca. 3 ms).
     * Für einfache Anwendungen ohne Timer-ISR.
     * @param rowDwellUs  Anzeigezeit pro Zeile in Mikrosekunden.
     */
    void refresh(uint16_t rowDwellUs = 200);

    // ── Timer-ISR-Betrieb ─────────────────────────────────────────────────────

    /**
     * Timer1-CTC-Interrupt aktivieren/deaktivieren.
     * Bei true: ISR ruft automatisch scanRow() alle ~1,25 ms auf (50 Hz Gesamt).
     * Achtung: scanRow() nicht zusätzlich manuell aufrufen, wenn ISR aktiv ist.
     */
    void enableInterruptDriven(bool enable);

    // ── Helligkeit ────────────────────────────────────────────────────────────

    /**
     * Helligkeit einstellen: 0 = aus, 1 = niedrig, 2 = mittel, 3 = voll.
     * Nutzt EN1 und EN2 zur Steuerung. Das genaue Verhalten hängt von der
     * Hardware ab und muss ggf. angepasst werden.
     */
    void setBrightness(uint8_t level);

private:
    // Framebuffer: je ein Byte-Array für Grün und Rot.
    // [Zeile 0…63][Byte 0…24] – Bit 7 = Spalte 0 des Bytes (MSB-first).
    uint8_t _greenBuf[MATRIX_ROWS][MATRIX_BYTES_ROW];
    uint8_t _redBuf  [MATRIX_ROWS][MATRIX_BYTES_ROW];

    uint8_t _brightness = 3; // Helligkeitsstufe 0–3
    uint8_t _scanRow    = 0; // Aktuelle Scan-Adresse 0–15

    // Alle 200 Bits einer Scan-Zeile einschieben, Adresse setzen und latchen.
    void _shiftAndLatchRow(uint8_t rowAddr);

    // Zeilenadresse (0–15) in PORTL schreiben (A0/A1/A2 + CS).
    void _setRowAddress(uint8_t rowAddr);

    // Display-Ausgabe über EN1/EN2 aktivieren oder deaktivieren.
    void _setEnable(bool on);
};

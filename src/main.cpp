/**
 * main.cpp
 *
 * Demo-Anwendung für die 64×200-Bi-Color-LED-Matrix (Zugzielanzeige).
 *
 * Zeigt eine Abfolge von Testmustern, um die Hardware-Verdrahtung zu
 * überprüfen und die Leistungsfähigkeit des Treibers zu demonstrieren:
 *
 *   1. Farbbalken          – prüft alle drei Farben flächig
 *   2. Schachbrettmuster   – prüft alternierend jeden einzelnen Pixel
 *   3. Rahmentest          – prüft Rechteck-Ausgabe an den Display-Grenzen
 *   4. Bouncing Ball       – ISR-betriebene Animation (zeigt Interrupt-Refresh)
 *
 * Die Muster wechseln automatisch alle DEMO_INTERVAL_MS Millisekunden.
 * Während des Bouncing-Ball-Demos läuft der Display-Refresh im Hintergrund
 * via Timer1-ISR; die Hauptschleife kümmert sich nur um die Spiellogik.
 */

#include <Arduino.h>
#include "MatrixDisplay.h"

static constexpr uint32_t DEMO_INTERVAL_MS = 4000; // Anzeigezeit je Muster

MatrixDisplay display;

// ─── Testmuster ───────────────────────────────────────────────────────────────

/** Drei horizontale Farbbalken (Grün / Orange / Rot). */
static void patternColorBars() {
    constexpr uint8_t thirdH = MATRIX_ROWS / 3;

    display.fillRect(0, 0,          MATRIX_COLS, thirdH,              Color::GREEN);
    display.fillRect(0, thirdH,     MATRIX_COLS, thirdH,              Color::ORANGE);
    display.fillRect(0, 2 * thirdH, MATRIX_COLS, MATRIX_ROWS - 2 * thirdH, Color::RED);
}

/** Schachbrettmuster, abwechselnd Grün und Rot. */
static void patternCheckerboard() {
    for (uint8_t y = 0; y < MATRIX_ROWS; ++y) {
        for (uint16_t x = 0; x < MATRIX_COLS; ++x) {
            display.setPixel(x, y, ((x ^ y) & 1u) ? Color::RED : Color::GREEN);
        }
    }
}

/** Doppelter Rahmen: äußerer in Grün, innerer in Rot. */
static void patternBorders() {
    display.drawRect(0, 0, MATRIX_COLS, MATRIX_ROWS, Color::GREEN);
    display.drawRect(2, 2, MATRIX_COLS - 4, MATRIX_ROWS - 4, Color::RED);
}

// ─── Bouncing-Ball-Demo ───────────────────────────────────────────────────────

struct Ball {
    int16_t x, y;    // Position (Festpunktzahl, ×4 Sub-Pixel für weichere Bewegung)
    int8_t  dx, dy;  // Geschwindigkeit (Sub-Pixel pro Schritt)
};

/** Ein Pixel an Sub-Pixel-Position (x/4, y/4) zeichnen. */
static void ballSetPixel(int16_t subX, int16_t subY, Color color) {
    display.setPixel(static_cast<uint16_t>(subX >> 2),
                     static_cast<uint8_t> (subY >> 2),
                     color);
}

/** Bouncing-Ball-Animation – läuft im Vordergrund, ISR übernimmt Display-Refresh. */
static void demoBouncing() {
    Ball ball = { 4,  4,  3,  2 }; // Startposition und -geschwindigkeit
    Ball ball2 = { (MATRIX_COLS - 1) * 4, (MATRIX_ROWS - 1) * 4, -2, 3 };

    const uint32_t endTime = millis() + DEMO_INTERVAL_MS;

    while (millis() < endTime) {
        // Alten Ball-Pixel löschen
        ballSetPixel(ball.x,  ball.y,  Color::OFF);
        ballSetPixel(ball2.x, ball2.y, Color::OFF);

        // Positionen aktualisieren
        ball.x  += ball.dx;
        ball.y  += ball.dy;
        ball2.x += ball2.dx;
        ball2.y += ball2.dy;

        // Wandkollision X (0 … (MATRIX_COLS-1)*4)
        constexpr int16_t maxX = static_cast<int16_t>((MATRIX_COLS - 1) * 4);
        constexpr int16_t maxY = static_cast<int16_t>((MATRIX_ROWS - 1) * 4);

        if (ball.x <= 0)    { ball.x = 0;    ball.dx = -ball.dx; }
        if (ball.x >= maxX) { ball.x = maxX; ball.dx = -ball.dx; }
        if (ball.y <= 0)    { ball.y = 0;    ball.dy = -ball.dy; }
        if (ball.y >= maxY) { ball.y = maxY; ball.dy = -ball.dy; }

        if (ball2.x <= 0)    { ball2.x = 0;    ball2.dx = -ball2.dx; }
        if (ball2.x >= maxX) { ball2.x = maxX; ball2.dx = -ball2.dx; }
        if (ball2.y <= 0)    { ball2.y = 0;    ball2.dy = -ball2.dy; }
        if (ball2.y >= maxY) { ball2.y = maxY; ball2.dy = -ball2.dy; }

        // Neue Ball-Position zeichnen
        ballSetPixel(ball.x,  ball.y,  Color::GREEN);
        ballSetPixel(ball2.x, ball2.y, Color::RED);

        // ISR übernimmt Refresh – kurze Pause damit Geschwindigkeit sichtbar ist
        delay(10);
    }
}

// ─── Demo-Sequenz ─────────────────────────────────────────────────────────────

static void runDemo() {
    // ── 1. Farbbalken ──
    Serial.println(F("Muster: Farbbalken"));
    display.clear();
    patternColorBars();

    display.enableInterruptDriven(false);
    const uint32_t t1 = millis() + DEMO_INTERVAL_MS;
    while (millis() < t1) display.refresh();

    // ── 2. Schachbrett ──
    Serial.println(F("Muster: Schachbrett"));
    display.clear();
    patternCheckerboard();

    const uint32_t t2 = millis() + DEMO_INTERVAL_MS;
    while (millis() < t2) display.refresh();

    // ── 3. Rahmen ──
    Serial.println(F("Muster: Rahmen"));
    display.clear();
    patternBorders();

    const uint32_t t3 = millis() + DEMO_INTERVAL_MS;
    while (millis() < t3) display.refresh();

    // ── 4. Bouncing Ball (ISR-betrieben) ──
    Serial.println(F("Muster: Bouncing Ball (Timer-ISR)"));
    display.clear();
    display.enableInterruptDriven(true);  // ab jetzt übernimmt Timer1 den Refresh
    demoBouncing();
    display.enableInterruptDriven(false);
}

// ─── Arduino-Einstiegspunkte ──────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    Serial.println(F("=== Zugzielanzeige Matrix-LED Treiber ==="));
    Serial.print(F("Aufloesung: "));
    Serial.print(MATRIX_COLS);
    Serial.print(F(" x "));
    Serial.println(MATRIX_ROWS);
    Serial.print(F("Panels: "));
    Serial.println(MATRIX_NUM_PANELS);

    display.begin();
    display.setBrightness(3);

    // Kurzen Selbsttest ausgeben: alle LEDs kurz an
    display.clear(Color::ORANGE);
    display.refresh(500); // 500 µs pro Zeile → ~8 ms sichtbarer Aufleuchter
    display.clear();
}

void loop() {
    runDemo();
}

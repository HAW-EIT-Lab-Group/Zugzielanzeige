/**
 * Einzelkabel-Test für die 64×200 Bi-Color LED-Matrix (Zugzielanzeige)
 *
 * Testet ein einzelnes Flachbandkabel (16 Zeilen × 200 Spalten).
 * Die Muster wechseln alle paar Sekunden automatisch; über Serial kann
 * der aktuelle Zustand mitgelesen werden (115200 Baud).
 *
 * Testmuster-Reihenfolge:
 *   1. Alle LEDs Grün       – Datenpfad grün OK?
 *   2. Alle LEDs Rot        – Datenpfad rot OK?
 *   3. Alle LEDs Orange     – beide Kanäle gleichzeitig OK?
 *   4. Schachbrett G/R      – Spaltenauflösung OK?
 *   5. Zeilenläufer         – alle 16 Zeilen einzeln, Zeilenadressierung OK?
 *   6. Spaltenläufer        – einzelne Spalte wandert, Takt/Latch OK?
 */

#include <Arduino.h>

// ─── Pin-Definitionen ─────────────────────────────────────────────────────────

#define PIN_G   22   // Data Green
#define PIN_R   24   // Data Red
#define PIN_CLK 26   // Clock (Schieberegister-Takt)
#define PIN_LAT 28   // Latch (Datenübernahme/Strobe)
// Arduino Mega definiert PIN_A0–A7 bereits für Analogpins – überschreiben
#undef PIN_A0
#undef PIN_A1
#undef PIN_A2
#define PIN_A0  30   // Zeilenadresse Bit 0
#define PIN_A1  32   // Zeilenadresse Bit 1
#define PIN_A2  34   // Zeilenadresse Bit 2
#define PIN_CS  36   // Chip-Select: LOW = Zeilen 0–7, HIGH = Zeilen 8–15
#define PIN_EN1 38   // Enable / Helligkeit 1
#define PIN_EN2 39   // Enable / Helligkeit 2

// ─── Display-Konstanten ───────────────────────────────────────────────────────

static constexpr uint16_t NUM_COLS    = 200; // Spalten pro Zeile
static constexpr uint8_t  NUM_ROWS    = 16;  // Zeilen pro Flachbandkabel
static constexpr uint16_t PATTERN_MS  = 4000; // Anzeigedauer pro Muster (ms)

// ─── Niederpegel-Hilfsfunktionen ──────────────────────────────────────────────

/**
 * Zeilenadresse setzen.
 * row 0–7:  CS=LOW,  A0/A1/A2 = row
 * row 8–15: CS=HIGH, A0/A1/A2 = row - 8
 */
static void setRowAddr(uint8_t row) {
    uint8_t addr = row & 0x07; // untere 3 Bits
    digitalWrite(PIN_A0, (addr & 0x01) ? HIGH : LOW);
    digitalWrite(PIN_A1, (addr & 0x02) ? HIGH : LOW);
    digitalWrite(PIN_A2, (addr & 0x04) ? HIGH : LOW);
    digitalWrite(PIN_CS,  row >= 8     ? HIGH : LOW);
}

/** Latch-Impuls: Schieberegister-Inhalt in Ausgangslatch übernehmen. */
static void latchData() {
    digitalWrite(PIN_LAT, HIGH);
    digitalWrite(PIN_LAT, LOW);
}

/**
 * Display ein- oder ausschalten.
 * Hinweis: Falls das Display aktiv-low ist, HIGH/LOW hier vertauschen.
 */
static void enableDisplay(bool on) {
    digitalWrite(PIN_EN1, on ? HIGH : LOW);
    digitalWrite(PIN_EN2, on ? HIGH : LOW);
}

/**
 * Einen CLK-Impuls erzeugen und dabei die Datenpins setzen.
 * Wird für jede Spalte einmal aufgerufen.
 */
static inline void clockBit(bool green, bool red) {
    digitalWrite(PIN_G,   green ? HIGH : LOW);
    digitalWrite(PIN_R,   red   ? HIGH : LOW);
    digitalWrite(PIN_CLK, HIGH);
    digitalWrite(PIN_CLK, LOW);
}

// ─── Scan-Funktion ────────────────────────────────────────────────────────────

/**
 * Eine Zeile vollständig ausgeben.
 *
 * @param row    Zeilennummer 0–15
 * @param green  Callback: liefert true wenn Spalte col grün sein soll
 * @param red    Callback: liefert true wenn Spalte col rot sein soll
 *
 * Ablauf: Enable aus → 200 Bits einschieben → Adresse setzen → Latch → Enable an
 */
static void scanRow(uint8_t row,
                    bool (*green)(uint8_t row, uint16_t col),
                    bool (*red  )(uint8_t row, uint16_t col))
{
    enableDisplay(false); // Ausgabe sperren – verhindert Geisterbilder

    for (uint16_t col = 0; col < NUM_COLS; ++col) {
        clockBit(green(row, col), red(row, col));
    }

    setRowAddr(row);
    latchData();
    enableDisplay(true);
}

/**
 * Alle 16 Zeilen einmal durchscannen (ein komplettes Frame).
 * Wartet zwischen den Zeilen rowDwellUs Mikrosekunden.
 */
static void refreshFrame(bool (*green)(uint8_t, uint16_t),
                          bool (*red  )(uint8_t, uint16_t),
                          uint16_t rowDwellUs = 300)
{
    for (uint8_t row = 0; row < NUM_ROWS; ++row) {
        scanRow(row, green, red);
        if (rowDwellUs) delayMicroseconds(rowDwellUs);
    }
}

// ─── Testmuster-Datenfunktionen ───────────────────────────────────────────────

// Alle Pixel grün
static bool allGreen (uint8_t, uint16_t) { return true;  }
static bool allRed   (uint8_t, uint16_t) { return true;  }
static bool noPixel  (uint8_t, uint16_t) { return false; }

// Schachbrett: gerade Spalten grün in geraden Zeilen, ungerade Spalten grün in ungeraden
static bool chessGreen(uint8_t row, uint16_t col) { return ((col + row) & 1) == 0; }
static bool chessRed  (uint8_t row, uint16_t col) { return ((col + row) & 1) == 1; }

// Zeilenläufer: nur die aktuelle Laufzeile leuchtet
static uint8_t  walkRow = 0;
static bool rowWalkGreen(uint8_t row, uint16_t)     { return row == walkRow; }
static bool rowWalkRed  (uint8_t row, uint16_t col) { return row == walkRow && (col & 1); }

// Spaltenläufer: eine einzelne Spalte wandert durch alle 200 Positionen
static uint16_t walkCol = 0;
static bool colWalkGreen(uint8_t, uint16_t col) { return col == walkCol; }
static bool colWalkRed  (uint8_t, uint16_t col) { return col == walkCol; }

// ─── Testmuster-Schleife ──────────────────────────────────────────────────────

/**
 * Wiederholt refreshFrame für PATTERN_MS Millisekunden.
 * Zwischen jedem Frame kann optionale Logik (update) ausgeführt werden.
 */
static void runPattern(const char* name,
                       bool (*green)(uint8_t, uint16_t),
                       bool (*red  )(uint8_t, uint16_t),
                       void (*update)() = nullptr)
{
    Serial.print(F("Muster: "));
    Serial.println(name);

    uint32_t end = millis() + PATTERN_MS;
    while (millis() < end) {
        refreshFrame(green, red);
        if (update) update();
    }
}

// Update-Callbacks für die laufenden Muster
static void nextWalkRow() {
    static uint32_t lastStep = 0;
    if (millis() - lastStep > 200) { // alle 200 ms eine Zeile weiter
        walkRow = (walkRow + 1) % NUM_ROWS;
        lastStep = millis();
    }
}

static void nextWalkCol() {
    static uint32_t lastStep = 0;
    if (millis() - lastStep > 15) { // alle 15 ms eine Spalte weiter
        walkCol = (walkCol + 1) % NUM_COLS;
        lastStep = millis();
    }
}

// ─── Arduino-Einstiegspunkte ──────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);

    const uint8_t pins[] = { PIN_G, PIN_R, PIN_CLK, PIN_LAT,
                              PIN_A0, PIN_A1, PIN_A2, PIN_CS,
                              PIN_EN1, PIN_EN2 };
    for (uint8_t p : pins) {
        pinMode(p, OUTPUT);
        digitalWrite(p, LOW);
    }
    // EN1/EN2 dauerhaft LOW – nicht mehr anfassen
}

// ─── Kombinations-Test ────────────────────────────────────────────────────────
// Probiert alle Kombinationen von Enable-Polarität und CS-Polarität durch.
// Jede Kombination leuchtet 1 Sekunde – Serial Monitor zeigt welche gerade läuft.

struct Combo {
    const char* name;
    bool enOnHigh; // true = EN HIGH ist "an", false = EN LOW ist "an"
    bool csLowFor0; // true = CS LOW für Zeilen 0-7, false = CS HIGH für Zeilen 0-7
};

static const Combo combos[] = {
    { "EN=HIGH-an  CS=LOW-fuer-0-7",  true,  true  },
    { "EN=HIGH-an  CS=HIGH-fuer-0-7", true,  false },
    { "EN=LOW-an   CS=LOW-fuer-0-7",  false, true  },
    { "EN=LOW-an   CS=HIGH-fuer-0-7", false, false },
};

void loop() {
    for (const Combo& c : combos) {
        Serial.print(F("Teste: ")); Serial.println(c.name);

        // Daten einschieben
        for (uint16_t col = 0; col < NUM_COLS; col++) {
            clockBit(true, true); // Orange
        }

        // Zeilenadresse 0 setzen mit dieser CS-Polarität
        digitalWrite(PIN_A0, LOW);
        digitalWrite(PIN_A1, LOW);
        digitalWrite(PIN_A2, LOW);
        digitalWrite(PIN_CS, c.csLowFor0 ? LOW : HIGH);

        latchData();

        // Enable auf "an" schalten
        uint8_t enOn = c.enOnHigh ? HIGH : LOW;
        digitalWrite(PIN_EN1, enOn);
        digitalWrite(PIN_EN2, enOn);

        delay(1000); // 1 Sekunde leuchten lassen

        // Enable aus
        uint8_t enOff = c.enOnHigh ? LOW : HIGH;
        digitalWrite(PIN_EN1, enOff);
        digitalWrite(PIN_EN2, enOff);

        delay(300); // kurze Pause zwischen Kombinationen
    }
}

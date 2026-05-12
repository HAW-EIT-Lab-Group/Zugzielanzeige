/**
 * MatrixDisplay.cpp
 *
 * Implementierung des Scan-Treibers für die 64×200-Bi-Color-LED-Matrix.
 *
 * Scan-Ablauf pro Zeile (rowAddr 0…15)
 * ─────────────────────────────────────
 * 1. Enable ausschalten  → verhindert Geisterbilder während des Umschaltens
 * 2. 200 × CLK-Impuls    → Daten für alle 4 Panels gleichzeitig in PORTC
 * 3. Zeilenadresse setzen (A0/A1/A2 + CS in PORTL)
 * 4. LATCH-Impuls        → Daten vom Schieberegister ins Ausgangslatch übernehmen
 * 5. Enable einschalten  → neue Zeile leuchtet
 *
 * Timing-Optimierung
 * ──────────────────
 * Kritischer Pfad ist die innere Schleife: 200 Bits × (PORTC schreiben +
 * CLK-High + CLK-Low) = ~600 Register-Zugriffe pro Scan-Zeile. Durch Nutzung
 * von PORTC/PORTL statt digitalWrite() dauert ein CLK-Zyklus ca. 125 ns
 * (2 Takte @ 16 MHz), was einer theoretischen Datenrate von 8 Mbit/s entspricht.
 */

#include "MatrixDisplay.h"
#include "matrix_config.h"
#include <avr/pgmspace.h>
#include <string.h>

// Globaler Zeiger für den Timer1-ISR (nur ein Display-Objekt zulässig)
static MatrixDisplay* _isrInstance = nullptr;

// ─── Initialisierung ──────────────────────────────────────────────────────────

void MatrixDisplay::begin() {
    // DDRC und DDRL vollständig auf Ausgang setzen
    DDRC = 0xFF;
    DDRL = 0xFF;

    // Alle Ausgänge auf Low (Display aus, CLK/LATCH inaktiv)
    PORTC = 0x00;
    PORTL = 0x00;

    memset(_greenBuf, 0, sizeof(_greenBuf));
    memset(_redBuf,   0, sizeof(_redBuf));

    _scanRow    = 0;
    _brightness = 3;
}

// ─── Pixelzugriff ─────────────────────────────────────────────────────────────

void MatrixDisplay::setPixel(uint16_t x, uint8_t y, Color color) {
    if (x >= MATRIX_COLS || y >= MATRIX_ROWS) return;

    const uint8_t byteIdx = static_cast<uint8_t>(x >> 3);
    const uint8_t bitMask = 0x80u >> (x & 7u); // MSB-first: Spalte 0 = Bit 7

    const uint8_t c = static_cast<uint8_t>(color);
    if (c & 0x01) _greenBuf[y][byteIdx] |=  bitMask;
    else          _greenBuf[y][byteIdx] &= ~bitMask;
    if (c & 0x02) _redBuf  [y][byteIdx] |=  bitMask;
    else          _redBuf  [y][byteIdx] &= ~bitMask;
}

Color MatrixDisplay::getPixel(uint16_t x, uint8_t y) const {
    if (x >= MATRIX_COLS || y >= MATRIX_ROWS) return Color::OFF;

    const uint8_t byteIdx = static_cast<uint8_t>(x >> 3);
    const uint8_t bitMask = 0x80u >> (x & 7u);

    const uint8_t g = (_greenBuf[y][byteIdx] & bitMask) ? 0x01u : 0u;
    const uint8_t r = (_redBuf  [y][byteIdx] & bitMask) ? 0x02u : 0u;
    return static_cast<Color>(g | r);
}

// ─── Grafik-Primitiven ────────────────────────────────────────────────────────

void MatrixDisplay::clear(Color color) {
    const uint8_t gFill = (static_cast<uint8_t>(color) & 0x01u) ? 0xFF : 0x00;
    const uint8_t rFill = (static_cast<uint8_t>(color) & 0x02u) ? 0xFF : 0x00;
    memset(_greenBuf, gFill, sizeof(_greenBuf));
    memset(_redBuf,   rFill, sizeof(_redBuf));
}

void MatrixDisplay::drawHLine(uint16_t x1, uint16_t x2, uint8_t y, Color color) {
    const uint16_t xEnd = min(x2, static_cast<uint16_t>(MATRIX_COLS - 1));
    for (uint16_t x = x1; x <= xEnd; ++x)
        setPixel(x, y, color);
}

void MatrixDisplay::drawVLine(uint8_t y1, uint8_t y2, uint16_t x, Color color) {
    const uint8_t yEnd = min(y2, static_cast<uint8_t>(MATRIX_ROWS - 1));
    for (uint8_t y = y1; y <= yEnd; ++y)
        setPixel(x, y, color);
}

void MatrixDisplay::drawRect(uint16_t x, uint8_t y, uint16_t w, uint8_t h, Color color) {
    drawHLine(x, x + w - 1, y,         color);
    drawHLine(x, x + w - 1, y + h - 1, color);
    drawVLine(y, y + h - 1, x,         color);
    drawVLine(y, y + h - 1, x + w - 1, color);
}

void MatrixDisplay::fillRect(uint16_t x, uint8_t y, uint16_t w, uint8_t h, Color color) {
    const uint8_t yEnd = min(static_cast<uint16_t>(y + h - 1),
                             static_cast<uint16_t>(MATRIX_ROWS - 1));
    for (uint8_t row = y; row <= yEnd; ++row)
        drawHLine(x, x + w - 1, row, color);
}

void MatrixDisplay::drawBitmap(const uint8_t* bitmap, uint16_t x, uint8_t y,
                               uint16_t w, uint8_t h, Color color) {
    const uint8_t stride = static_cast<uint8_t>((w + 7u) / 8u);
    for (uint8_t row = 0; row < h; ++row) {
        for (uint16_t col = 0; col < w; ++col) {
            const uint8_t byteVal = pgm_read_byte(bitmap + row * stride + (col >> 3));
            if (byteVal & (0x80u >> (col & 7u)))
                setPixel(x + col, y + row, color);
        }
    }
}

// ─── Interne Hilfsfunktionen ──────────────────────────────────────────────────

void MatrixDisplay::_setRowAddress(uint8_t rowAddr) {
    // A0/A1/A2 auf PORTL-Bits 2–4, CS auf Bit 5 (HIGH wenn rowAddr >= 8)
    uint8_t portVal = PORTL & ~PL_ADDR_MASK;
    portVal |= static_cast<uint8_t>((rowAddr & 0x07u) << 2u);
    if (rowAddr >= 8u) portVal |= PL_CS;
    PORTL = portVal;
}

void MatrixDisplay::_setEnable(bool on) {
    uint8_t portVal = PORTL & ~(PL_EN1 | PL_EN2);
    if (on) {
        // Helligkeit 0 = aus, 1 = EN1, 2 = EN1+EN2, 3 = EN1+EN2 (beide voll)
        // Anpassen falls die Hardware anders reagiert.
        if (_brightness >= 1u) portVal |= PL_EN1;
        if (_brightness >= 2u) portVal |= PL_EN2;
    }
    PORTL = portVal;
}

void MatrixDisplay::_shiftAndLatchRow(uint8_t rowAddr) {
    // 1. Ausgabe deaktivieren – verhindert Artefakte beim Umschalten
    _setEnable(false);

    // 2. Zeilendaten für alle 4 Panels parallel in die Schieberegister takten.
    //    Pro CLK-Impuls werden alle 8 Datenpins (PORTC) gleichzeitig gesetzt.
    //    PORTC-Bitmapping (PC0…PC7 = Pin 37…30):
    //      Bit 0: Panel 0 Grün   Bit 1: Panel 0 Rot
    //      Bit 2: Panel 1 Grün   Bit 3: Panel 1 Rot
    //      Bit 4: Panel 2 Grün   Bit 5: Panel 2 Rot
    //      Bit 6: Panel 3 Grün   Bit 7: Panel 3 Rot
    for (uint8_t byteIdx = 0; byteIdx < MATRIX_BYTES_ROW; ++byteIdx) {

        // Bytes der 4 physikalischen Zeilen (je 16 Zeilen Abstand) vorab lesen
        // – vermeidet wiederholte Array-Indizierung im inneren Bit-Loop.
        const uint8_t g0 = _greenBuf[0u * MATRIX_ROWS_PANEL + rowAddr][byteIdx];
        const uint8_t r0 = _redBuf  [0u * MATRIX_ROWS_PANEL + rowAddr][byteIdx];
        const uint8_t g1 = _greenBuf[1u * MATRIX_ROWS_PANEL + rowAddr][byteIdx];
        const uint8_t r1 = _redBuf  [1u * MATRIX_ROWS_PANEL + rowAddr][byteIdx];
        const uint8_t g2 = _greenBuf[2u * MATRIX_ROWS_PANEL + rowAddr][byteIdx];
        const uint8_t r2 = _redBuf  [2u * MATRIX_ROWS_PANEL + rowAddr][byteIdx];
        const uint8_t g3 = _greenBuf[3u * MATRIX_ROWS_PANEL + rowAddr][byteIdx];
        const uint8_t r3 = _redBuf  [3u * MATRIX_ROWS_PANEL + rowAddr][byteIdx];

        // 8 Bits MSB-first durchlaufen (Bit 7 = linkste Spalte des Bytes)
        uint8_t mask = 0x80u;
        do {
            PORTC = static_cast<uint8_t>(
                ((g0 & mask) ? 0x01u : 0u) |
                ((r0 & mask) ? 0x02u : 0u) |
                ((g1 & mask) ? 0x04u : 0u) |
                ((r1 & mask) ? 0x08u : 0u) |
                ((g2 & mask) ? 0x10u : 0u) |
                ((r2 & mask) ? 0x20u : 0u) |
                ((g3 & mask) ? 0x40u : 0u) |
                ((r3 & mask) ? 0x80u : 0u)
            );
            // CLK-Impuls (High → Low)
            PORTL |=  PL_CLK;
            PORTL &= ~PL_CLK;
            mask >>= 1u;
        } while (mask != 0u);
    }

    // 3. Zeilenadresse setzen (A0/A1/A2 + CS)
    _setRowAddress(rowAddr);

    // 4. LATCH-Impuls: Schieberegister-Inhalt in Ausgangslatch übernehmen
    PORTL |=  PL_LATCH;
    PORTL &= ~PL_LATCH;

    // 5. Ausgabe aktivieren – neue Zeile leuchtet
    _setEnable(true);
}

// ─── Öffentliche Refresh-Methoden ─────────────────────────────────────────────

void MatrixDisplay::scanRow() {
    _shiftAndLatchRow(_scanRow);
    _scanRow = (_scanRow + 1u) % MATRIX_ROWS_PANEL; // rollt nach 15 → 0
}

void MatrixDisplay::refresh(uint16_t rowDwellUs) {
    for (uint8_t row = 0u; row < MATRIX_ROWS_PANEL; ++row) {
        _shiftAndLatchRow(row);
        if (rowDwellUs > 0u) delayMicroseconds(rowDwellUs);
    }
}

void MatrixDisplay::setBrightness(uint8_t level) {
    _brightness = constrain(level, 0u, 3u);
}

// ─── Timer1-ISR-Betrieb ───────────────────────────────────────────────────────

void MatrixDisplay::enableInterruptDriven(bool enable) {
    if (enable) {
        _isrInstance = this;

        // Timer1 CTC-Modus, Prescaler 8:
        //   16 MHz / 8 = 2 000 000 Ticks/s
        //   1,25 ms (50 Hz / 16 Zeilen) = 2500 Ticks → OCR1A = 2499
        TCCR1A = 0;
        TCCR1B = (1u << WGM12) | (1u << CS11); // CTC + Prescaler 8
        OCR1A  = 2499u;
        TIMSK1 = (1u << OCIE1A);               // Compare-Match-Interrupt freigeben

    } else {
        TIMSK1     = 0;           // Interrupt sperren
        _isrInstance = nullptr;
    }
}

// ISR muss im globalen Namensraum definiert werden.
ISR(TIMER1_COMPA_vect) {
    if (_isrInstance) _isrInstance->scanRow();
}

#include <Arduino.h>
#include <avr/pgmspace.h>
#include "Game.h"
#include "Graphics.h"
#include "config.h"
#include "MazeData.h"
#include "SNESpad.h"



enum Input{TASTATUR, CONTROLLER};
#define EINGABEMODUS CONTROLLER

// Steuerung bei EINGABEMODUS TASTATUR: über Serial (9600 Baud) mit W/A/S/D, Neustart nach Sieg/Niederlage mit R.
// Ein Terminal ohne Zeilenpuffer (z.B. "pio device monitor" oder PuTTY) reagiert
// sofort auf Tastendruck; die Arduino-IDE-Konsole sendet erst nach Enter.
//
// Steuerung bei EINGABEMODUS CONTROLLER: ...
//
// Das Labyrinth ist nicht mehr generiert, sondern kommt direkt aus dem
// selbst gerenderten bild.png (siehe MazeData.h): Graphics::loadImage()
// zeichnet einmalig das komplette Bild (Rahmen-Logo links, Highscore-Text
// rechts, Wände in der Mitte). Punkte, Pacman und Geist werden zur Laufzeit
// nur innerhalb des Labyrinth-Bereichs [MAZE_X0,MAZE_X1] gezeichnet/bewegt -
// die Wanddaten liegen im Flash (PROGMEM), nicht im SRAM.



//SNES Controller: Pin-Belegung
#define CLOCK 5
#define LATCH 6
#define DATA0 7
#define DATA1 -1 // optional (required for super multitap, scope, xband keyboard, etc)
#define IOSEL -1 //

SNESpad snespad(CLOCK, LATCH, DATA0, DATA1, IOSEL);

// Mindestabstand zwischen zwei Controllerabfragen (s. eingabeLesen())
#define CONTROLLER_POLL_MS 15




// Das Labyrinth ist auf einem 9px-Raster gezeichnet (8px Gang + 1px Wand).
// Basis-Offset 1/1 landet auf allen 14x7 Rasterpunkten (per Skript
// verifiziert); nach zweimaliger Rückmeldung vom echten Display insgesamt
// 6 Pixel nach links und 3 nach unten verschoben, Punkte sind 2x2 statt
// 1x1 groß. DOT_COLS ist auf 15 erhöht, weil die Linksverschiebung sonst
// rechts eine Spalte hätte fehlen lassen (per Skript nachgemessen).
#define DOT_PITCH 9
#define DOT_OFFSET_X (1 - 3 - 3)
#define DOT_OFFSET_Y (1 + 3)
#define DOT_SIZE 2
#define DOT_COLS 15
#define DOT_ROWS 7

// Basistakt: bei 100% legt eine Figur pro BASIS_TICK_MS einen Pixel zurück.
// 40ms ist so gewählt, dass Level 1 (Pacman 80%) wieder auf die bisherigen
// 50ms/Pixel kommt - das bisherige Spielgefühl bleibt also erhalten.
#define BASIS_TICK_MS 40
#define LEVEL_STUFEN 4

// Augen sind auf dem Rückweg schneller als alles andere (wie im Original),
// ebenfalls um 15% entschärft
#define AUGEN_PCT 128

// Fällt der Fright-Rest darunter, blinken die Geister doppelt so schnell -
// das ist die Entsprechung zu den "Flashes" der Original-Tabelle und kündigt
// das Ende des Energizers an.
#define FRIGHT_WARN_MS 2000

// Notbremse aus dem Original: wer aufhört zu fressen, bekommt die Geister
// trotzdem. Nach dieser Zeit ohne gefressenen Punkt kommt der nächste raus.
#define FREIGABE_TIMER_L1_MS 4000
#define FREIGABE_TIMER_L5_MS 3000

// Breite der Tunnelzone an beiden Rändern, in der Geister ausgebremst werden
#define TUNNEL_ZONE 15

// Extraleben bei diesen Punktzahlen (je einmal pro Spiel)
#define BONUS_LEBEN_1 1000
#define BONUS_LEBEN_2 3000

// Energizer: vier pro Level. Das Spielfeld wird in vier Quadranten geteilt
// (links/rechts x oben/unten), in jedem liegt genau einer an zufälliger
// Stelle. Sie ersetzen jeweils einen normalen Punkt. Gültige Punktzellen je
// Quadrant (per Skript geprüft): 18 / 24 / 24 / 30 - überall genug Auswahl.
#define ENERGIZER_ANZAHL 4
#define PUNKTE_ENERGIZER 50
// Gefressene Geister während EINES Energizers: 200/400/800/1600.
// Der Multiplikator wird beim Ablaufen des Energizers zurückgesetzt,
// nicht pro Level.
#define PUNKTE_GEIST_BASIS 200
#define GEIST_KETTE_MAX 4

#define BLINK_MS 250 // Takt für Energizer- und Frightened-Blinken

#define LEVEL_TEXT_MS 2500 // Anzeigedauer des "LEVEL x REACHED"-Bildschirms
#define TEXT_W 3
#define TEXT_H 5
#define TEXT_PITCH 4
#define MUND_WECHSEL 4 // alle N Schritte wechselt Pacman zwischen offen und zu


// Pacman/Geist-Sprites (Grafik) sind 6x6 Pixel, um (x,y) herum von -2 bis +3
// versetzt. Pacman zusätzlich 3px tiefer gezeichnet (Rückmeldung: wirkte in
// den Gängen nicht vertikal zentriert) - betrifft nur die Zeichenposition.
#define SPRITE_MIN -2
#define SPRITE_MAX 3
#define SPRITE_SIZE 6
// Reine Zeichenversätze (betreffen nie die Kollision/Hitbox):
// Pacman: 1 weiter nach links, insgesamt 1 nach oben (vorher +3, jetzt -4 mehr)
// Geist: 1 nach links, 1 nach oben
#define PACMAN_X_SHIFT -1
#define PACMAN_Y_SHIFT -1
#define GEIST_X_SHIFT -1
#define GEIST_Y_SHIFT -1

// Kollisionsbox (Wände + Pacman<->Geist) ist unabhängig von der 6x6-Grafik
// wirklich 8x8 Pixel groß, um (x,y) herum von -4 bis +3 versetzt. Damit die
// Box immer komplett im Labyrinth liegt, muss der Ankerpunkt (x,y) selbst in
// [ANCHOR_X_MIN,ANCHOR_X_MAX]x[ANCHOR_Y_MIN,ANCHOR_Y_MAX] bleiben - per
// Skript gegen bild.png simuliert: von jedem Punkt aus ist so jeder andere
// erreichbar, nirgends bleibt die Box stecken.
#define HITBOX_MIN -4
#define HITBOX_MAX 3
#define HITBOX_SIZE 6
#define ANCHOR_X_MIN (MAZE_X0 - HITBOX_MIN)
#define ANCHOR_X_MAX (MAZE_X1 - HITBOX_MAX)
#define ANCHOR_Y_MIN (0 - HITBOX_MIN)
#define ANCHOR_Y_MAX (MAZE_H - 1 - HITBOX_MAX)
// einzige Zeile, in der eine 8px hohe Box exakt in die 8 Zeilen hohe
// Tunnelöffnung [TUNNEL_Y0,TUNNEL_Y1] passt
#define TUNNEL_MID (TUNNEL_Y0 - HITBOX_MIN)

// Geisterhaus aus bild.png (per Skript ausgemessen):
// Wände x=62 und x=80, oben y=27, unten y=36, Innenraum x=63..79, y=28..35.
// Die Tür ist nur x=68..74 (7 Pixel) breit - die 8x8-Hitbox passt dort NICHT
// hindurch, und die gemalten Deko-Geister im Haus haben grüne Augen, die als
// Wand zählen. Geister im Haus laufen deshalb auf einem festen Pfad heraus und
// ignorieren dabei die Wandprüfung (im Original ist die Tür ebenfalls ein
// Sonderfeld, das nur Geister passieren können). Pacman kommt mangels Platz
// ohnehin nie ins Haus.
#define HAUS_GATE_X 71    // Mitte der Tür
#define HAUS_EXIT_Y 23    // erster boxfreier Anker oberhalb der Tür
#define HAUS_WARTE_Y 32   // Wartehöhe im Haus
#define HAUS_ERHOLUNG_MS 2000 // so lange bleibt ein gefressener Geist in der Hausmitte
#define HAUS_PINKY_X 67
#define HAUS_INKY_X 75

// Innenraum des Hauses (ohne Wände). Dieser Bereich wird komplett schwarz
// gehalten: im Originalbild sind dort zwei Geister aufgemalt (rote Körper,
// grüne Augen). Die würden sonst bei jedem loadImage() bzw. beim Löschen
// eines Geist-Sprites wieder auftauchen und dort auch Punkte erzeugen.
#define HAUS_X0 63
#define HAUS_X1 79
#define HAUS_Y0 28
#define HAUS_Y1 35

// Ab wie vielen gegessenen Punkten die Geister das Haus verlassen.
// Blinky ist von Anfang an draußen und läuft sofort los.
#define PINKY_DOTS 20
#define INKY_DOTS 50

// So viele Punkte vor der Freigabe fängt der wartende Geist an zu blinken
// (Sprite <-> dunkel), damit man sein Erscheinen kommen sieht
#define SPAWN_WARNUNG_DOTS 5

#define KACHEL DOT_PITCH // eine "Kachel" des Original-Spiels = 9 px Raster

// ---- HUD (Positionen aus bild.png ausgemessen) ----
// Ziffernraster: 8 Ziffern, je 3x5 Pixel, Abstand 4 Pixel, erste Spalte x=136
#define HUD_DIGIT_X0 136
#define HUD_DIGIT_PITCH 4
#define HUD_DIGIT_W 3
#define HUD_DIGIT_H 5
#define HUD_DIGITS 8
#define HIGHSCORE_Y 7
#define SCORE_Y 22
#define PUNKTE_PRO_DOT 10

// Leben-Icons oben links: 3 Pacman-Symbole, je 6x6 bei x=1
#define LEBEN_ICON_X 1
#define LEBEN_MAX 3

// Ablauf beim Sterben, in drei Schritten:
//  1. alles friert ein, die Geister bleiben stehen wo sie sind - so sieht man,
//     welcher Geist einen erwischt hat
//  2. die Geister verschwinden, Pacman fällt in sich zusammen (Animation)
//  3. kurze Pause, dann Leben abziehen und neu aufstellen bzw. Game Over
#define TOD_FREEZE_MS 1000
#define TOD_ANIM_MS 1500
#define TOD_PAUSE_MS 400
#define TOD_FRAMES 5

// Game-Over-Bildschirm: der Hinweis erscheint später als die Eingabe frei
// wird - man kann also schon neu starten, bevor "PRESS BUTTON" dasteht
#define GAMEOVER_EINGABE_MS 1000
#define GAMEOVER_PROMPT_MS 3000

// Startbildschirm und Countdown vor dem Losspielen
#define START_SCALE 5 // Vergrößerung des Pacman-Symbols
#define BEREIT_MS 1000

// Vier gelbe Pixel unten links im Originalbild, die nichts darstellen
#define ARTEFAKT_X0 3
#define ARTEFAKT_X1 4
#define ARTEFAKT_Y0 58
#define ARTEFAKT_Y1 59

enum Richtung { OBEN, UNTEN, LINKS, RECHTS, KEINE };
enum SpielStatus { STARTBILDSCHIRM, BEREIT, LAEUFT, TOD_FREEZE, TOD_ANIM, TOD_WARTEN,
                   LEVEL_GESCHAFFT, GAMEOVER };
// AUGEN: gefressener Geist, der als reines Augenpaar zurück ins Haus wandert.
// In diesem Zustand hat er KEINE Hitbox - er kann Pacman weder töten noch
// erneut gefressen werden.
// ERHOLT_SICH: die Augen sind in der Hausmitte angekommen, der Geist ist
// wieder komplett und wartet HAUS_ERHOLUNG_MS, bevor er hinausläuft. Auch
// hier keine Hitbox (Pacman passt ohnehin nicht ins Haus).
enum GeistZustand { IM_HAUS, VERLAESST_HAUS, DRAUSSEN, AUGEN, ERHOLT_SICH };

// Aus bild.png ausgeschnittenes Pacman-Sprite (0=schwarz,3=gelb), Mundöffnung
// nach rechts. Für die anderen 3 Richtungen von Hand gespiegelt/gedreht, da
// die Grafik nicht symmetrisch genug ist, um das per Formel zu drehen.
static const uint8_t pacmanIconRechts[SPRITE_SIZE][SPRITE_SIZE] PROGMEM = {
    {0,3,3,3,3,0},
    {3,3,3,3,3,3},
    {3,3,3,3,0,0},
    {3,3,3,0,0,0},
    {3,3,3,3,3,3},
    {0,3,3,3,3,0},
};
static const uint8_t pacmanIconLinks[SPRITE_SIZE][SPRITE_SIZE] PROGMEM = {
    {0,3,3,3,3,0},
    {3,3,3,3,3,3},
    {0,0,3,3,3,3},
    {0,0,0,3,3,3},
    {3,3,3,3,3,3},
    {0,3,3,3,3,0},
};
static const uint8_t pacmanIconOben[SPRITE_SIZE][SPRITE_SIZE] PROGMEM = {
    {0,3,0,0,3,0},
    {3,3,0,0,3,3},
    {3,3,3,0,3,3},
    {3,3,3,3,3,3},
    {3,3,3,3,3,3},
    {0,3,3,3,3,0},
};
static const uint8_t pacmanIconUnten[SPRITE_SIZE][SPRITE_SIZE] PROGMEM = {
    {0,3,3,3,3,0},
    {3,3,3,3,3,3},
    {3,3,3,3,3,3},
    {3,3,3,0,3,3},
    {3,3,0,0,3,3},
    {0,3,0,0,3,0},
};

// geschlossener Mund (voller Kreis) - im Wechsel mit den vier offenen
// Varianten ergibt das die Kau-Animation
static const uint8_t pacmanIconZu[SPRITE_SIZE][SPRITE_SIZE] PROGMEM = {
    {0,3,3,3,3,0},
    {3,3,3,3,3,3},
    {3,3,3,3,3,3},
    {3,3,3,3,3,3},
    {3,3,3,3,3,3},
    {0,3,3,3,3,0},
};

// siehe Analyse der Deko-Icons links (Pacman) und beim Tunnel (Geist)
static const uint8_t geistIcon[SPRITE_SIZE][SPRITE_SIZE] PROGMEM = {
    {0,2,2,2,2,0},
    {2,1,2,2,1,2},
    {2,1,2,2,1,2},
    {2,2,2,2,2,2},
    {2,2,2,2,2,2},
    {2,0,2,2,0,2},
};

// Grün-Variante (Körper grün, Augen rot) - im Wechsel mit geistIcon ergibt
// das während des Energizer-Effekts das rot/grüne Blinken
static const uint8_t geistIconGruen[SPRITE_SIZE][SPRITE_SIZE] PROGMEM = {
    {0,1,1,1,1,0},
    {1,2,1,1,2,1},
    {1,2,1,1,2,1},
    {1,1,1,1,1,1},
    {1,1,1,1,1,1},
    {1,0,1,1,0,1},
};

// Sterbe-Animation: Pacman fällt in sich zusammen, bis nichts mehr übrig ist
static const uint8_t todIcon[TOD_FRAMES][SPRITE_SIZE][SPRITE_SIZE] PROGMEM = {
    {{0,3,3,3,3,0},
     {3,3,3,3,3,3},
     {3,3,3,3,3,3},
     {3,3,3,3,3,3},
     {3,3,3,3,3,3},
     {0,3,3,3,3,0}},
    {{0,0,0,0,0,0},
     {0,3,3,3,3,0},
     {0,3,3,3,3,0},
     {0,3,3,3,3,0},
     {0,3,3,3,3,0},
     {0,0,0,0,0,0}},
    {{0,0,0,0,0,0},
     {0,0,0,0,0,0},
     {0,0,3,3,0,0},
     {0,0,3,3,0,0},
     {0,0,0,0,0,0},
     {0,0,0,0,0,0}},
    {{0,0,0,0,0,0},
     {0,0,0,0,0,0},
     {0,0,3,0,0,0},
     {0,0,0,0,0,0},
     {0,0,0,0,0,0},
     {0,0,0,0,0,0}},
    {{0,0,0,0,0,0},
     {0,0,0,0,0,0},
     {0,0,0,0,0,0},
     {0,0,0,0,0,0},
     {0,0,0,0,0,0},
     {0,0,0,0,0,0}},
};

// Nur die beiden grünen Augenbalken - so sieht ein gefressener Geist auf dem
// Rückweg ins Haus aus (ohne Körper, ohne Hitbox)
static const uint8_t augenIcon[SPRITE_SIZE][SPRITE_SIZE] PROGMEM = {
    {0,0,0,0,0,0},
    {0,1,0,0,1,0},
    {0,1,0,0,1,0},
    {0,0,0,0,0,0},
    {0,0,0,0,0,0},
    {0,0,0,0,0,0},
};

// wählt die Pacman-Grafik passend zur aktuellen Bewegungsrichtung, damit die
// Mundöffnung immer in Fahrtrichtung zeigt
static const uint8_t (*pacmanIconFuer(Richtung r))[SPRITE_SIZE]
{
    switch (r)
    {
        case LINKS: return pacmanIconLinks;
        case OBEN: return pacmanIconOben;
        case UNTEN: return pacmanIconUnten;
        default: return pacmanIconRechts;
    }
}


// Ziffern 0-9 als 3x5-Muster (Bit 2 = linke Spalte), Stil wie die
// Platzhalter-Nullen im Originalbild
static const uint8_t ziffernFont[10][HUD_DIGIT_H] PROGMEM = {
    {0b011, 0b101, 0b101, 0b101, 0b110}, // 0
    {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
    {0b110, 0b001, 0b010, 0b100, 0b111}, // 2
    {0b110, 0b001, 0b010, 0b001, 0b110}, // 3
    {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
    {0b111, 0b100, 0b111, 0b001, 0b110}, // 5
    {0b011, 0b100, 0b111, 0b101, 0b110}, // 6
    {0b111, 0b001, 0b010, 0b100, 0b100}, // 7
    {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
    {0b111, 0b101, 0b111, 0b001, 0b110}, // 9
};

// Buchstaben A-Z als 3x5-Muster (fuer den "LEVEL x REACHED"-Bildschirm)
static const uint8_t buchstabenFont[26][TEXT_H] PROGMEM = {
    {0b010,0b101,0b111,0b101,0b101}, // A
    {0b110,0b101,0b110,0b101,0b110}, // B
    {0b011,0b100,0b100,0b100,0b011}, // C
    {0b110,0b101,0b101,0b101,0b110}, // D
    {0b111,0b100,0b110,0b100,0b111}, // E
    {0b111,0b100,0b110,0b100,0b100}, // F
    {0b011,0b100,0b101,0b101,0b011}, // G
    {0b101,0b101,0b111,0b101,0b101}, // H
    {0b111,0b010,0b010,0b010,0b111}, // I
    {0b001,0b001,0b001,0b101,0b010}, // J
    {0b101,0b101,0b110,0b101,0b101}, // K
    {0b100,0b100,0b100,0b100,0b111}, // L
    {0b101,0b111,0b111,0b101,0b101}, // M
    {0b101,0b111,0b111,0b111,0b101}, // N
    {0b010,0b101,0b101,0b101,0b010}, // O
    {0b110,0b101,0b110,0b100,0b100}, // P
    {0b010,0b101,0b101,0b111,0b011}, // Q
    {0b110,0b101,0b110,0b101,0b101}, // R
    {0b011,0b100,0b010,0b001,0b110}, // S
    {0b111,0b010,0b010,0b010,0b010}, // T
    {0b101,0b101,0b101,0b101,0b011}, // U
    {0b101,0b101,0b101,0b101,0b010}, // V
    {0b101,0b101,0b111,0b111,0b101}, // W
    {0b101,0b101,0b010,0b101,0b101}, // X
    {0b101,0b101,0b010,0b010,0b010}, // Y
    {0b111,0b001,0b010,0b100,0b111}, // Z
};

// Pro Level: Tickdauer (kleiner = schneller) und Länge der Geist-Modi.
// Mit steigendem Level werden beide Figuren schneller, die Geister holen
// gegenüber Pacman auf und die Jagdphasen werden länger - das macht das
// Spiel Stufe für Stufe schwerer. Ab LEVEL_STUFEN gilt die letzte Zeile.
// Geschwindigkeiten in Prozent wie in den Original-Tabellen. 100% = ein Pixel
// pro BASIS_TICK_MS. Jede Figur sammelt pro Basistakt ihren Prozentwert in
// einem Akku; bei >=100 macht sie einen Schritt. So lassen sich auch 71% oder
// 105% sauber abbilden, was mit festen ms-Werten nicht ging.
//
// Die Punktzahl-Schwellen (Elroy) sind auf unser Labyrinth umgerechnet:
// wir haben 96 Punkte statt der 244 im Original, also Faktor ~0,39.
struct LevelDaten
{
    uint8_t pacPct, pacDotsPct;             // normal / während er Punkte frisst
    uint8_t pacFrightPct, pacFrightDotsPct; // dasselbe während des Energizers
    uint8_t geistPct, geistFrightPct, tunnelPct;
    uint8_t elroy1Rest, elroy1Pct; // ab so vielen Restpunkten wird Blinky schneller
    uint8_t elroy2Rest, elroy2Pct;
    uint16_t frightMs; // 0 = Energizer gibt nur Punkte, kein Fright (wie ab Level 21)
};
// Stufen: Level 1 | 2-4 | 5-20 | 21+
// Gegenüber den Original-Werten entschärft: alle Pacman-Tempi x0,90 und alle
// Geister-Tempi x0,85 - das Spiel war sonst zu schwer. Die Verhältnisse
// untereinander (Dots-Bremse, Tunnel, Elroy) bleiben dabei erhalten.
static const LevelDaten levelTabelle[LEVEL_STUFEN] PROGMEM = {
    {72, 64, 81, 71,  64, 43, 34,   8, 68,   4, 72, 6000},
    {81, 71, 86, 75,  72, 47, 38,  14, 77,   7, 81, 4000},
    {90, 78, 90, 78,  81, 51, 43,  20, 85,  10, 89, 2000},
    {81, 71, 81, 71,  81, 51, 43,  47, 85,  24, 89,    0},
};

// Scatter/Chase-Phasen in Sekunden, abwechselnd ab Scatter. 0 = Ende der
// Liste, danach wird für immer gejagt. Die 1033s/1-60stel-Kuriosität des ROMs
// ist weggelassen - sie bedeutet praktisch nichts anderes als "Chase forever".
#define PHASEN_MAX 7
static const uint8_t phasenTabelle[3][PHASEN_MAX] PROGMEM = {
    {7, 20, 7, 20, 5, 20, 5}, // Level 1
    {7, 20, 7, 20, 5, 0, 0},  // Level 2-4
    {5, 20, 5, 20, 5, 0, 0},  // Level 5+
};

// y-Positionen der drei Leben-Symbole im Originalbild
static const uint8_t lebenIconY[LEBEN_MAX] = {2, 11, 20};

// pro Punkt-Rasterzelle 1 Bit: wurde der Punkt schon gegessen?
static uint8_t dotEaten[(DOT_COLS * DOT_ROWS + 7) / 8];

static uint8_t pacmanX, pacmanY;
static Richtung pacmanRichtung, gewuenschteRichtung;

// Index 0 = Blinky, 1 = Pinky, 2 = Inky. Die Geister kollidieren bewusst
// NICHT miteinander - im engen Haus müssen sie sich gegenseitig durchdringen
// können, um herauszukommen.
#define GEIST_ANZAHL 3
struct Geist
{
    uint8_t x, y;
    Richtung richtung;
    GeistZustand zustand;
    bool aengstlich; // während des Energizers: fressbar statt tödlich
    unsigned long wartStart; // Beginn der Pause im Haus (Zustand ERHOLT_SICH)
};
static Geist geister[GEIST_ANZAHL];

static bool geistScatter;

static uint16_t punkteUebrig;
static uint16_t dotsGegessen;
static uint32_t punkte;    // 32 Bit: mit Geisterboni sprengt der Stand 16 Bit
static uint32_t highscore; // überlebt Game Over, nur Reset bei Stromausfall
static uint8_t leben;
static uint16_t level;
static uint8_t bonusStufe;  // 0 = keins, 1 = 1000er vergeben, 2 = beide
static uint8_t mundZaehler; // treibt die Kau-Animation
static uint8_t todFrame;    // aktuelles Bild der Sterbe-Animation
static unsigned long bereitZeit;        // Start der 2-Sekunden-Pause
static bool gameoverPromptGezeichnet;   // "PRESS BUTTON" schon eingeblendet?
static SpielStatus spielStatus;

// Energizer: Index der belegten Punkt-Rasterzelle, 0xFFFF = keiner
static uint16_t energizerIdx[ENERGIZER_ANZAHL];

// Energizer-Effekt (global für alle Geister)
static bool frightAktiv;
static unsigned long frightStart;
static uint8_t geisterKette; // 0..GEIST_KETTE_MAX-1, verdoppelt die Punkte

static bool blinkPhase;
static unsigned long blinkStart;

// Basistakt + Geschwindigkeits-Akkus (siehe LevelDaten)
static unsigned long letzterTick;
static uint8_t pacAkku;
static uint8_t geistAkku[GEIST_ANZAHL];
static bool dotImLetztenTick; // Pacman ist langsamer, während er Punkte frisst

// Scatter/Chase-Phasenfolge
static uint8_t phasenIdx;
static unsigned long phasenStart;

// Zeitpunkt des letzten gefressenen Punkts (für den Freigabe-Timer)
static unsigned long letzterDotZeit;

// aus levelTabelle[] geladen
static LevelDaten lvl;

// Kau-Animation: im Wechsel offener Mund (in Laufrichtung) und geschlossener
static const uint8_t (*pacmanIconAktuell())[SPRITE_SIZE]
{
    if ((mundZaehler / MUND_WECHSEL) & 1) return pacmanIconZu;
    return pacmanIconFuer(pacmanRichtung);
}

static unsigned long todZeit;


static void neuesSpiel();
static void spielerStirbt();
static void spielStarten();
static void energizerGefressen();
static void geistGefressen(uint8_t idx);
static void geistSchrittZuZiel(Geist &g, int16_t zielX, int16_t zielY);
static void spritesLoeschen();
static void spritesZeichnen();
static bool spawnWarnungAktiv(uint8_t idx);
static uint8_t levelStufe();
static bool elroyAktiv();

static inline bool bitLesen(const uint8_t *arr, uint16_t i) { return (arr[i >> 3] >> (i & 7)) & 1; }
static inline void bitSetzen(uint8_t *arr, uint16_t i) { arr[i >> 3] |= (1 << (i & 7)); }

// x muss im Bereich [MAZE_X0,MAZE_X1] liegen (siehe naechstePixel/spriteZeichnen),
// sonst würde (x-MAZE_X0) unterlaufen und außerhalb der PROGMEM-Tabelle lesen
static inline bool istWand(uint8_t x, uint8_t y)
{
    uint8_t lx = x - MAZE_X0;
    uint8_t rowByte = pgm_read_byte(&mazeWalls[y][lx >> 3]);
    return (rowByte >> (7 - (lx & 7))) & 1;
}

// true, wenn die komplette 8x8-Box um (x,y) im Labyrinth liegt und frei von
// Wänden ist. x,y als int16_t, damit der Grenz-Vergleich bei sehr kleinen/
// großen Werten nicht durch vorherigen uint8_t-Unter-/Überlauf verfälscht wird
static bool boxFrei(int16_t x, int16_t y)
{
    if (x < ANCHOR_X_MIN || x > ANCHOR_X_MAX) return false;
    if (y < ANCHOR_Y_MIN || y > ANCHOR_Y_MAX) return false;
    for (int8_t dx = HITBOX_MIN; dx <= HITBOX_MAX; dx++)
        for (int8_t dy = HITBOX_MIN; dy <= HITBOX_MAX; dy++)
            if (istWand((uint8_t)(x + dx), (uint8_t)(y + dy))) return false;
    return true;
}

static inline bool imGeisterhaus(uint8_t x, uint8_t y)
{
    return x >= HAUS_X0 && x <= HAUS_X1 && y >= HAUS_Y0 && y <= HAUS_Y1;
}

// Liefert die obere linke Ecke der 2x2-Punktzelle (dc,dr) in x/y, false falls
// sie (nach der Verschiebung) ganz oder teilweise außerhalb des Labyrinths
// oder im Geisterhaus läge
static bool dotZellPosition(uint8_t dc, uint8_t dr, uint8_t &x, uint8_t &y)
{
    int16_t ix = (int16_t)MAZE_X0 + DOT_OFFSET_X + (int16_t)dc * DOT_PITCH;
    int16_t iy = (int16_t)DOT_OFFSET_Y + (int16_t)dr * DOT_PITCH;
    if (ix < MAZE_X0 || ix + DOT_SIZE - 1 > MAZE_X1) return false;
    if (iy < 0 || iy + DOT_SIZE - 1 >= MAZE_H) return false;

    // Punkte im Geisterhaus wären für Pacman unerreichbar (er passt nicht
    // durch die 7px-Tür) - sie würden das Level unlösbar machen
    if (ix + DOT_SIZE - 1 >= HAUS_X0 && ix <= HAUS_X1 &&
        iy + DOT_SIZE - 1 >= HAUS_Y0 && iy <= HAUS_Y1) return false;

    x = (uint8_t)ix;
    y = (uint8_t)iy;
    return true;
}

// Prüft, ob (px,py) innerhalb der 2x2-Fläche irgendeiner Punktzelle liegt,
// und liefert deren Index in dotEaten über idxOut
static bool dotZelleVon(uint8_t px, uint8_t py, uint16_t &idxOut)
{
    int16_t rx = (int16_t)px - MAZE_X0 - DOT_OFFSET_X;
    int16_t ry = (int16_t)py - DOT_OFFSET_Y;
    if (rx < 0 || rx % DOT_PITCH >= DOT_SIZE) return false;
    if (ry < 0 || ry % DOT_PITCH >= DOT_SIZE) return false;
    uint16_t dc = rx / DOT_PITCH;
    uint16_t dr = ry / DOT_PITCH;
    if (dc >= DOT_COLS || dr >= DOT_ROWS) return false;
    uint8_t zx, zy;
    if (!dotZellPosition((uint8_t)dc, (uint8_t)dr, zx, zy)) return false;
    idxOut = dr * DOT_COLS + dc;
    return true;
}

// Zeichnet einen Pixel innerhalb des Labyrinths gemäß aktuellem Zustand
// (Wand / Punkt / frei) - wird benutzt um die Spur hinter Pacman/Geist
// wiederherzustellen. x liegt hier immer in [MAZE_X0,MAZE_X1] (s.o.)
static bool istEnergizer(uint16_t idx)
{
    for (uint8_t e = 0; e < ENERGIZER_ANZAHL; e++)
        if (energizerIdx[e] == idx) return true;
    return false;
}

// Das Panel kann nur schwarz/grün/rot/gelb - ein echtes Orange gibt es nicht.
// Gelb (rot+grün) ist die nächstliegende Farbe, deshalb blinkt der Energizer
// zwischen Rot und Gelb.
static inline uint8_t energizerFarbe() { return blinkPhase ? YELLOW : RED; }

static void pixelNatuerlichZeichnen(uint8_t x, uint8_t y)
{
    // Hausinneres bleibt immer schwarz - sonst würden beim Löschen eines
    // Geist-Sprites die aufgemalten Deko-Geister wieder sichtbar
    if (imGeisterhaus(x, y)) { Graphics::drawPixel(x, y, BLACK); return; }

    if (istWand(x, y)) { Graphics::drawPixel(x, y, GREEN); return; }

    uint16_t idx;
    if (dotZelleVon(x, y, idx) && !bitLesen(dotEaten, idx))
    {
        Graphics::drawPixel(x, y, istEnergizer(idx) ? energizerFarbe() : RED);
        return;
    }

    Graphics::drawPixel(x, y, BLACK);
}

// Zeichnet die beiden Energizer neu (für das Blinken)
static void energizerZeichnen()
{
    for (uint8_t e = 0; e < ENERGIZER_ANZAHL; e++)
    {
        uint16_t idx = energizerIdx[e];
        if (idx == 0xFFFF || bitLesen(dotEaten, idx)) continue;

        uint8_t x0, y0;
        if (!dotZellPosition(idx % DOT_COLS, idx / DOT_COLS, x0, y0)) continue;

        for (uint8_t dx = 0; dx < DOT_SIZE; dx++)
            for (uint8_t dy = 0; dy < DOT_SIZE; dy++)
                Graphics::drawPixel(x0 + dx, y0 + dy, energizerFarbe());
    }
}

// Wählt eine zufällige gültige Punktzelle im angegebenen Quadranten.
// Zwei Durchläufe (zählen, dann die n-te heraussuchen) statt einer
// Kandidatenliste - spart RAM auf dem Mega.
static uint16_t waehleEnergizer(bool rechteHaelfte, bool untereHaelfte)
{
    uint16_t anzahl = 0;
    for (uint8_t dr = 0; dr < DOT_ROWS; dr++)
        for (uint8_t dc = 0; dc < DOT_COLS; dc++)
        {
            if ((dc >= DOT_COLS / 2) != rechteHaelfte) continue;
            if ((dr >= DOT_ROWS / 2) != untereHaelfte) continue;
            uint8_t x, y;
            if (dotZellPosition(dc, dr, x, y) && !istWand(x, y)) anzahl++;
        }

    if (anzahl == 0) return 0xFFFF;

    uint16_t ziel = (uint16_t)random(anzahl);
    uint16_t i = 0;
    for (uint8_t dr = 0; dr < DOT_ROWS; dr++)
        for (uint8_t dc = 0; dc < DOT_COLS; dc++)
        {
            if ((dc >= DOT_COLS / 2) != rechteHaelfte) continue;
            if ((dr >= DOT_ROWS / 2) != untereHaelfte) continue;
            uint8_t x, y;
            if (!dotZellPosition(dc, dr, x, y) || istWand(x, y)) continue;
            if (i == ziel) return (uint16_t)dr * DOT_COLS + dc;
            i++;
        }

    return 0xFFFF;
}

// px/py werden auf [MAZE_X0,MAZE_X1]x[0,MAZE_H-1] begrenzt: x wegen des
// Seitentunnels (Pacman/Geist können auf Spalte MAZE_X0/MAZE_X1 stehen, das
// Sprite würde sonst in die Deko-Bereiche links/rechts hineinragen), y weil
// (x,y) nah an Zeile 0/MAZE_H-1 stehen kann (direkt neben der Wand) und
// sonst außerhalb der Anzeige gezeichnet bzw. gelesen würde
static void spriteZeichnen(uint8_t x, uint8_t y, const uint8_t icon[SPRITE_SIZE][SPRITE_SIZE], int8_t xShift, int8_t yShift)
{
    for (int8_t dx = SPRITE_MIN; dx <= SPRITE_MAX; dx++)
    {
        int16_t px = (int16_t)x + dx + xShift;
        if (px < MAZE_X0 || px > MAZE_X1) continue;
        for (int8_t dy = SPRITE_MIN; dy <= SPRITE_MAX; dy++)
        {
            int16_t py = (int16_t)y + dy + yShift;
            if (py < 0 || py >= MAZE_H) continue;
            uint8_t farbe = pgm_read_byte(&icon[dy - SPRITE_MIN][dx - SPRITE_MIN]);
            Graphics::drawPixel((uint8_t)px, (uint8_t)py, farbe);
        }
    }
}

static void spriteLoeschen(uint8_t x, uint8_t y, int8_t xShift, int8_t yShift)
{
    for (int8_t dx = SPRITE_MIN; dx <= SPRITE_MAX; dx++)
    {
        int16_t px = (int16_t)x + dx + xShift;
        if (px < MAZE_X0 || px > MAZE_X1) continue;
        for (int8_t dy = SPRITE_MIN; dy <= SPRITE_MAX; dy++)
        {
            int16_t py = (int16_t)y + dy + yShift;
            if (py < 0 || py >= MAZE_H) continue;
            pixelNatuerlichZeichnen((uint8_t)px, (uint8_t)py);
        }
    }
}

// Übermalt die im Originalbild aufgemalten Deko-Geister im Haus, damit dort
// nur die echten Spielfiguren zu sehen sind
// --- Geschwindigkeiten in Prozent (siehe levelTabelle) ---

static uint8_t pacProzent()
{
    if (frightAktiv)
        return dotImLetztenTick ? lvl.pacFrightDotsPct : lvl.pacFrightPct;
    return dotImLetztenTick ? lvl.pacDotsPct : lvl.pacPct;
}

// Blinky wird schneller, je weniger Punkte übrig sind ("Cruise Elroy")
static bool elroyAktiv()
{
    return punkteUebrig <= lvl.elroy1Rest;
}

static uint8_t geistProzent(uint8_t idx)
{
    const Geist &g = geister[idx];

    if (g.zustand == AUGEN) return AUGEN_PCT;
    if (g.aengstlich) return lvl.geistFrightPct;

    // Im Tunnel kriechen die Geister - das ist Pacmans einzige Chance,
    // Abstand zu gewinnen
    if (g.y == TUNNEL_MID &&
        (g.x <= ANCHOR_X_MIN + TUNNEL_ZONE || g.x >= ANCHOR_X_MAX - TUNNEL_ZONE))
        return lvl.tunnelPct;

    if (idx == 0) // nur Blinky bekommt Elroy
    {
        if (punkteUebrig <= lvl.elroy2Rest) return lvl.elroy2Pct;
        if (punkteUebrig <= lvl.elroy1Rest) return lvl.elroy1Pct;
    }

    return lvl.geistPct;
}

// Ab wie vielen gegessenen Punkten dieser Geist das Haus verlässt
// (0 = steht von Anfang an draußen)
static uint16_t geistFreigabeDots(uint8_t idx)
{
    if (idx == 1) return PINKY_DOTS;
    if (idx == 2) return INKY_DOTS;
    return 0;
}

// true, wenn der wartende Geist kurz vor seiner Freigabe steht und deshalb
// als Vorwarnung blinken soll
static bool spawnWarnungAktiv(uint8_t idx)
{
    uint16_t frei = geistFreigabeDots(idx);
    if (frei < SPAWN_WARNUNG_DOTS) return false;
    return dotsGegessen + SPAWN_WARNUNG_DOTS >= frei;
}

// Blinktakt der ängstlichen Geister. Läuft der Energizer bald ab, wird doppelt
// so schnell geblinkt - das ist die Entsprechung zu den "Flashes" im Original
// und warnt davor, dass die Geister gleich wieder gefährlich sind.
static bool geistBlinkPhase()
{
    if (!frightAktiv) return blinkPhase;

    unsigned long verstrichen = millis() - frightStart;
    if (verstrichen < lvl.frightMs && (lvl.frightMs - verstrichen) <= FRIGHT_WARN_MS)
        return (millis() / (BLINK_MS / 2)) & 1;

    return blinkPhase;
}

// Alle Figuren vom Feld nehmen (Hintergrund wiederherstellen)
static void spritesLoeschen()
{
    for (uint8_t i = 0; i < GEIST_ANZAHL; i++)
        spriteLoeschen(geister[i].x, geister[i].y, GEIST_X_SHIFT, GEIST_Y_SHIFT);
    spriteLoeschen(pacmanX, pacmanY, PACMAN_X_SHIFT, PACMAN_Y_SHIFT);
}

// Alle Figuren in fester Z-Reihenfolge zeichnen: erst die Geister, ZULETZT
// Pacman. Damit liegt der Spieler immer obenauf. Vorher zeichnete jede
// Bewegungsfunktion nur ihre eigene Figur, sodass ein Geist (bzw. dessen
// Löschen) den Spieler zerlegen konnte, wenn beide sich überlappten - z.B.
// beim Drüberlaufen über die zurückkehrenden Augen, die ja keine Hitbox haben.
static void spritesZeichnen()
{
    for (uint8_t i = 0; i < GEIST_ANZAHL; i++)
    {
        // Kurz vor der Freigabe blinkt der wartende Geist: in der einen
        // Blinkphase wird er einfach nicht gezeichnet (Hintergrund ist durch
        // spritesLoeschen() schon wiederhergestellt) -> Bild/dunkel im Wechsel
        if (geister[i].zustand == IM_HAUS && blinkPhase && spawnWarnungAktiv(i)) continue;

        const uint8_t (*icon)[SPRITE_SIZE];
        if (geister[i].zustand == AUGEN) icon = augenIcon; // nur die Augenbalken
        else if (geister[i].aengstlich && geistBlinkPhase()) icon = geistIconGruen; // Fright: rot/grün
        else icon = geistIcon;

        spriteZeichnen(geister[i].x, geister[i].y, icon, GEIST_X_SHIFT, GEIST_Y_SHIFT);
    }

    spriteZeichnen(pacmanX, pacmanY, pacmanIconAktuell(), PACMAN_X_SHIFT, PACMAN_Y_SHIFT);
}

static void geisterhausLeeren()
{
    for (uint8_t x = HAUS_X0; x <= HAUS_X1; x++)
        for (uint8_t y = HAUS_Y0; y <= HAUS_Y1; y++)
            Graphics::drawPixel(x, y, BLACK);
}

// Übermalt die vier losen gelben Pixel unten links im Originalbild
static void bildArtefakteEntfernen()
{
    for (uint8_t x = ARTEFAKT_X0; x <= ARTEFAKT_X1; x++)
        for (uint8_t y = ARTEFAKT_Y0; y <= ARTEFAKT_Y1; y++)
            Graphics::drawPixel(x, y, BLACK);
}

static void alleDotsZeichnen()
{
    for (uint8_t dr = 0; dr < DOT_ROWS; dr++)
    {
        for (uint8_t dc = 0; dc < DOT_COLS; dc++)
        {
            if (bitLesen(dotEaten, (uint16_t)dr * DOT_COLS + dc)) continue;
            uint8_t x0, y0;
            if (!dotZellPosition(dc, dr, x0, y0)) continue;
            for (uint8_t dx = 0; dx < DOT_SIZE; dx++)
                for (uint8_t dy = 0; dy < DOT_SIZE; dy++)
                    if (!istWand(x0 + dx, y0 + dy)) Graphics::drawPixel(x0 + dx, y0 + dy, RED);
        }
    }
}

// Zeichnet eine Zahl rechtsbündig mit führenden Nullen in das Ziffernraster
// des HUD (wie die Platzhalter im Originalbild)
static void zeichneZahl(uint8_t y, uint32_t wert)
{
    for (int8_t i = HUD_DIGITS - 1; i >= 0; i--)
    {
        uint8_t ziffer = wert % 10;
        wert /= 10;
        uint8_t x0 = HUD_DIGIT_X0 + (uint8_t)i * HUD_DIGIT_PITCH;

        for (uint8_t row = 0; row < HUD_DIGIT_H; row++)
        {
            uint8_t muster = pgm_read_byte(&ziffernFont[ziffer][row]);
            for (uint8_t col = 0; col < HUD_DIGIT_W; col++)
            {
                bool an = (muster >> (HUD_DIGIT_W - 1 - col)) & 1;
                Graphics::drawPixel(x0 + col, y + row, an ? RED : BLACK);
            }
        }
    }
}

static void punkteZeichnen()
{
    zeichneZahl(HIGHSCORE_Y, highscore);
    zeichneZahl(SCORE_Y, punkte);
}

// Holt die 5 Zeilen eines Zeichens (A-Z und 0-9), false z.B. beim Leerzeichen
static bool zeichenHolen(char c, uint8_t *zeilen)
{
    if (c >= '0' && c <= '9')
    {
        for (uint8_t i = 0; i < TEXT_H; i++) zeilen[i] = pgm_read_byte(&ziffernFont[c - '0'][i]);
        return true;
    }
    if (c >= 'A' && c <= 'Z')
    {
        for (uint8_t i = 0; i < TEXT_H; i++) zeilen[i] = pgm_read_byte(&buchstabenFont[c - 'A'][i]);
        return true;
    }
    return false;
}

static void zeichneText(uint8_t x, uint8_t y, const char *text, uint8_t farbe)
{
    for (const char *p = text; *p; p++, x += TEXT_PITCH)
    {
        uint8_t zeilen[TEXT_H];
        if (!zeichenHolen(*p, zeilen)) continue; // Leerzeichen: nur weiterrücken

        for (uint8_t row = 0; row < TEXT_H; row++)
            for (uint8_t col = 0; col < TEXT_W; col++)
                if ((zeilen[row] >> (TEXT_W - 1 - col)) & 1)
                    Graphics::drawPixel(x + col, y + row, farbe);
    }
}

static void zeichneTextZentriert(uint8_t y, const char *text, uint8_t farbe)
{
    uint8_t n = 0;
    while (text[n]) n++;
    uint8_t breite = n * TEXT_PITCH - 1;
    uint8_t x = (breite < WIDTH) ? (WIDTH - breite) / 2 : 0;
    zeichneText(x, y, text, farbe);
}

// Zeichnet ein 6x6-Sprite vergrößert (jeder Pixel wird zu einem scale*scale
// großen Block) - für den Pacman auf dem Startbildschirm
static void zeichneGrossesIcon(uint8_t x0, uint8_t y0,
                               const uint8_t icon[SPRITE_SIZE][SPRITE_SIZE], uint8_t scale)
{
    for (uint8_t sy = 0; sy < SPRITE_SIZE; sy++)
        for (uint8_t sx = 0; sx < SPRITE_SIZE; sx++)
        {
            uint8_t farbe = pgm_read_byte(&icon[sy][sx]);
            for (uint8_t dy = 0; dy < scale; dy++)
                for (uint8_t dx = 0; dx < scale; dx++)
                    Graphics::drawPixel(x0 + sx * scale + dx, y0 + sy * scale + dy, farbe);
        }
}

static void startbildschirmZeichnen()
{
    Graphics::clear();
    uint8_t breite = SPRITE_SIZE * START_SCALE;
    zeichneGrossesIcon((WIDTH - breite) / 2, 6, pacmanIconRechts, START_SCALE);
    zeichneTextZentriert(45, "PRESS BUTTON TO START", YELLOW);
}

static void gameoverBildschirmZeichnen()
{
    Graphics::clear();
    zeichneTextZentriert(20, "GAME OVER", RED);
}

// Leerer Bildschirm mit grüner Meldung, mittig ausgerichtet
static void levelTextZeichnen()
{
    char puffer[24];
    uint8_t n = 0;
    for (const char *p = "LEVEL "; *p; p++) puffer[n++] = *p;

    char ziffern[5];
    uint8_t zn = 0;
    uint16_t v = level;
    do { ziffern[zn++] = '0' + (v % 10); v /= 10; } while (v && zn < sizeof(ziffern));
    while (zn) puffer[n++] = ziffern[--zn];

    for (const char *p = " REACHED"; *p; p++) puffer[n++] = *p;
    puffer[n] = '\0';

    uint8_t breite = n * TEXT_PITCH - 1;
    uint8_t x = (breite < WIDTH) ? (WIDTH - breite) / 2 : 0;

    Graphics::clear();
    zeichneText(x, (HEIGHT_ALL - TEXT_H) / 2, puffer, GREEN);
}

// Verbrauchte Leben werden schwarz übermalt; das Symbol ist dasselbe wie im
// Originalbild (Pacman nach rechts)
static void lebenZeichnen()
{
    for (uint8_t i = 0; i < LEBEN_MAX; i++)
    {
        bool sichtbar = (i < leben);
        for (uint8_t dx = 0; dx < SPRITE_SIZE; dx++)
            for (uint8_t dy = 0; dy < SPRITE_SIZE; dy++)
            {
                uint8_t farbe = sichtbar ? pgm_read_byte(&pacmanIconRechts[dy][dx]) : BLACK;
                Graphics::drawPixel(LEBEN_ICON_X + dx, lebenIconY[i] + dy, farbe);
            }
    }
}

// Sucht die nächste Position ab (startX,startY), an der die komplette 8x8-Box
// frei liegt, zeilenweise. Damit müssen Start-/Geisterpositionen nicht von
// Hand aus den Rohdaten abgelesen werden - robust gegenüber künftigen
// Änderungen an bild.png. startX/startY werden auf den gültigen Anker-Bereich
// begrenzt, damit boxFrei() nie mit einer von vornherein unmöglichen Position
// aufgerufen wird.
static void findeOffenenPixel(uint8_t startX, uint8_t startY, uint8_t &outX, uint8_t &outY)
{
    if (startX < ANCHOR_X_MIN) startX = ANCHOR_X_MIN;
    if (startY < ANCHOR_Y_MIN) startY = ANCHOR_Y_MIN;

    for (uint8_t y = startY; y <= ANCHOR_Y_MAX; y++)
    {
        uint8_t x0 = (y == startY) ? startX : ANCHOR_X_MIN;
        for (uint16_t x = x0; x <= ANCHOR_X_MAX; x++)
        {
            if (boxFrei((uint8_t)x, y)) { outX = (uint8_t)x; outY = y; return; }
        }
    }
    outX = ANCHOR_X_MIN;
    outY = ANCHOR_Y_MIN;
}

// Zielposition für eine Richtung, true falls die komplette 8x8-Box dort frei
// liegt. Am linken/rechten Anker-Rand wird nur in TUNNEL_MID (der einzigen
// Zeile, in der die Box exakt in die Tunnelöffnung passt) auf die jeweils
// andere Seite gewrappt (Seitentunnel wie im Original-Pacman), sonst
// blockiert.
static bool naechstePixel(uint8_t x, uint8_t y, Richtung r, uint8_t &nx, uint8_t &ny)
{
    nx = x; ny = y;
    switch (r)
    {
        case OBEN: ny = y - 1; break;
        case UNTEN: ny = y + 1; break;
        case LINKS:
            if (x <= ANCHOR_X_MIN)
            {
                if (y != TUNNEL_MID) return false;
                nx = ANCHOR_X_MAX;
                return boxFrei(nx, ny);
            }
            nx = x - 1;
            break;
        case RECHTS:
            if (x >= ANCHOR_X_MAX)
            {
                if (y != TUNNEL_MID) return false;
                nx = ANCHOR_X_MIN;
                return boxFrei(nx, ny);
            }
            nx = x + 1;
            break;
        default:
            return false;
    }
    return boxFrei(nx, ny);
}

static Richtung gegenrichtung(Richtung r)
{
    switch (r)
    {
        case OBEN: return UNTEN;
        case UNTEN: return OBEN;
        case LINKS: return RECHTS;
        case RECHTS: return LINKS;
        default: return KEINE;
    }
}

// Zwei 8x8-Hitboxen (je zentriert auf x1,y1 bzw. x2,y2) überlappen genau
// dann, wenn ihr Mittelpunktabstand in beiden Achsen < HITBOX_SIZE ist
static bool nah(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    int16_t dx = (int16_t)x1 - (int16_t)x2;
    int16_t dy = (int16_t)y1 - (int16_t)y2;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx < HITBOX_SIZE && dy < HITBOX_SIZE;
}

// Alle Punkte gefressen -> nächstes Level. Punkte, Leben und Highscore
// bleiben erhalten, nur das Labyrinth wird neu befüllt.
static void spielGewonnen()
{
    level++;
    spielStatus = LEVEL_GESCHAFFT;
    todZeit = millis();
    levelTextZeichnen();

    //Serial.print(F("Level geschafft! Jetzt Level "));
    //Serial.println(level);
}

// Ein Leben geht verloren. Sind noch welche übrig, startet nur die Runde neu
// (gegessene Punkte und Punktestand bleiben erhalten). Beim letzten Leben ist
// Schluss: dann setzt neuesSpiel() den Punktestand zurück - der Highscore
// bleibt aber stehen.
// Startet nur die Sterbe-Sequenz. Das Leben wird erst am Ende abgezogen
// (siehe Game::update), damit man vorher noch sieht, was passiert ist.
static void spielerStirbt()
{
    spielStatus = TOD_FREEZE;
    todZeit = millis();
    todFrame = 0;
    //Serial.println(F("Erwischt!"));
}

static void bewegePacman()
{
    uint8_t nx, ny;
    Richtung effektiv;

    if (naechstePixel(pacmanX, pacmanY, gewuenschteRichtung, nx, ny))
        effektiv = gewuenschteRichtung;
    else if (naechstePixel(pacmanX, pacmanY, pacmanRichtung, nx, ny))
        effektiv = pacmanRichtung;
    else
        return; // blockiert, Pacman bleibt stehen

    pacmanRichtung = effektiv;
    mundZaehler++;
    pacmanX = nx;
    pacmanY = ny;

    // Punkte werden mit der kompletten 8x8-Hitbox eingesammelt, nicht nur an
    // (pacmanX,pacmanY) selbst - dieselbe Zelle kann dabei mehrfach über
    // verschiedene Pixel der Box gefunden werden, bitLesen verhindert Doppelzählung
    for (int8_t dx = HITBOX_MIN; dx <= HITBOX_MAX; dx++)
    {
        for (int8_t dy = HITBOX_MIN; dy <= HITBOX_MAX; dy++)
        {
            uint8_t px = pacmanX + dx;
            uint8_t py = pacmanY + dy;
            uint16_t idx;
            if (!dotZelleVon(px, py, idx) || bitLesen(dotEaten, idx)) continue;

            bitSetzen(dotEaten, idx);
            punkteUebrig--;
            dotsGegessen++;
            dotImLetztenTick = true; // bremst Pacman im nächsten Takt
            letzterDotZeit = millis();

            if (istEnergizer(idx))
            {
                punkte += PUNKTE_ENERGIZER;
                energizerGefressen();
            }
            else
            {
                punkte += PUNKTE_PRO_DOT;
            }

            if (punkte > highscore) highscore = punkte;
            punkteZeichnen();

            // Extraleben, je einmal pro Spiel
            if (bonusStufe < 1 && punkte >= BONUS_LEBEN_1)
            {
                bonusStufe = 1;
                if (leben < LEBEN_MAX) { leben++; lebenZeichnen(); }
                //Serial.println(F("Extraleben (1000)!"));
            }
            if (bonusStufe < 2 && punkte >= BONUS_LEBEN_2)
            {
                bonusStufe = 2;
                if (leben < LEBEN_MAX) { leben++; lebenZeichnen(); }
                //Serial.println(F("Extraleben (3000)!"));
            }

            // Punkt-Zelle explizit neu zeichnen: das Pacman-Sprite ist wegen
            // PACMAN_Y_SHIFT nicht deckungsgleich mit der Hitbox, würde den
            // gegessenen Punkt also nicht zuverlässig überdecken
            uint8_t zx, zy;
            if (dotZellPosition(idx % DOT_COLS, idx / DOT_COLS, zx, zy))
                for (uint8_t ddx = 0; ddx < DOT_SIZE; ddx++)
                    for (uint8_t ddy = 0; ddy < DOT_SIZE; ddy++)
                        pixelNatuerlichZeichnen(zx + ddx, zy + ddy);
        }
    }

}

// Ängstliche Geister fressen, sonst sterben. Augen haben keine Hitbox.
// Wird einmal pro Basistakt aufgerufen, nachdem sich alles bewegt hat.
static void kollisionPruefen()
{
    for (uint8_t i = 0; i < GEIST_ANZAHL; i++)
    {
        Geist &g = geister[i];
        if (g.zustand == IM_HAUS || g.zustand == AUGEN || g.zustand == ERHOLT_SICH) continue;
        if (!nah(pacmanX, pacmanY, g.x, g.y)) continue;

        if (g.aengstlich) { geistGefressen(i); continue; }
        spielerStirbt();
        return;
    }
    if (punkteUebrig == 0) spielGewonnen();
}

// Scatter/Chase folgt jetzt der Phasenliste des Originals statt einem simplen
// Hin und Her. Jeder Phasenwechsel erzwingt eine Richtungsumkehr - das ist
// zugleich das, was Endlosschleifen der Geister aufbricht.
static void geistModusPruefen(unsigned long jetzt)
{
    if (phasenIdx >= PHASEN_MAX) return; // Chase für immer

    uint8_t stufe = levelStufe();
    if (stufe > 2) stufe = 2; // ab Level 21 gilt die 5+-Zeile
    uint8_t dauerS = pgm_read_byte(&phasenTabelle[stufe][phasenIdx]);

    if (dauerS != 0 && jetzt - phasenStart < (unsigned long)dauerS * 1000) return;

    if (dauerS == 0) phasenIdx = PHASEN_MAX; // Liste zu Ende -> Chase forever
    else phasenIdx++;

    phasenStart = jetzt;
    geistScatter = (phasenIdx < PHASEN_MAX) && ((phasenIdx & 1) == 0);

    for (uint8_t i = 0; i < GEIST_ANZAHL; i++)
    {
        if (geister[i].zustand != DRAUSSEN) continue;
        uint8_t nx, ny;
        Richtung rueck = gegenrichtung(geister[i].richtung);
        if (naechstePixel(geister[i].x, geister[i].y, rueck, nx, ny)) geister[i].richtung = rueck;
    }
}

// Energizer gefressen: alle Geister draußen kehren um und werden ängstlich.
// Die Fresskette startet neu (200/400/800/1600 innerhalb dieses Energizers).
static void energizerGefressen()
{
    frightAktiv = true;
    frightStart = millis();
    geisterKette = 0;

    for (uint8_t i = 0; i < GEIST_ANZAHL; i++)
    {
        if (geister[i].zustand != DRAUSSEN) continue;
        geister[i].aengstlich = true;

        uint8_t nx, ny;
        Richtung rueck = gegenrichtung(geister[i].richtung);
        if (naechstePixel(geister[i].x, geister[i].y, rueck, nx, ny)) geister[i].richtung = rueck;
    }

    //Serial.println(F("Energizer!"));
}

// Fright-Effekt abgelaufen: Geister wieder normal, Multiplikator zurück
static void frightBeenden()
{
    frightAktiv = false;
    geisterKette = 0;
    for (uint8_t i = 0; i < GEIST_ANZAHL; i++) geister[i].aengstlich = false;
}

static void richtungsVektor(Richtung r, int8_t &dx, int8_t &dy)
{
    dx = 0; dy = 0;
    switch (r)
    {
        case OBEN: dy = -1; break;
        case UNTEN: dy = 1; break;
        case LINKS: dx = -1; break;
        case RECHTS: dx = 1; break;
        default: break;
    }
}

// Zielpunkt je Geist - int16_t, weil die Ziele bewusst weit außerhalb des
// Spielfelds liegen dürfen (das ist im Original genau so und erzeugt die
// unterschiedlichen Verfolgungsmuster)
static void geistZiel(uint8_t idx, int16_t &zx, int16_t &zy)
{
    // Elroy-Blinky ignoriert Scatter und jagt durchgehend (wie im Original)
    if (geistScatter && !(idx == 0 && elroyAktiv()))
    {
        switch (idx)
        {
            case 0: zx = ANCHOR_X_MAX; zy = ANCHOR_Y_MIN; break; // Blinky: oben rechts
            case 1: zx = ANCHOR_X_MIN; zy = ANCHOR_Y_MIN; break; // Pinky: oben links
            default: zx = ANCHOR_X_MAX; zy = ANCHOR_Y_MAX; break; // Inky: unten rechts
        }
        return;
    }

    if (idx == 0) { zx = pacmanX; zy = pacmanY; return; } // Blinky: direkt auf Pacman

    int8_t rdx, rdy;
    richtungsVektor(pacmanRichtung, rdx, rdy);

    if (idx == 1) // Pinky: 4 Kacheln vor Pacman
    {
        zx = (int16_t)pacmanX + (int16_t)rdx * 4 * KACHEL;
        zy = (int16_t)pacmanY + (int16_t)rdy * 4 * KACHEL;
        return;
    }

    // Inky: Vektor von Blinky zu "2 Kacheln vor Pacman", verdoppelt
    int16_t px = (int16_t)pacmanX + (int16_t)rdx * 2 * KACHEL;
    int16_t py = (int16_t)pacmanY + (int16_t)rdy * 2 * KACHEL;
    zx = 2 * px - (int16_t)geister[0].x;
    zy = 2 * py - (int16_t)geister[0].y;
}

// Fester Ausstiegspfad aus dem Haus: erst waagrecht unter die Tür, dann
// senkrecht hinaus. Ignoriert bewusst die Wandprüfung (siehe HAUS_* oben).
static void bewegeGeistImHaus(uint8_t idx)
{
    Geist &g = geister[idx];

    if (g.x != HAUS_GATE_X)
    {
        g.richtung = (g.x < HAUS_GATE_X) ? RECHTS : LINKS;
        g.x += (g.x < HAUS_GATE_X) ? 1 : -1;
        return;
    }

    if (g.y > HAUS_EXIT_Y)
    {
        g.richtung = OBEN;
        g.y--;
        return;
    }

    g.zustand = DRAUSSEN; // oben angekommen -> ab jetzt normale KI
    g.richtung = LINKS;
}

// Geist-KI nach dem Vorbild des Original-Pacman (statt der alten Luftlinien-
// Verfolgung, die sich an Wänden festgefahren hat):
//  - der Geist kehrt nie um (Ausnahmen: Sackgasse, Moduswechsel)
//  - an jeder Kreuzung wählt er die Richtung, deren Zielfeld dem Zielpunkt
//    in Luftlinie am nächsten liegt; bei Gleichstand gilt die Reihenfolge
//    oben, links, unten, rechts
//  - im Scatter-Modus ist das Ziel die Ecke oben rechts statt Pacman
// Dadurch folgt er den Gängen, statt gegen Wände zu drücken.
static void bewegeGeistDraussen(uint8_t idx)
{
    Geist &g = geister[idx];

    static const Richtung reihenfolge[4] = { OBEN, LINKS, UNTEN, RECHTS };
    Richtung rueck = gegenrichtung(g.richtung);

    // Ängstliche Geister verfolgen nicht, sondern würfeln an jeder Kreuzung
    // eine der erlaubten Richtungen aus (Umkehren bleibt verboten)
    if (g.aengstlich)
    {
        Richtung moeglich[4];
        uint8_t n = 0;
        uint8_t zielX[4], zielY[4];

        for (uint8_t i = 0; i < 4; i++)
        {
            Richtung r = reihenfolge[i];
            if (r == rueck) continue;
            uint8_t nx, ny;
            if (!naechstePixel(g.x, g.y, r, nx, ny)) continue;
            moeglich[n] = r; zielX[n] = nx; zielY[n] = ny; n++;
        }

        if (n == 0)
        {
            uint8_t nx, ny;
            if (!naechstePixel(g.x, g.y, rueck, nx, ny)) return;
            g.richtung = rueck; g.x = nx; g.y = ny;
            return;
        }

        uint8_t w = (uint8_t)random(n);
        g.richtung = moeglich[w];
        g.x = zielX[w];
        g.y = zielY[w];
        return;
    }

    int16_t zielX, zielY;
    geistZiel(idx, zielX, zielY);
    geistSchrittZuZiel(g, zielX, zielY);
}

// Ein Schritt der Original-Wegewahl in Richtung (zielX,zielY). Ausgelagert,
// weil sowohl die jagenden Geister als auch die zurückkehrenden Augen sie
// benutzen - nur mit unterschiedlichem Ziel.
static void geistSchrittZuZiel(Geist &g, int16_t zielX, int16_t zielY)
{
    static const Richtung reihenfolge[4] = { OBEN, LINKS, UNTEN, RECHTS };
    Richtung rueck = gegenrichtung(g.richtung);

    Richtung besteRichtung = KEINE;
    uint8_t besteX = g.x, besteY = g.y;
    int32_t besteDistanz = 0;

    for (uint8_t i = 0; i < 4; i++)
    {
        Richtung r = reihenfolge[i];
        if (r == rueck) continue; // Umkehren ist verboten

        uint8_t nx, ny;
        if (!naechstePixel(g.x, g.y, r, nx, ny)) continue;

        int32_t ddx = (int32_t)nx - (int32_t)zielX;
        int32_t ddy = (int32_t)ny - (int32_t)zielY;
        int32_t dist = ddx * ddx + ddy * ddy;

        // strikt kleiner -> bei Gleichstand gewinnt die frühere Richtung
        // aus reihenfolge[], also der Original-Tie-Break
        if (besteRichtung == KEINE || dist < besteDistanz)
        {
            besteRichtung = r;
            besteDistanz = dist;
            besteX = nx;
            besteY = ny;
        }
    }

    if (besteRichtung == KEINE)
    {
        // Sackgasse: hier ist Umkehren die einzige Möglichkeit
        uint8_t nx, ny;
        if (!naechstePixel(g.x, g.y, rueck, nx, ny)) return; // komplett eingeschlossen
        besteRichtung = rueck;
        besteX = nx;
        besteY = ny;
    }

    g.richtung = besteRichtung;
    g.x = besteX;
    g.y = besteY;
}

// Gefressener Geist als Augenpaar: läuft mit der normalen Wegewahl zum
// Hauseingang und steigt dort senkrecht ins Haus hinab (dabei werden die
// Wände ignoriert, genau wie beim Herauslaufen - die Tür ist zu schmal für
// die 8x8-Box). Per Skript geprüft: von jeder Position und Richtung aus wird
// der Eingang erreicht (längster Weg 175 Schritte).
static void bewegeAugen(uint8_t idx)
{
    Geist &g = geister[idx];

    // Phase 2: über der Tür -> senkrecht ins Haus. Nur zwischen Ausgang und
    // Wartehöhe - vorher reichte g.y >= HAUS_EXIT_Y, dadurch galten auch
    // Augen, die im Gang UNTER dem Haus an x=HAUS_GATE_X vorbeiliefen, als
    // "angekommen" und liefen von unten durch die Wände ins Haus.
    if (g.x == HAUS_GATE_X && g.y >= HAUS_EXIT_Y && g.y <= HAUS_WARTE_Y)
    {
        if (g.y < HAUS_WARTE_Y)
        {
            g.richtung = UNTEN;
            g.y++;
            return;
        }
        // in der Mitte angekommen -> Geist ist wieder komplett, wartet kurz
        g.zustand = ERHOLT_SICH;
        g.richtung = OBEN;
        g.wartStart = millis();
        return;
    }

    // Phase 1: durch das Labyrinth zurück zum Hauseingang
    geistSchrittZuZiel(g, HAUS_GATE_X, HAUS_EXIT_Y);
}

// Ängstlichen Geist gefressen: 200/400/800/1600 je nachdem, der wievielte er
// innerhalb dieses Energizers ist. Danach bleibt nur das Augenpaar übrig, das
// zum Haus zurückwandert.
static void geistGefressen(uint8_t idx)
{
    Geist &g = geister[idx];

    uint16_t wert = PUNKTE_GEIST_BASIS;
    for (uint8_t i = 0; i < geisterKette; i++) wert *= 2;
    if (geisterKette < GEIST_KETTE_MAX - 1) geisterKette++;

    punkte += wert;
    if (punkte > highscore) highscore = punkte;
    punkteZeichnen();

    // Körper weg, nur die Augen bleiben und laufen von hier aus zurück.
    // Gezeichnet wird zentral am Ende des Basistakts.
    spritesLoeschen();
    g.aengstlich = false;
    g.zustand = AUGEN;
    spritesZeichnen();
    (void)g;

    //Serial.print(F("Geist gefressen: +"));
    //Serial.println(wert);
}

// Ein einzelner Schritt eines Geistes (ohne Zeichnen/Kollision - das macht
// der Basistakt in Game::update für alle Figuren gemeinsam)
static void geistSchritt(uint8_t idx)
{
    Geist &g = geister[idx];
    if (g.zustand == IM_HAUS) return; // wartet noch auf seine Freigabe

    // Nach dem Gefressenwerden: steht HAUS_ERHOLUNG_MS in der Hausmitte,
    // dann geht es wie beim ersten Verlassen wieder hinaus
    if (g.zustand == ERHOLT_SICH)
    {
        if (millis() - g.wartStart >= HAUS_ERHOLUNG_MS) g.zustand = VERLAESST_HAUS;
        return;
    }

    if (g.zustand == AUGEN) bewegeAugen(idx);
    else if (g.zustand == VERLAESST_HAUS) bewegeGeistImHaus(idx);
    else bewegeGeistDraussen(idx);
}

static void tasteVerarbeiten(char c)
{
    if (c < 32) return; // Steuerzeichen wie Zeilenumbruch ignorieren

    // Start- und Game-Over-Bildschirm reagieren auf jede Taste
    if (spielStatus == STARTBILDSCHIRM) { spielStarten(); return; }

    if (spielStatus == GAMEOVER)
    {
        // Eingabe wird erst nach einer Sekunde angenommen, damit ein
        // Tastendruck aus dem laufenden Spiel nicht sofort neu startet
        if (millis() - todZeit >= GAMEOVER_EINGABE_MS) spielStarten();
        return;
    }

    switch (c)
    {
        case 'w': case 'W': gewuenschteRichtung = OBEN; break;
        case 's': case 'S': gewuenschteRichtung = UNTEN; break;
        case 'a': case 'A': gewuenschteRichtung = LINKS; break;
        case 'd': case 'D': gewuenschteRichtung = RECHTS; break;
        default: break;
    }
}

static void controllerVerarbeiten()
{
    // key_pressed gilt nur für diese Abfrage. War es static und wurde nie
    // zurückgesetzt, blieb es nach dem ersten Tastendruck für immer true -
    // der Game-Over-Bildschirm startete dann nach einer Sekunde von selbst neu.
    static bool warGedrueckt = false; // Stand der vorherigen Abfrage
    bool key_pressed = false;
    snespad.poll();

    // D-Pad (Steuerkreuz): ein Bit pro Richtung, Bitnummer = Richtung-Enum
    static uint8_t vorherRichtungen = 0;
    uint8_t richtungen = 0;
    if (snespad.directionUp)    richtungen |= 1 << OBEN;
    if (snespad.directionDown)  richtungen |= 1 << UNTEN;
    if (snespad.directionLeft)  richtungen |= 1 << LINKS;
    if (snespad.directionRight) richtungen |= 1 << RECHTS;

    // Beim Abrollen über das Kreuz sind kurz zwei Richtungen gleichzeitig
    // gedrückt (diagonal). Vorher gewann dann immer oben > unten > links >
    // rechts, egal was man eigentlich wollte. Jetzt gewinnt die zuletzt dazu
    // gedrückte Richtung; ohne neue bleibt die aktuelle, solange sie gehalten wird.
    uint8_t neu = richtungen & ~vorherRichtungen;
    vorherRichtungen = richtungen;
    uint8_t auswahl = neu ? neu : richtungen;
    if (auswahl) {
        key_pressed = true;
        if (neu || !(richtungen & (1 << gewuenschteRichtung))) {
            for (uint8_t r = OBEN; r <= RECHTS; r++) {
                if (auswahl & (1 << r)) { gewuenschteRichtung = (Richtung)r; break; }
            }
        }
    }

    // Aktionstasten
    if (snespad.buttonA) {
        Serial.println("Button A is pressed.");
        key_pressed = true;
    }
    else if (snespad.buttonB) {
        //Serial.println("Button B is pressed.");
        key_pressed = true;
    }
    else if (snespad.buttonX) {
        //Serial.println("Button X is pressed.");
        key_pressed = true;
    }
    else if (snespad.buttonY) {
        //Serial.println("Button Y is pressed.");
        key_pressed = true;
    }

    // Schultertasten
    else if (snespad.buttonL) {
        //Serial.println("Button L is pressed.");
        key_pressed = true;
    }
    else if (snespad.buttonR) {
        //Serial.println("Button R is pressed.");
        key_pressed = true;
    }

    // Start/Select
    else if (snespad.buttonStart) {
        //Serial.println("Button Start is pressed.");
        key_pressed = true;
    }
    else if (snespad.buttonSelect) {
        //Serial.println("Button Select is pressed.");
        key_pressed = true;
    }


    // Nur ein neuer Tastendruck zählt - eine aus dem Spiel heraus gehaltene
    // Taste (z.B. das Steuerkreuz beim Sterben) startet nicht von selbst neu
    bool neuGedrueckt = key_pressed && !warGedrueckt;
    warGedrueckt = key_pressed;

    // Start- und Game-Over-Bildschirm reagieren auf jede Taste
    if (spielStatus == STARTBILDSCHIRM && neuGedrueckt) { spielStarten(); return; }

    if (spielStatus == GAMEOVER && neuGedrueckt)
    {
        // Eingabe wird erst nach einer Sekunde angenommen, damit ein
        // Tastendruck aus dem laufenden Spiel nicht sofort neu startet
        if (millis() - todZeit >= GAMEOVER_EINGABE_MS) spielStarten();
        return;
    }

}

static void controllerInit()
{
    snespad.begin(); // init lib and gpio
    snespad.start(); // detect controller
}

static void eingabeLesen()
{
    if(EINGABEMODUS == TASTATUR){
        while (Serial.available() > 0)
            tasteVerarbeiten((char)Serial.read());
    }
    if(EINGABEMODUS == CONTROLLER){
        // Der Controller wird bewusst nicht in jeder loop()-Iteration
        // abgefragt: ein snespad.poll() schiebt das Bitprotokoll mit festen
        // Wartezeiten heraus und dauert dadurch gut 0,6 ms - gegenueber den
        // rund 3 ms eines Display::refresh() ist das ein Fuenftel der Zeit,
        // die dann nicht in die Matrix geht.
        // Die gelesene Richtung wird ohnehin erst im Basistakt ausgewertet
        // (BASIS_TICK_MS = 40 ms); CONTROLLER_POLL_MS liegt weit darunter,
        // an Reaktion und Spielgefuehl aendert sich also nichts.
        static unsigned long letzterPoll = 0;
        unsigned long jetzt = millis();
        if (jetzt - letzterPoll >= CONTROLLER_POLL_MS)
        {
            letzterPoll = jetzt;
            controllerVerarbeiten();
        }
    }
}

// Setzt nur die Figuren zurück (nach einem verlorenen Leben) - gegessene
// Punkte, Punktestand und Leben bleiben erhalten
static void rundeZuruecksetzen()
{
    findeOffenenPixel(MAZE_X0 + 1, 1, pacmanX, pacmanY);
    pacmanRichtung = RECHTS;
    gewuenschteRichtung = RECHTS;

    // Blinky steht von Anfang an direkt über der Tür des Geisterhauses,
    // Pinky und Inky warten drinnen auf ihre Freigabe (s. PINKY_DOTS/INKY_DOTS)
    geister[0].x = HAUS_GATE_X;  geister[0].y = HAUS_EXIT_Y;
    geister[0].richtung = LINKS; geister[0].zustand = DRAUSSEN;

    geister[1].x = HAUS_PINKY_X; geister[1].y = HAUS_WARTE_Y;
    geister[1].richtung = OBEN;  geister[1].zustand = IM_HAUS;

    geister[2].x = HAUS_INKY_X;  geister[2].y = HAUS_WARTE_Y;
    geister[2].richtung = OBEN;  geister[2].zustand = IM_HAUS;

    // Der Energizer-Effekt endet immer mit der Runde. Ohne das hier blieben
    // Geister nach einem Tod bzw. Levelwechsel dauerhaft ängstlich: das
    // aengstlich-Flag wird sonst nur von frightBeenden() gelöscht, und das
    // läuft nur, solange frightAktiv gesetzt ist. Wurde frightAktiv anderswo
    // (Levelstart, Game Over) direkt zurückgesetzt, blieb das Flag für immer
    // stehen - bis der nächste Energizer den Ablauf neu anstieß.
    frightAktiv = false;
    geisterKette = 0;
    for (uint8_t i = 0; i < GEIST_ANZAHL; i++) geister[i].aengstlich = false;

    spielStatus = LAEUFT;

    letzterTick = millis();
    pacAkku = 0;
    for (uint8_t i = 0; i < GEIST_ANZAHL; i++) geistAkku[i] = 0;
    dotImLetztenTick = false;
    letzterDotZeit = millis();
    phasenIdx = 0;
    phasenStart = millis();

    geistScatter = true; // wie im Original wird im Scatter-Modus gestartet

    Graphics::loadImage(); // komplettes Originalbild (Deko + Wände + HUD-Text)
    geisterhausLeeren();
    bildArtefakteEntfernen();
    alleDotsZeichnen();
    punkteZeichnen();
    lebenZeichnen();

    spritesZeichnen(); // gleiche Z-Reihenfolge wie im Spielverlauf
}

// Stufe der Level-Tabelle: 0 = Level 1, 1 = Level 2-4, 2 = Level 5-20, 3 = 21+
static uint8_t levelStufe()
{
    if (level <= 1) return 0;
    if (level <= 4) return 1;
    if (level <= 20) return 2;
    return 3;
}

// Lädt die Werte des aktuellen Levels aus der Tabelle
static void levelDatenLaden()
{
    memcpy_P(&lvl, &levelTabelle[levelStufe()], sizeof(LevelDaten));
}

// Baut das Labyrinth für das aktuelle Level neu auf. Punkte, Leben und
// Highscore bleiben erhalten - das ist der Übergang von Level n zu n+1.
static void levelStarten()
{
    levelDatenLaden();

    memset(dotEaten, 0, sizeof(dotEaten));
    dotsGegessen = 0;
    // frightAktiv/geisterKette/aengstlich werden zentral in
    // rundeZuruecksetzen() gelöscht (wird unten aufgerufen)

    // Vier Energizer: je einer pro Quadrant, Position darin zufällig
    energizerIdx[0] = waehleEnergizer(false, false); // oben links
    energizerIdx[1] = waehleEnergizer(true, false);  // oben rechts
    energizerIdx[2] = waehleEnergizer(false, true);  // unten links
    energizerIdx[3] = waehleEnergizer(true, true);   // unten rechts

    // Punkte zählen (nur Rasterzellen, die nach der Verschiebung noch
    // vollständig im Labyrinth liegen und nicht auf einer Wand landen)
    punkteUebrig = 0;
    for (uint8_t dr = 0; dr < DOT_ROWS; dr++)
    {
        for (uint8_t dc = 0; dc < DOT_COLS; dc++)
        {
            uint8_t x, y;
            if (dotZellPosition(dc, dr, x, y) && !istWand(x, y)) punkteUebrig++;
        }
    }

    rundeZuruecksetzen();

    // Startfeld von Pacman zählt nicht als Punkt
    uint16_t startIdx;
    if (dotZelleVon(pacmanX, pacmanY, startIdx) && !bitLesen(dotEaten, startIdx))
    {
        bitSetzen(dotEaten, startIdx);
        punkteUebrig--;
    }
}

static void neuesSpiel()
{
    punkte = 0; // Highscore bleibt bewusst stehen
    leben = LEBEN_MAX;
    level = 1;
    bonusStufe = 0;
    mundZaehler = 0;

    levelStarten();

    //Serial.println(F("Neues Spiel: WASD zum Bewegen"));
}

// Spiel aufbauen und erst nach BEREIT_MS loslaufen lassen - das Feld ist
// dann schon sichtbar, aber noch bewegt sich nichts
static void spielStarten()
{
    neuesSpiel();
    spielStatus = BEREIT;
    bereitZeit = millis();
}

void Game::init()
{
    Serial.begin(9600);

    if(EINGABEMODUS == CONTROLLER) controllerInit();
    // Zufallsgenerator anstoßen: ein offener Analogeingang liefert Rauschen.
    // Ohne das würde random() nach jedem Reset dieselbe Folge liefern und die
    // Energizer lägen jedes Mal an den gleichen Stellen.
    randomSeed(analogRead(A0) ^ micros());

    spielStatus = STARTBILDSCHIRM;
    startbildschirmZeichnen();
    //Serial.println(F("Taste druecken zum Starten"));
}

void Game::update()
{
    eingabeLesen();

    unsigned long jetzt = millis();

    // Startbildschirm: wartet nur auf einen Tastendruck
    if (spielStatus == STARTBILDSCHIRM) return;

    // Feld steht, aber es geht erst nach der Pause los
    if (spielStatus == BEREIT)
    {
        if (jetzt - bereitZeit < BEREIT_MS) return;

        spielStatus = LAEUFT;
        // Uhren erst jetzt starten, damit die Pause weder Phasen noch den
        // Freigabe-Timer anschiebt
        letzterTick = jetzt;
        letzterDotZeit = jetzt;
        phasenStart = jetzt;
        blinkStart = jetzt;
        return;
    }

    if (spielStatus == LEVEL_GESCHAFFT)
    {
        // "LEVEL x REACHED" steht kurz, danach geht es im nächsten Level weiter
        if (jetzt - todZeit >= LEVEL_TEXT_MS) levelStarten();
        return;
    }

    // Schritt 1: alles steht still, die Geister bleiben sichtbar stehen
    if (spielStatus == TOD_FREEZE)
    {
        if (jetzt - todZeit < TOD_FREEZE_MS) return;

        spritesLoeschen(); // Geister verschwinden
        spriteZeichnen(pacmanX, pacmanY, todIcon[0], PACMAN_X_SHIFT, PACMAN_Y_SHIFT);
        todFrame = 0;
        spielStatus = TOD_ANIM;
        todZeit = jetzt;
        return;
    }

    // Schritt 2: Pacman fällt in sich zusammen
    if (spielStatus == TOD_ANIM)
    {
        uint8_t frame = (uint8_t)((jetzt - todZeit) / (TOD_ANIM_MS / TOD_FRAMES));
        if (frame >= TOD_FRAMES)
        {
            spielStatus = TOD_WARTEN;
            todZeit = jetzt;
            return;
        }
        if (frame != todFrame)
        {
            todFrame = frame;
            spriteLoeschen(pacmanX, pacmanY, PACMAN_X_SHIFT, PACMAN_Y_SHIFT);
            spriteZeichnen(pacmanX, pacmanY, todIcon[frame], PACMAN_X_SHIFT, PACMAN_Y_SHIFT);
        }
        return;
    }

    // Schritt 3: kurze Pause, dann Leben abziehen und weiter oder Game Over
    if (spielStatus == TOD_WARTEN)
    {
        if (jetzt - todZeit < TOD_PAUSE_MS) return;

        if (leben > 0) leben--;
        lebenZeichnen();

        if (leben == 0)
        {
            spielStatus = GAMEOVER;
            todZeit = jetzt;
            gameoverPromptGezeichnet = false;
            gameoverBildschirmZeichnen();
            //Serial.print(F("Game Over! Punkte: "));
            //Serial.println(punkte);
        }
        else
        {
            //Serial.print(F("Leben verloren, uebrig: "));
            //Serial.println(leben);
            rundeZuruecksetzen();
        }
        return;
    }

    if (spielStatus == GAMEOVER)
    {
        // Der Hinweis kommt später als die Eingabefreigabe - wer schnell ist,
        // kann schon vorher neu starten (siehe tasteVerarbeiten)
        if (!gameoverPromptGezeichnet && jetzt - todZeit >= GAMEOVER_PROMPT_MS)
        {
            gameoverPromptGezeichnet = true;
            zeichneTextZentriert(36, "PRESS BUTTON TO RESTART", YELLOW);
        }
        return; // bleibt stehen, bis eine Taste kommt
    }

    // spielStatus == LAEUFT
    // Blinktakt für Energizer und ängstliche Geister
    if (jetzt - blinkStart >= BLINK_MS)
    {
        blinkStart = jetzt;
        blinkPhase = !blinkPhase;
        energizerZeichnen();
    }

    // Energizer-Effekt abgelaufen?
    if (frightAktiv && jetzt - frightStart >= lvl.frightMs) frightBeenden();

    // Freigabe der wartenden Geister nach gefressenen Punkten
    if (geister[1].zustand == IM_HAUS && dotsGegessen >= PINKY_DOTS)
        geister[1].zustand = VERLAESST_HAUS;
    if (geister[2].zustand == IM_HAUS && dotsGegessen >= INKY_DOTS)
        geister[2].zustand = VERLAESST_HAUS;

    // Notbremse: wer aufhört zu fressen, bekommt trotzdem Besuch
    unsigned long freigabeMs = (levelStufe() >= 2) ? FREIGABE_TIMER_L5_MS : FREIGABE_TIMER_L1_MS;
    if (jetzt - letzterDotZeit >= freigabeMs)
    {
        for (uint8_t i = 1; i < GEIST_ANZAHL; i++)
        {
            if (geister[i].zustand != IM_HAUS) continue;
            geister[i].zustand = VERLAESST_HAUS;
            break; // immer nur einen pro Ablauf
        }
        letzterDotZeit = jetzt;
    }

    geistModusPruefen(jetzt);

    // Basistakt: jede Figur sammelt ihren Prozentwert und läuft bei >=100
    // einen Pixel. Alles wird gemeinsam gelöscht, bewegt und gezeichnet -
    // damit stimmt die Z-Reihenfolge und die Kollision wird einmal geprüft.
    if (jetzt - letzterTick < BASIS_TICK_MS) return;
    letzterTick = jetzt;

    spritesLoeschen();

    uint8_t pct = pacProzent();
    dotImLetztenTick = false;
    pacAkku += pct;
    while (pacAkku >= 100 && spielStatus == LAEUFT)
    {
        pacAkku -= 100;
        bewegePacman();
    }

    for (uint8_t i = 0; i < GEIST_ANZAHL; i++)
    {
        geistAkku[i] += geistProzent(i);
        while (geistAkku[i] >= 100)
        {
            geistAkku[i] -= 100;
            geistSchritt(i);
        }
    }

    spritesZeichnen();
    kollisionPruefen();
}

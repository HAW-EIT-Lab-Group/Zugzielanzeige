// ---------------------------------------------------------------------------
// ZZA-Simulator - zeigt die 200x64-Bitmatrix des Spiels in einem Fenster.
//
// Es wird KEIN Spielcode nachgebaut: Game.cpp und Graphics.cpp aus lib/ werden
// unveraendert mitkompiliert. Ersetzt sind nur die Teile, die echte Hardware
// brauchen:
//   Arduino.h / avr/pgmspace.h -> simulator/shim
//   Display.cpp (GPIO)         -> src/sim_display.cpp
//
// Gezeichnet wird genau das, was das echte Panel anzeigen wuerde: die Farbe
// jedes Pixels wird aus Graphics::bitmap mit derselben Bitverteilung wie in
// Graphics::drawPixel() zurueckgelesen (siehe sim_display.cpp).
// ---------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>   // GET_X_LPARAM / GET_Y_LPARAM

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <string>
#include <vector>

#include "config.h"
#include "Graphics.h"
#include "Game.h"
#include "Display.h"
#include "MazeData.h"

#include "sim_runtime.h"
#include "sim_display.h"

// --- Layout -----------------------------------------------------------------
static const int PANEL_W   = WIDTH;       // 200
static const int PANEL_H   = HEIGHT_ALL;  // 64
static const int RAND_L    = 34;          // Platz fuer die Zeilenlineale
static const int RAND_O    = 22;          // Platz fuer das Spaltenlineal
static const int STATUS_H  = 146;         // Infobereich unter dem Panel
static const int SKAL_MIN  = 1;
static const int SKAL_MAX  = 24;

// --- Farben -----------------------------------------------------------------
// 0=BLACK, 1=GREEN, 2=RED, 3=YELLOW - so wie die LEDs auf dem Panel leuchten
static const uint32_t LED_FARBE[4] = { 0x00141414, 0x0028F028, 0x00FF3822, 0x00FFC814 };
static const uint32_t PANEL_LUECKE = 0x00000000;
static const COLORREF C_HINTERGRUND = RGB(24, 24, 28);
static const COLORREF C_TEXT        = RGB(220, 220, 220);
static const COLORREF C_TEXT_DIM    = RGB(140, 140, 150);
static const COLORREF C_GITTER      = RGB(70, 70, 80);
static const COLORREF C_SEKTION     = RGB(255, 0, 255);
static const COLORREF C_MAZE        = RGB(80, 160, 255);
static const COLORREF C_TUNNEL      = RGB(0, 220, 220);
static const COLORREF C_MAUS        = RGB(255, 255, 255);

// --- Zustand ----------------------------------------------------------------
static HWND     g_hwnd        = NULL;
static bool     g_laeuft      = true;
static bool     g_pause       = false;
static double   g_tempo       = 1.0;
static double   g_uhrRest     = 0.0;          // Sub-ms-Rest der virtuellen Uhr
static unsigned long g_virtMs = 0;

static bool     g_ovGitter    = false;
static bool     g_ovSektionen = false;
static bool     g_ovMaze      = false;
static bool     g_yfix        = false;

static int      g_mausX = -1, g_mausY = -1;   // Panel-Koordinaten unter der Maus
static double   g_fps   = 0.0;
static unsigned long g_frames = 0;
static int      g_schuss = 0;                 // Zaehler fuer Screenshots

// Panel-DIB (fertig skaliert, wird nur bei Skalierungswechsel neu gebaut)
static HDC      g_panelDC   = NULL;
static HBITMAP  g_panelBmp  = NULL;
static uint32_t *g_panelBits = NULL;
static int      g_skal      = 0;

// Doppelpufferung fuer das ganze Fenster
static HDC      g_backDC  = NULL;
static HBITMAP  g_backBmp = NULL;
static int      g_backW = 0, g_backH = 0;

static HFONT    g_font = NULL, g_fontKlein = NULL;

// ---------------------------------------------------------------------------
// Hilfsfunktionen
// ---------------------------------------------------------------------------
static const char *farbName(uint8_t c)
{
    switch (c)
    {
        case BLACK:  return "AUS";
        case GREEN:  return "GRUEN";
        case RED:    return "ROT";
        case YELLOW: return "GELB";
        default:     return "?";
    }
}

// Wanddaten aus MazeData.h - nur fuer die Anzeige unter der Maus
static bool istWandSim(int x, int y)
{
    if (x < MAZE_X0 || x > MAZE_X1 || y < 0 || y >= MAZE_H) return false;
    int lx = x - MAZE_X0;
    return (mazeWalls[y][lx >> 3] >> (7 - (lx & 7))) & 1;
}

static void exeVerzeichnis(char *puffer, size_t n)
{
    GetModuleFileNameA(NULL, puffer, (DWORD)n);
    char *slash = strrchr(puffer, '\\');
    if (slash) *slash = '\0';
}

// ---------------------------------------------------------------------------
// Panel-Bitmap
// ---------------------------------------------------------------------------
static void panelBitmapFreigeben()
{
    if (g_panelBmp) { DeleteObject(g_panelBmp); g_panelBmp = NULL; }
    if (g_panelDC)  { DeleteDC(g_panelDC);      g_panelDC  = NULL; }
    g_panelBits = NULL;
}

static void panelBitmapErzeugen(HDC ref, int skal)
{
    panelBitmapFreigeben();
    g_skal = skal;

    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof bi);
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = PANEL_W * skal;
    bi.bmiHeader.biHeight      = -(PANEL_H * skal);   // negativ = oben-nach-unten
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    g_panelDC  = CreateCompatibleDC(ref);
    g_panelBmp = CreateDIBSection(g_panelDC, &bi, DIB_RGB_COLORS, (void **)&g_panelBits, NULL, 0);
    SelectObject(g_panelDC, g_panelBmp);
}

// Ein LED-Kaestchen je Farbe vorberechnen (runde "LED" mit Luecke drumherum)
static void ledBloeckeBauen(int skal, std::vector<uint32_t> bloecke[4])
{
    double mitte = (skal - 1) / 2.0;
    double r     = (skal - 1) / 2.0 + 0.15;

    for (int c = 0; c < 4; c++)
    {
        bloecke[c].assign((size_t)skal * skal, PANEL_LUECKE);
        for (int j = 0; j < skal; j++)
        {
            for (int i = 0; i < skal; i++)
            {
                bool drin = true;
                if (skal >= 5)
                {
                    double dx = i - mitte, dy = j - mitte;
                    drin = (dx * dx + dy * dy) <= r * r;
                }
                bloecke[c][(size_t)j * skal + i] = drin ? LED_FARBE[c] : PANEL_LUECKE;
            }
        }
    }
}

static void panelZeichnen(int skal)
{
    static int              blockSkal = -1;
    static std::vector<uint32_t> bloecke[4];
    if (blockSkal != skal) { ledBloeckeBauen(skal, bloecke); blockSkal = skal; }

    const int pitch = PANEL_W * skal;

    for (int y = 0; y < PANEL_H; y++)
    {
        for (int x = 0; x < PANEL_W; x++)
        {
            uint8_t c = SimDisplay::pixelFarbe(x, y, g_yfix);
            const uint32_t *quelle = bloecke[c].data();
            uint32_t *ziel = g_panelBits + (size_t)(y * skal) * pitch + (size_t)x * skal;
            for (int j = 0; j < skal; j++)
                memcpy(ziel + (size_t)j * pitch, quelle + (size_t)j * skal, (size_t)skal * sizeof(uint32_t));
        }
    }
}

// ---------------------------------------------------------------------------
// Overlays und Text
// ---------------------------------------------------------------------------
static void linieH(HDC dc, int ox, int oy, int skal, int y, COLORREF farbe)
{
    HPEN stift = CreatePen(PS_SOLID, 1, farbe);
    HGDIOBJ alt = SelectObject(dc, stift);
    MoveToEx(dc, ox, oy + y * skal, NULL);
    LineTo(dc, ox + PANEL_W * skal, oy + y * skal);
    SelectObject(dc, alt);
    DeleteObject(stift);
}

static void linieV(HDC dc, int ox, int oy, int skal, int x, COLORREF farbe)
{
    HPEN stift = CreatePen(PS_SOLID, 1, farbe);
    HGDIOBJ alt = SelectObject(dc, stift);
    MoveToEx(dc, ox + x * skal, oy, NULL);
    LineTo(dc, ox + x * skal, oy + PANEL_H * skal);
    SelectObject(dc, alt);
    DeleteObject(stift);
}

static void overlaysZeichnen(HDC dc, int ox, int oy, int skal)
{
    if (g_ovGitter)
    {
        for (int x = 0; x <= PANEL_W; x += 10) linieV(dc, ox, oy, skal, x, C_GITTER);
        for (int y = 0; y <= PANEL_H; y += 8)  linieH(dc, ox, oy, skal, y, C_GITTER);

        // Lineale: Spalten alle 20 px, Zeilen alle 8 px
        SelectObject(dc, g_fontKlein);
        SetTextColor(dc, C_TEXT_DIM);
        char txt[16];
        for (int x = 0; x <= PANEL_W - 20; x += 20)
        {
            snprintf(txt, sizeof txt, "%d", x);
            TextOutA(dc, ox + x * skal + 1, oy - 15, txt, (int)strlen(txt));
        }
        for (int y = 0; y <= PANEL_H - 8; y += 8)
        {
            snprintf(txt, sizeof txt, "%d", y);
            TextOutA(dc, ox - 26, oy + y * skal, txt, (int)strlen(txt));
        }
    }

    if (g_ovSektionen)
    {
        SelectObject(dc, g_fontKlein);
        SetTextColor(dc, C_SEKTION);
        for (int s = 0; s < NR_OF_SECTIONS; s++)
        {
            int y = s * HEIGHT_SECTION;
            if (s > 0) linieH(dc, ox, oy, skal, y, C_SEKTION);
            char txt[48];
            // Bitposition in bitmap[x][y%16], vgl. Graphics::drawPixel()
            snprintf(txt, sizeof txt, "y %d-%d  Bits %d/%d",
                     y, y + HEIGHT_SECTION - 1,
                     ((NR_OF_SECTIONS - 1) - s) * 2, ((NR_OF_SECTIONS - 1) - s) * 2 + 1);
            TextOutA(dc, ox + PANEL_W * skal + 6, oy + y * skal, txt, (int)strlen(txt));
        }
    }

    if (g_ovMaze)
    {
        linieV(dc, ox, oy, skal, MAZE_X0,     C_MAZE);
        linieV(dc, ox, oy, skal, MAZE_X1 + 1, C_MAZE);
        linieH(dc, ox, oy, skal, TUNNEL_Y0,     C_TUNNEL);
        linieH(dc, ox, oy, skal, TUNNEL_Y1 + 1, C_TUNNEL);

        SelectObject(dc, g_fontKlein);
        SetTextColor(dc, C_MAZE);
        char txt[64];
        snprintf(txt, sizeof txt, "Labyrinth x=%d..%d", MAZE_X0, MAZE_X1);
        TextOutA(dc, ox + MAZE_X0 * skal + 2, oy + PANEL_H * skal + 2, txt, (int)strlen(txt));
        SetTextColor(dc, C_TUNNEL);
        snprintf(txt, sizeof txt, "Tunnel y=%d..%d", TUNNEL_Y0, TUNNEL_Y1);
        TextOutA(dc, ox + PANEL_W * skal + 6, oy + TUNNEL_Y0 * skal - 14, txt, (int)strlen(txt));
    }

    // Fadenkreuz auf dem Pixel unter der Maus
    if (g_mausX >= 0)
    {
        HPEN stift = CreatePen(PS_SOLID, 1, C_MAUS);
        HGDIOBJ altStift = SelectObject(dc, stift);
        HGDIOBJ altPinsel = SelectObject(dc, GetStockObject(NULL_BRUSH));
        Rectangle(dc, ox + g_mausX * skal - 1, oy + g_mausY * skal - 1,
                      ox + (g_mausX + 1) * skal + 1, oy + (g_mausY + 1) * skal + 1);
        SelectObject(dc, altPinsel);
        SelectObject(dc, altStift);
        DeleteObject(stift);
    }
}

static void textZeile(HDC dc, int x, int *y, COLORREF farbe, const char *fmt, ...)
{
    char puffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(puffer, sizeof puffer, fmt, args);
    va_end(args);

    SetTextColor(dc, farbe);
    TextOutA(dc, x, *y, puffer, (int)strlen(puffer));
    *y += 17;
}

static void statusZeichnen(HDC dc, int x, int y, int skal)
{
    SelectObject(dc, g_font);

    textZeile(dc, x, &y, C_TEXT,
              "Zeit %6lu ms | %5.1f FPS | Tempo %.2fx | %s | Frames %lu | Display::refresh() %lu",
              g_virtMs, g_fps, g_tempo, g_pause ? "PAUSE" : "laeuft",
              g_frames, SimDisplay::refreshZaehler());

    if (g_mausX >= 0)
    {
        uint8_t c = SimDisplay::pixelFarbe(g_mausX, g_mausY, g_yfix);
        int zeile = g_mausY % HEIGHT_SECTION;
        textZeile(dc, x, &y, C_TEXT,
                  "Pixel x=%3d y=%2d | Farbe %-5s (%d) | Sektion %d, bitmap[%d][%d] = 0x%02X | Wand: %s | im Labyrinth: %s",
                  g_mausX, g_mausY, farbName(c), c,
                  g_mausY / HEIGHT_SECTION, g_mausX, zeile, Graphics::bitmap[g_mausX][zeile],
                  istWandSim(g_mausX, g_mausY) ? "ja" : "nein",
                  (g_mausX >= MAZE_X0 && g_mausX <= MAZE_X1) ? "ja" : "nein");
    }
    else
    {
        textZeile(dc, x, &y, C_TEXT_DIM, "Pixel: Maus ueber das Panel bewegen fuer Koordinaten/Farbe/Bitmap-Byte");
    }

    textZeile(dc, x, &y, C_TEXT_DIM,
              "Spiel:  W A S D = bewegen, R = Neustart   |   Skalierung %dx (Fenstergroesse aendern)", skal);
    textZeile(dc, x, &y, C_TEXT_DIM,
              "Zeit:   Leertaste = Pause/weiter, N = +90 ms (ein Spielzug), B = +1 ms, +/- = Tempo, 0 = 1.00x");
    textZeile(dc, x, &y, C_TEXT_DIM,
              "Ansicht:F2 Gitter %s | F3 Sektionen %s | F4 Labyrinth %s | F5 refresh-Zeilenversatz %s | F11 Textdump | F12 Screenshot | ESC Ende",
              g_ovGitter ? "AN " : "aus", g_ovSektionen ? "AN " : "aus",
              g_ovMaze ? "AN " : "aus", g_yfix ? "AN " : "aus");

    y += 4;
    int anzahl = SimRuntime::logAnzahl();
    for (int i = anzahl - 3; i < anzahl; i++)
    {
        if (i < 0) continue;
        textZeile(dc, x, &y, RGB(120, 200, 120), "Serial> %s", SimRuntime::logZeile(i));
    }
}

// ---------------------------------------------------------------------------
// Kompletter Bildaufbau
// ---------------------------------------------------------------------------
static void bildZeichnen(HDC ziel, int breite, int hoehe)
{
    // Rueckpuffer anlegen/anpassen
    if (!g_backDC) g_backDC = CreateCompatibleDC(ziel);
    if (breite != g_backW || hoehe != g_backH)
    {
        if (g_backBmp) DeleteObject(g_backBmp);
        g_backBmp = CreateCompatibleBitmap(ziel, breite, hoehe);
        SelectObject(g_backDC, g_backBmp);
        g_backW = breite; g_backH = hoehe;
    }

    // Skalierung so waehlen, dass das Panel ins Fenster passt
    int skal = (breite - RAND_L - 10) / PANEL_W;
    int skalY = (hoehe - RAND_O - STATUS_H) / PANEL_H;
    if (skalY < skal) skal = skalY;
    if (skal < SKAL_MIN) skal = SKAL_MIN;
    if (skal > SKAL_MAX) skal = SKAL_MAX;
    if (skal != g_skal) panelBitmapErzeugen(ziel, skal);

    HBRUSH hg = CreateSolidBrush(C_HINTERGRUND);
    RECT alles = { 0, 0, breite, hoehe };
    FillRect(g_backDC, &alles, hg);
    DeleteObject(hg);

    SetBkMode(g_backDC, TRANSPARENT);

    panelZeichnen(skal);
    BitBlt(g_backDC, RAND_L, RAND_O, PANEL_W * skal, PANEL_H * skal, g_panelDC, 0, 0, SRCCOPY);

    overlaysZeichnen(g_backDC, RAND_L, RAND_O, skal);
    statusZeichnen(g_backDC, 10, RAND_O + PANEL_H * skal + 14, skal);

    BitBlt(ziel, 0, 0, breite, hoehe, g_backDC, 0, 0, SRCCOPY);
}

// ---------------------------------------------------------------------------
// Ausgabe in Dateien
// ---------------------------------------------------------------------------
// Bitmatrix als Text: '.' aus, 'G' gruen, 'R' rot, 'Y' gelb
static void textDump(FILE *f)
{
    fprintf(f, "# ZZA-Simulator Bitmatrix %dx%d bei t=%lu ms\n", PANEL_W, PANEL_H, g_virtMs);
    fprintf(f, "# . = aus, G = gruen, R = rot, Y = gelb\n");
    fprintf(f, "     ");
    for (int x = 0; x < PANEL_W; x += 10) fprintf(f, "%-10d", x);
    fprintf(f, "\n");
    for (int y = 0; y < PANEL_H; y++)
    {
        fprintf(f, "%3d  ", y);
        for (int x = 0; x < PANEL_W; x++)
            fputc(".GRY"[SimDisplay::pixelFarbe(x, y, g_yfix)], f);
        fprintf(f, "\n");
    }
}

static void textDumpSpeichern()
{
    char pfad[MAX_PATH];
    exeVerzeichnis(pfad, sizeof pfad);
    strncat(pfad, "\\ausgabe", sizeof(pfad) - strlen(pfad) - 1);
    CreateDirectoryA(pfad, NULL);

    char datei[MAX_PATH];
    snprintf(datei, sizeof datei, "%s\\bitmatrix_%lu.txt", pfad, g_virtMs);
    FILE *f = fopen(datei, "w");
    if (!f) { printf("[Sim] Konnte %s nicht schreiben\n", datei); return; }
    textDump(f);
    fclose(f);
    printf("[Sim] Textdump gespeichert: %s\n", datei);
}

// 24-Bit-BMP aus dem (bereits skalierten) Panel-DIB schreiben
static void screenshotSpeichern()
{
    if (!g_panelBits) return;
    int w = PANEL_W * g_skal, h = PANEL_H * g_skal;
    int zeilenBytes = (w * 3 + 3) & ~3;

    char pfad[MAX_PATH];
    exeVerzeichnis(pfad, sizeof pfad);
    strncat(pfad, "\\ausgabe", sizeof(pfad) - strlen(pfad) - 1);
    CreateDirectoryA(pfad, NULL);

    char datei[MAX_PATH];
    snprintf(datei, sizeof datei, "%s\\screenshot_%03d.bmp", pfad, ++g_schuss);
    FILE *f = fopen(datei, "wb");
    if (!f) { printf("[Sim] Konnte %s nicht schreiben\n", datei); return; }

    BITMAPFILEHEADER fh; ZeroMemory(&fh, sizeof fh);
    BITMAPINFOHEADER ih; ZeroMemory(&ih, sizeof ih);
    fh.bfType    = 0x4D42;
    fh.bfOffBits = sizeof fh + sizeof ih;
    fh.bfSize    = fh.bfOffBits + zeilenBytes * h;
    ih.biSize    = sizeof ih;
    ih.biWidth   = w;
    ih.biHeight  = h;          // positiv = unten-nach-oben
    ih.biPlanes  = 1;
    ih.biBitCount = 24;
    fwrite(&fh, sizeof fh, 1, f);
    fwrite(&ih, sizeof ih, 1, f);

    std::vector<unsigned char> zeile((size_t)zeilenBytes, 0);
    for (int y = h - 1; y >= 0; y--)
    {
        for (int x = 0; x < w; x++)
        {
            uint32_t p = g_panelBits[(size_t)y * w + x];
            zeile[(size_t)x * 3 + 0] = (unsigned char)(p & 0xFF);
            zeile[(size_t)x * 3 + 1] = (unsigned char)((p >> 8) & 0xFF);
            zeile[(size_t)x * 3 + 2] = (unsigned char)((p >> 16) & 0xFF);
        }
        fwrite(zeile.data(), 1, (size_t)zeilenBytes, f);
    }
    fclose(f);
    printf("[Sim] Screenshot gespeichert: %s\n", datei);
}

// ---------------------------------------------------------------------------
// Eingaben
// ---------------------------------------------------------------------------
static void zeitSchritt(unsigned long ms)
{
    g_virtMs += ms;
    SimRuntime::uhrSetzen(g_virtMs);
    printf("[Sim] Zeitschritt +%lu ms -> t=%lu ms\n", ms, g_virtMs);
}

static void zeichenTaste(char c)
{
    switch (c)
    {
        // Spieltasten unveraendert an Serial weiterreichen
        case 'w': case 'W': case 'a': case 'A':
        case 's': case 'S': case 'd': case 'D':
        case 'r': case 'R':
            SimRuntime::tasteSenden(c);
            break;

        case 'n': case 'N': zeitSchritt(90); break;   // ein Spielzug
        case 'b': case 'B': zeitSchritt(1);  break;
        case '+':           g_tempo = g_tempo * 1.5 > 16.0 ? 16.0 : g_tempo * 1.5; break;
        case '-':           g_tempo = g_tempo / 1.5 < 0.05 ? 0.05 : g_tempo / 1.5; break;
        case '0':           g_tempo = 1.0; break;
        default: break;
    }
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_DESTROY:
            g_laeuft = false;
            PostQuitMessage(0);
            return 0;

        case WM_ERASEBKGND:
            return 1; // wird komplett selbst gezeichnet

        case WM_CHAR:
            zeichenTaste((char)wp);
            return 0;

        case WM_KEYDOWN:
            switch (wp)
            {
                case VK_ESCAPE: g_laeuft = false; DestroyWindow(hwnd); break;
                case VK_SPACE:  g_pause = !g_pause; break;
                case VK_F2:     g_ovGitter    = !g_ovGitter;    break;
                case VK_F3:     g_ovSektionen = !g_ovSektionen; break;
                case VK_F4:     g_ovMaze      = !g_ovMaze;      break;
                case VK_F5:     g_yfix        = !g_yfix;        break;
                case VK_F11:    textDumpSpeichern();   break;
                case VK_F12:    screenshotSpeichern(); break;
                case VK_ADD:      g_tempo = g_tempo * 1.5 > 16.0 ? 16.0 : g_tempo * 1.5; break;
                case VK_SUBTRACT: g_tempo = g_tempo / 1.5 < 0.05 ? 0.05 : g_tempo / 1.5; break;
                default: break;
            }
            return 0;

        case WM_MOUSEMOVE:
        {
            int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
            if (g_skal > 0)
            {
                int px = (mx - RAND_L) / g_skal;
                int py = (my - RAND_O) / g_skal;
                if (mx >= RAND_L && my >= RAND_O && px < PANEL_W && py < PANEL_H)
                {
                    g_mausX = px; g_mausY = py;
                }
                else { g_mausX = g_mausY = -1; }
            }
            return 0;
        }

        case WM_LBUTTONDOWN:
            if (g_mausX >= 0)
            {
                uint8_t c = SimDisplay::pixelFarbe(g_mausX, g_mausY, g_yfix);
                printf("[Sim] x=%d y=%d Farbe=%s(%d) bitmap[%d][%d]=0x%02X Wand=%s\n",
                       g_mausX, g_mausY, farbName(c), c,
                       g_mausX, g_mausY % HEIGHT_SECTION,
                       Graphics::bitmap[g_mausX][g_mausY % HEIGHT_SECTION],
                       istWandSim(g_mausX, g_mausY) ? "ja" : "nein");
                fflush(stdout);
            }
            return 0;

        default:
            return DefWindowProcA(hwnd, msg, wp, lp);
    }
}

// ---------------------------------------------------------------------------
// Kopflos: feste Zeitschritte, danach Textdump - fuer Skripte/Regressionstests
// ---------------------------------------------------------------------------
static int kopflosLaufen(unsigned long dauerMs, const char *tasten, const char *dumpDatei)
{
    Display::init();
    Game::init();
    Display::refresh();

    if (tasten)
        for (const char *t = tasten; *t; t++) SimRuntime::tasteSenden(*t);

    const unsigned long SCHRITT = 5; // ms je Iteration -> deterministisch
    for (unsigned long t = 0; t <= dauerMs; t += SCHRITT)
    {
        g_virtMs = t;
        SimRuntime::uhrSetzen(t);
        Game::update();
        Display::refresh();
    }

    FILE *f = stdout;
    if (dumpDatei)
    {
        f = fopen(dumpDatei, "w");
        if (!f) { printf("[Sim] Konnte %s nicht schreiben\n", dumpDatei); return 1; }
    }
    textDump(f);
    if (dumpDatei) { fclose(f); printf("[Sim] Dump geschrieben: %s\n", dumpDatei); }
    return 0;
}

static void hilfeAusgeben()
{
    printf("ZZA-Simulator\n"
           "  zza_sim.exe                       Fenster mit 200x64-Panel oeffnen\n"
           "  zza_sim.exe --ms 6000 [--keys dd] [--dump datei.txt]\n"
           "                                    ohne Fenster rechnen und Bitmatrix als Text ausgeben\n"
           "  zza_sim.exe --autoquit 5 [--shot] Fenster oeffnen, nach 5 s beenden (fuer Skripte)\n"
           "  zza_sim.exe --seed 42             anderes Spiel wuerfeln (wirkt auf analogRead)\n"
           "                                    ohne --seed laeuft jeder Start exakt gleich ab\n"
           "  zza_sim.exe --help                diese Hilfe\n");
}

// ---------------------------------------------------------------------------
int main(int argc, char **argv)
{
    unsigned long kopflosMs = 0;
    const char *tasten = NULL, *dumpDatei = NULL;
    bool kopflos = false;
    double autoQuitS = 0.0;
    bool schussBeimEnde = false;

    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) { hilfeAusgeben(); return 0; }
        else if (!strcmp(argv[i], "--ms")   && i + 1 < argc) { kopflosMs = strtoul(argv[++i], NULL, 10); kopflos = true; }
        else if (!strcmp(argv[i], "--keys") && i + 1 < argc) { tasten = argv[++i]; }
        else if (!strcmp(argv[i], "--dump") && i + 1 < argc) { dumpDatei = argv[++i]; }
        else if (!strcmp(argv[i], "--seed") && i + 1 < argc)
            SimRuntime::analogRauschenSetzen((uint32_t)strtoul(argv[++i], NULL, 10));
        else if (!strcmp(argv[i], "--autoquit") && i + 1 < argc) { autoQuitS = atof(argv[++i]); }
        else if (!strcmp(argv[i], "--shot")) { schussBeimEnde = true; }
        else { printf("Unbekannte Option: %s\n", argv[i]); hilfeAusgeben(); return 1; }
    }

    if (kopflos) return kopflosLaufen(kopflosMs, tasten, dumpDatei);

    // --- Fenster anlegen ---
    HINSTANCE inst = GetModuleHandle(NULL);
    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = inst;
    wc.hCursor       = LoadCursor(NULL, IDC_CROSS);
    wc.lpszClassName = "ZZA_SIM";
    RegisterClassA(&wc);

    int startSkal = 7;
    RECT r = { 0, 0, RAND_L + PANEL_W * startSkal + 10, RAND_O + PANEL_H * startSkal + STATUS_H };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);

    g_hwnd = CreateWindowExA(0, "ZZA_SIM", "ZZA-Simulator - 200x64 Bitmatrix",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             r.right - r.left, r.bottom - r.top, NULL, NULL, inst, NULL);
    if (!g_hwnd) { printf("Fenster konnte nicht erstellt werden\n"); return 1; }
    ShowWindow(g_hwnd, SW_SHOW);

    g_font      = CreateFontA(15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              FIXED_PITCH | FF_MODERN, "Consolas");
    g_fontKlein = CreateFontA(11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              FIXED_PITCH | FF_MODERN, "Consolas");

    // --- Spiel starten (genau wie src/main.cpp) ---
    Display::init();
    Game::init();
    Display::refresh();

    LARGE_INTEGER freq, letzte, fpsMarke, start;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&letzte);
    fpsMarke = letzte;
    start    = letzte;
    unsigned long fpsFrames = 0;

    MSG msg;
    while (g_laeuft)
    {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) g_laeuft = false;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (!g_laeuft) break;

        LARGE_INTEGER jetzt;
        QueryPerformanceCounter(&jetzt);
        double deltaMs = (double)(jetzt.QuadPart - letzte.QuadPart) * 1000.0 / (double)freq.QuadPart;
        letzte = jetzt;
        if (deltaMs > 250.0) deltaMs = 250.0; // nach Fenster-Verschieben nicht "aufholen"

        // virtuelle Uhr weiterstellen (pausiert / gebremst / beschleunigt)
        if (!g_pause)
        {
            g_uhrRest += deltaMs * g_tempo;
            unsigned long ganze = (unsigned long)g_uhrRest;
            g_uhrRest -= (double)ganze;
            g_virtMs += ganze;
        }
        SimRuntime::uhrSetzen(g_virtMs);

        // genau die loop() aus src/main.cpp
        Game::update();
        Display::refresh();

        HDC dc = GetDC(g_hwnd);
        RECT cr; GetClientRect(g_hwnd, &cr);
        bildZeichnen(dc, cr.right, cr.bottom);
        ReleaseDC(g_hwnd, dc);

        g_frames++;
        fpsFrames++;
        double seitMarke = (double)(jetzt.QuadPart - fpsMarke.QuadPart) / (double)freq.QuadPart;
        if (seitMarke >= 0.5)
        {
            g_fps = fpsFrames / seitMarke;
            fpsFrames = 0;
            fpsMarke = jetzt;
        }

        if (autoQuitS > 0.0 &&
            (double)(jetzt.QuadPart - start.QuadPart) / (double)freq.QuadPart >= autoQuitS)
        {
            printf("[Sim] --autoquit nach %.1f s: %lu Bilder gezeichnet, %.1f FPS\n",
                   autoQuitS, g_frames, g_fps);
            if (schussBeimEnde) screenshotSpeichern();
            g_laeuft = false;
        }

        Sleep(4); // ~60-120 Bilder/s, CPU bleibt ruhig
    }

    panelBitmapFreigeben();
    if (g_backBmp) DeleteObject(g_backBmp);
    if (g_backDC)  DeleteDC(g_backDC);
    if (g_font)      DeleteObject(g_font);
    if (g_fontKlein) DeleteObject(g_fontKlein);
    return 0;
}

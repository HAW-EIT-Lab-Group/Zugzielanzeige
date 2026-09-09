# ZZA-Simulator

Zeigt die 200x64-Bitmatrix des Spiels in einem Windows-Fenster – zum Debuggen
ohne Arduino und ohne LED-Panel.

**Wichtig:** Der Simulator baut das Spiel *nicht* nach. `lib/Game/Game.cpp` und
`lib/Graphics/Graphics.cpp` werden unverändert mitkompiliert. Was im Fenster zu
sehen ist, ist genau das, was auch auf dem Panel leuchten würde – die Farbe
jedes Pixels wird mit derselben Bitverteilung wie in `Graphics::drawPixel()` aus
`Graphics::bitmap` zurückgelesen (siehe `src/sim_display.cpp`).

Ersetzt sind nur die Teile, die echte Hardware brauchen:

| Original                       | Ersatz im Simulator      |
|--------------------------------|--------------------------|
| `Arduino.h`, `avr/pgmspace.h`  | `shim/`                  |
| `lib/Display/Display.cpp` (GPIO, Schieberegister) | `src/sim_display.cpp` |
| `src/main.cpp` (setup/loop)    | `src/sim_main.cpp`       |

Ändert man etwas am Spiel, reicht ein erneutes `build.bat` – ein Auseinander-
laufen von Spiel und Simulator ist nicht möglich.

## Was der Simulator automatisch mitbekommt

Normal weiterprogrammieren, `run.bat` starten – mehr ist nicht nötig:

* Änderungen an `Game.cpp`, `Graphics.cpp`, `config.h`, `MazeData.h`,
  `imageData.h` und allen anderen Headern. Es wird jedes Mal komplett neu
  übersetzt, veraltete Objektdateien kann es nicht geben.
* **Neue Module:** alle `.cpp` unter `lib/` werden automatisch eingesammelt,
  jeder Unterordner von `lib/` wird als Include-Pfad gesetzt. Legst du
  `lib/Sound/Sound.cpp` an, läuft die Datei ohne Änderung an `build.bat` mit.
  Beim Bauen wird aufgelistet, was gefunden wurde (`+` = übersetzt,
  `-` = übersprungen).

Drei Stellen musst du selbst nachziehen – sie ersetzen ja gerade die Hardware:

1. **`lib/Display/Display.cpp`** wird bewusst nicht übersetzt (spricht GPIOs an).
   Änderungen dort haben im Simulator keine Wirkung.
2. Änderst du die **Bitverteilung** in `Graphics::drawPixel()`, muss
   `SimDisplay::pixelFarbe()` in `src/sim_display.cpp` gleich mitgeändert
   werden – das ist die Umkehrfunktion dazu.
3. Kommt in `src/main.cpp` etwas Neues in `setup()`/`loop()` dazu, gehört es
   auch in `src/sim_main.cpp` (dort sind die beiden Aufrufe
   `Game::init()`/`Game::update()` nachgebaut).

## Voraussetzung

Visual Studio 2019/2022 mit der Arbeitslast **„Desktopentwicklung mit C++“**.
`build.bat` sucht die Installation selbst (über `vswhere.exe`), es muss keine
Developer-Eingabeaufforderung geöffnet werden. Sonst wird nichts gebraucht –
keine Bibliotheken, kein SDL, kein Python.

## Starten

```bash
run.bat
```

Baut den Simulator und öffnet das Fenster. Nur bauen: `build.bat`,
danach `zza_sim.exe` direkt starten.

## Bedienung

**Spiel** (geht durch den echten `Serial.read()`-Pfad des Spiels):

| Taste | Wirkung |
|-------|---------|
| `W` `A` `S` `D` | Pacman bewegen |
| `R` | Neustart nach Sieg/Niederlage |

**Zeit** – das Spiel läuft an einer virtuellen Uhr, `millis()` liefert deren
Wert. Damit lässt sich jeder Spielzug einzeln ansehen:

| Taste | Wirkung |
|-------|---------|
| Leertaste | Pause / weiter |
| `N` | +90 ms = genau ein Spielzug (`GEIST_TICK_MS`) |
| `B` | +1 ms |
| `+` / `-` | Zeitlupe bis Zeitraffer (0.05x … 16x) |
| `0` | zurück auf 1.00x |

**Ansicht:**

| Taste | Wirkung |
|-------|---------|
| `F2` | Gitter + Lineale (Spalten alle 20 px, Zeilen alle 8 px) |
| `F3` | Grenzen der 4 Sektionen + welche Bits in `bitmap[x][y%16]` dazugehören |
| `F4` | Labyrinth-Grenzen `MAZE_X0`/`MAZE_X1` und Tunnelzeilen `TUNNEL_Y0..Y1` |
| `F5` | Zeilenversatz aus `Display::refresh()` nachbilden (siehe unten) |
| `F11` | Bitmatrix als Textdatei nach `ausgabe/` |
| `F12` | Screenshot (BMP) nach `ausgabe/` |
| `ESC` | Beenden |

Die **Maus** über das Panel bewegen zeigt unten laufend: Koordinaten, Farbe,
Sektion, den rohen Wert `bitmap[x][y%16]` und ob an der Stelle laut
`MazeData.h` eine Wand ist. Linksklick schreibt dieselben Angaben zusätzlich in
die Konsole.

Serial-Ausgaben des Spiels (`Serial.println(...)`) landen in der Konsole und in
den letzten Zeilen des Infobereichs. Das Fenster ist frei skalierbar, die
Pixelgröße passt sich an.

## Ohne Fenster (Skripte, Regressionstests)

```bash
zza_sim.exe --ms 6000 --keys "ss" --dump lauf.txt
```

Rechnet 6000 ms virtuelle Zeit in festen 5-ms-Schritten – also vollkommen
deterministisch, unabhängig von der Rechnerleistung – schickt vorher die Tasten
`ss` an das Spiel und schreibt die Bitmatrix als Text (`.` aus, `G` grün, `R`
rot, `Y` gelb). Ohne `--dump` geht die Ausgabe nach stdout.

Zwei solche Dumps mit `diff` zu vergleichen ist der schnellste Weg, um zu sehen,
was eine Änderung am Spiel wirklich am Bild verändert hat.

`zza_sim.exe --autoquit 5 [--shot]` öffnet das Fenster und beendet sich nach
5 Sekunden von selbst, optional mit Screenshot.

## Zum Zeilenversatz (`F5`)

`Display::refresh()` wählt Zeile `y` an, schiebt aber `bitmap[x][y+1]` heraus
(bei `y == 15` die Zeile 0). Der Simulator zeigt standardmäßig die saubere
Umkehrung von `Graphics::drawPixel()`, also das Bild, das das Spiel zu zeichnen
glaubt. Mit `F5` lässt sich der Versatz aus `refresh()` zuschalten – nützlich,
falls das echte Panel gegenüber dem Simulator um eine Zeile verschoben aussieht.

## Dateien

```
simulator/
  build.bat          sucht Visual Studio, kompiliert alles
  run.bat            build.bat + starten (Argumente werden durchgereicht)
  shim/
    Arduino.h        millis(), Serial, F(), GPIO-Attrappen
    avr/pgmspace.h   PROGMEM/pgm_read_byte als normale Zugriffe
  src/
    sim_main.cpp     Fenster, Eingaben, Zeichnen, Overlays, Dumps
    sim_arduino.cpp  virtuelle Uhr, Serial <-> Fenster/Konsole
    sim_display.cpp  Ersatz für Display.cpp, Pixel-Rücklesen aus der Bitmap
  build/             Zwischendateien (wird angelegt)
  ausgabe/           Screenshots und Textdumps (wird angelegt)
```

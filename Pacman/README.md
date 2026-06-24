# Pac-Man for the Zugzielanzeige (200×64 LED matrix)

A self-contained, **arcade-style Pac-Man** that runs on the same Arduino Mega
1280 + LED panel as the main project, plus a **self-playing AI** that plays it
on its own (so you can watch it without a controller connected).

This folder is fully separate from the rest of the repo. It only *reads* the
original hardware definitions from [`../ArduinoSketch/header.h`](../ArduinoSketch/header.h)
(via `include/pacman_config.h`); nothing in `ArduinoSketch/`, `src/` or the
root `platformio.ini` is modified.

## What it does

```
TITLE  ──(AI presses START)──►  READY  ──►  PLAY  ──►  DYING ──┐
  ▲                                          ▲   │             │
  │                                          └───┘ (lives left)│
  └──────────────────  GAME OVER  ◄──────────────────────────-┘ (no lives left)
```

* Title / attract screen ("PAC-MAN", blinking "PUSH START", "DEMO – AI PLAY").
* Press start → the maze, dots, 4 power pellets, 4 ghosts, score & lives.
* Classic ghost behaviour: **scatter / chase** phases, the four arcade target
  rules (Blinky/Pinky/Inky/Clyde), **frightened** mode on a power pellet
  (ghosts flee, are edible for 200→400→800→1600 pts and return as eyes), and a
  timed ghost-house release.
* Lose all lives → **GAME OVER** → back to the title screen, looping forever.

The **AI player** ([`src/ai_player.cpp`](src/ai_player.cpp)) is the stand-in for
a joystick: it BFS-searches the maze to the nearest dot/pellet, avoids tiles next
to chasing ghosts, hunts frightened ghosts, and presses START on the title
screen. It is mortal on purpose — when it gets cornered you'll see the title
screen come back.

## Timing model (the important part)

Two clocks, deliberately decoupled:

| Clock | Where | Rate | Purpose |
|-------|-------|------|---------|
| **Display refresh** | `Timer1` ISR in `src/display.cpp` | one local row every `ROW_SCAN_US` (800 µs) → 16 rows = ~78 Hz | **Constant, never affected by game load.** This is the "constant frame rate not influenced by anything else." |
| **Game tick** | `loop()` in `src/main.cpp` | fixed `FRAME_US` = 15 ms (~66 Hz) | Your "15 ms to calculate per frame" budget. All logic runs in well under it. |

Because the panel is multiplexed from a hardware timer, the picture stays rock
steady no matter how long a game frame takes to compute.

## Memory

The Mega 1280 has only **8 KB SRAM**, so the original `bool pixels[200][64]`
(25.6 KB) can't exist. Everything here is bit-packed:

* framebuffer: 2 bit-planes × 200×64 = **3200 B**
* maze (walls/dots/pellets): **~336 B**
* AI search buffers: **~2.5 KB** (static, never on the stack)

The maze is **generated at boot** from corridor rules (every horizontal corridor
crosses every vertical one ⇒ guaranteed fully connected), so there is no large
layout table in RAM and no hand-typed maze to get wrong.

## ⚠️ Hardware you MUST check before running

All of these are one-line edits in [`include/pacman_config.h`](include/pacman_config.h):

1. **Section 1–3 data pins.** The original wires only section 0 (data on 22/24).
   For the full 200×64 panel each of the other three ribbons needs its own
   red+green data line. I assigned `40–45` as placeholders:
   ```c
   #define P_R1 40   #define P_G1 41
   #define P_R2 42   #define P_G2 43
   #define P_R3 44   #define P_G3 45
   ```
   Change these to match your actual wiring. (The global signals
   A0/A1/A2/CS/CLK/STR/EN_R/EN_G are shared and taken straight from the tested
   `src/main.cpp`.)
2. **Enable polarity** — `ENABLE_ACTIVE_HIGH` (default 1, matching `src/main.cpp`).
   If the whole panel is inverted, set it to 0.
3. **Column order** — `COL_REVERSED` (default 0). If the image is mirrored
   left↔right, set it to 1.

## Build & upload

This is its own PlatformIO project. **Open this `Pacman/` folder** (not the repo
root) in PlatformIO / VS Code, then build & upload as usual:

```
pio run -t upload      # from inside Pacman/
```

The repo-root `platformio.ini` still builds the original `src/main.cpp`,
untouched.

## Playing it by hand later

Input is abstracted in [`include/input.h`](include/input.h). To use real
buttons/a joystick, write a `controller.cpp` that fills the same `Input`
(`dir` + `start`) from your pins and call it instead of `aiPoll(...)` in
`src/main.cpp`. Nothing in the game logic has to change.

## File map

| File | Role |
|------|------|
| `include/pacman_config.h` | pins (reuses `../ArduinoSketch/header.h`), timing, colours |
| `src/display.cpp` | bit-packed framebuffer + 4-section Timer1 multiplex driver |
| `src/gfx.cpp` | rectangles + 5×7 font |
| `src/maze.cpp` | boot-time maze generation + queries |
| `src/game.cpp` | state machine, movement, ghost AI, rendering |
| `src/ai_player.cpp` | **the self-playing controller** |
| `src/main.cpp` | setup + fixed-timestep loop |

## Tunables

In `src/game.cpp`: `START_LIVES` (set to `1` if you want the title screen after
*every* death), `SCATTER_TICKS`/`CHASE_TICKS`, `FRIGHT_TICKS`, speeds in
`ghostSpeedSteps()`. Refresh rate is `ROW_SCAN_US`, game rate is `FRAME_US`
(both in `include/pacman_config.h`).

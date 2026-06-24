/*----------------------------------------------------------------------------
 * main.cpp  -  entry point.
 *
 * Two clocks, deliberately decoupled:
 *   - The DISPLAY is refreshed by a Timer1 ISR (see display.cpp) at a fixed
 *     rate, so the picture never flickers no matter how long a frame's logic
 *     takes -> "constant frame rate, not influenced by anything else".
 *   - The GAME advances on a fixed 15 ms tick here in loop(). That is the
 *     per-frame compute budget; all the logic above runs in well under it.
 *--------------------------------------------------------------------------*/
#include <Arduino.h>
#include "display.h"
#include "game.h"
#include "ai_player.h"

static uint32_t nextFrame;

void setup() {
    Serial.begin(115200);
    dispBegin();        // pins + multiplex ISR (display now refreshes on its own)
    gameInit();
    nextFrame = micros();
}

void loop() {
    uint32_t now = micros();
    // Fixed-timestep tick. Signed compare handles micros() wraparound.
    if ((int32_t)(now - nextFrame) >= 0) {
        nextFrame += FRAME_US;

        Input in = aiPoll(gameGetState());   // the self-playing controller
        gameUpdate(in);
        gameRender();                        // draws into the framebuffer the ISR shows
    }
}

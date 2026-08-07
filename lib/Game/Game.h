#pragma once

namespace Game
{
    // einmalig in setup() aufrufen
    void init();

    // muss in jeder loop()-Iteration aufgerufen werden
    // blockiert nie (kein delay()), damit Display::refresh() nicht ins Stocken kommt
    void update();
}

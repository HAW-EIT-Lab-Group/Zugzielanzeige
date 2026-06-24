#pragma once
#include <stdint.h>

namespace Display
{
    void init();
    void refresh();
    void enable(uint8_t red, uint8_t green);
}
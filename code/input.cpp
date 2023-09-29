#pragma once

#if __clang__
#include "headers.h"
#endif

bool IsCommandDown(COMMAND command)
{
    bool result = G_COMMAND_STATES[command].down;
    return result;
}

bool IsCommandPressed(COMMAND command)
{
    bool result = G_COMMAND_STATES[command].pressed;
    return result;
}
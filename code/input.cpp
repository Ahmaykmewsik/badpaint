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

void InitNotificationMessage(String string, Arena *circularScratchBuffer)
{
    G_NOTIFICATION_MESSAGE = string;
    MoveStringToArena(&G_NOTIFICATION_MESSAGE, circularScratchBuffer);
    G_NOTIFICATION_ALPHA = 1.0;
}

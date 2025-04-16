#pragma once

#include "input.h"
#include "main.h"

bool IsCommandDown(COMMAND command)
{
    bool result = COMMAND_STATES[command].down;
    return result;
}

bool IsCommandPressed(COMMAND command)
{
    bool result = COMMAND_STATES[command].pressed;
    return result;
}

void InitNotificationMessage(String string, Arena *circularNotificationBuffer)
{
    G_NOTIFICATION_MESSAGE = ReallocString(string, circularNotificationBuffer);
    G_NOTIFICATION_ALPHA = 1.0;
}

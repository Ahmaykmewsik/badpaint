#pragma once

#include "input.h"
#include "main.h"

bool IsCommandDown(CommandInput *commandInputs, COMMAND command)
{
    bool result = commandInputs[command].down;
    return result;
}

bool IsCommandPressed(CommandInput *commandInputs, COMMAND command)
{
    bool result = commandInputs[command].pressed;
    return result;
}

void InitNotificationMessage(String string, Arena *circularNotificationBuffer)
{
    G_NOTIFICATION_MESSAGE = ReallocString(string, circularNotificationBuffer);
    G_NOTIFICATION_ALPHA = 1.0;
}

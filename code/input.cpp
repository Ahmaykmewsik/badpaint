#pragma once

#include "input.h"
#include "main.h"

static NotificationMessage notificationMessage = {}; 

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
    notificationMessage.string = ReallocString(string, circularNotificationBuffer);
	notificationMessage.alpha = 1.0f;
}

NotificationMessage *GetNotificationMessage()
{
	return &notificationMessage;
}

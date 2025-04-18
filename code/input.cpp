#pragma once

#include "input.h"
#include "main.h"

static NotificationMessage notificationMessage = {}; 

void InitNotificationMessage(String string, Arena *circularNotificationBuffer)
{
    notificationMessage.string = ReallocString(string, circularNotificationBuffer);
	notificationMessage.alpha = 1.0f;
}

NotificationMessage *GetNotificationMessage()
{
	return &notificationMessage;
}

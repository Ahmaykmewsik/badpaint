#pragma once

#include <base.h>

enum SLIDER_ACTION
{
    SLIDER_ACTION_NULL,
    SLIDER_ACTION_BRUSH_SIZE,
};

struct Slider
{
    SLIDER_ACTION sliderAction;
    unsigned int *unsignedIntToChange;
    float min;
    float max;
};

struct CommandInput
{
	b32 down;
	b32 pressed;
};

//TODO: (Ahmayk) Replace this stupid shit once we have better UI
struct NotificationMessage
{
	String string;
	f32 alpha;
};

void InitNotificationMessage(String string, Arena *circularNotificationBuffer);
NotificationMessage *GetNotificationMessage();

#pragma once

#include <base.h>
#include "../includes/raylib/src/raylib.h"

enum COMMAND : u32
{
    COMMAND_NULL,
    COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE,
    COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE,
    COMMAND_SWITCH_BRUSH_EFFECT_TO_MAX,
    COMMAND_SWITCH_BRUSH_EFFECT_TO_SHIFT,
    COMMAND_SWITCH_BRUSH_EFFECT_TO_RANDOM,
    COMMAND_EXPORT_IMAGE,
    COMMAND_COUNT,
};

//NOTE: (Ahmayk) This needs to be redesigned to include key modifiers 
static KeyboardKey COMMAND_KEY_BINDINGS[] = {
	KEY_NULL,
	KEY_E,
	KEY_R,
	KEY_A,
	KEY_S,
	KEY_N,
	KEY_NULL,
};

enum BRUSH_EFFECT : u32
{
    BRUSH_EFFECT_ERASE = 0,
    BRUSH_EFFECT_REMOVE = 1,
    BRUSH_EFFECT_MAX = 2,
    BRUSH_EFFECT_SHIFT = 3,
    BRUSH_EFFECT_RANDOM = 4,
};

static Color BRUSH_EFFECT_COLORS_PRIMARY[] =
{
	{ 0, 0, 0, 0},
	{ 230, 41, 55, 187 }, // RED
	{ 253, 249, 0, 187 }, // YELLOW
	{ 0, 121, 241, 187 }, //BLUE
	{ 200, 122, 255, 187 }, //PURPLE
};
static Color BRUSH_EFFECT_COLORS_PROCESSING[] =
{
	{ 255, 255, 255, 127},
	{ 230, 41, 55, 127 }, // RED
	{ 253, 249, 0, 127 }, // YELLOW
	{ 0, 121, 241, 127 }, //BLUE
	{ 200, 122, 255, 127 }, //PURPLE
};

struct Brush
{
    BRUSH_EFFECT brushEffect;
    u32 size;
};

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

bool IsCommandDown(CommandInput *commandInputs, COMMAND command);
bool IsCommandPressed(CommandInput *commandInputs, COMMAND command);
void InitNotificationMessage(String string, Arena *circularNotificationBuffer);
NotificationMessage *GetNotificationMessage();

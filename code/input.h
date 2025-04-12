#pragma once

#if __clang__
#include "headersNondependent.h"
#endif

enum COMMAND
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

struct CommandState
{
    KeyboardKey key;
    bool down;
    bool pressed;
    bool sentValue;
    float value;
};

static CommandState G_COMMAND_STATES[COMMAND_COUNT];

enum BRUSH_EFFECT : u32
{
    BRUSH_EFFECT_ERASE = 0,
    BRUSH_EFFECT_REMOVE = 1,
    BRUSH_EFFECT_MAX = 2,
    BRUSH_EFFECT_SHIFT = 3,
    BRUSH_EFFECT_RANDOM = 4,
};

static Color G_BRUSH_EFFECT_COLORS_PRIMARY[] =
{
	{ 0, 0, 0, 0},
	{ 230, 41, 55, 187 }, // RED
	{ 253, 249, 0, 187 }, // YELLOW
	{ 0, 121, 241, 187 }, //BLUE
	{ 200, 122, 255, 187 }, //PURPLE
};
static Color G_BRUSH_EFFECT_COLORS_PROCESSING[] =
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

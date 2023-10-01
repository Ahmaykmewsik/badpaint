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

enum BRUSH_EFFECT
{
    BRUSH_EFFECT_NULL,
    BRUSH_EFFECT_ERASE_EFFECT,
    BRUSH_EFFECT_REMOVE,
    BRUSH_EFFECT_MAX,
    BRUSH_EFFECT_SHIFT,
    BRUSH_EFFECT_RANDOM,
};

static Color G_BRUSH_EFFECT_COLORS[] = {BLANK, BLANK, RED, YELLOW, BLUE, PURPLE};

struct Brush
{
    BRUSH_EFFECT brushEffect;
    unsigned int size;
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
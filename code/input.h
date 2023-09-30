#pragma once

#if __clang__
#include "headersNondependent.h"
#endif

enum COMMAND
{
    COMMAND_NULL,
    COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE,
    COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE,
    COMMAND_EXPORT_IMAGE,
    COMMAND_COUNT,
};

#define MAX_KEYS_FOR_INPUT_BINDING 3

struct CommandState
{
    KeyboardKey key[MAX_KEYS_FOR_INPUT_BINDING];
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
};

static Color G_BRUSH_EFFECT_COLORS[] = {BLANK, BLANK, RED};

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
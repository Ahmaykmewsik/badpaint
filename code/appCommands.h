#pragma once

#include <base.h>

enum COMMAND : u32
{
    COMMAND_NULL,
    COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE,
    COMMAND_SWITCH_BRUSH_EFFECT_TO_MAX,
    COMMAND_SWITCH_BRUSH_EFFECT_TO_SHIFT,
    COMMAND_SWITCH_BRUSH_EFFECT_TO_RANDOM,
    COMMAND_SWITCH_TOOL_TO_PENCIL,
    COMMAND_SWITCH_TOOL_TO_ERASER,
    COMMAND_EXPORT_IMAGE,
    COMMAND_PAINT_ON_CANVAS_BETWEEN_POSITIONS,
    COMMAND_COUNT,
};

struct AppCommand
{
	COMMAND command;
	v2 value1V2;
	v2 value2V2;
};

struct AppCommandBuffer
{
	AppCommand *appCommands;
	u32 count;
	u32 size;
	ArenaMarker arenaMarker;
};

b32 IsCommandKeyBindingDown(COMMAND command);
b32 IsCommandKeyBindingPressed(COMMAND command);
AppCommand *PushAppCommand(AppCommandBuffer *appCommandBuffer);

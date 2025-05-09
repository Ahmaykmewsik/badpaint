#pragma once

#include <base.h>

enum COMMAND : u32
{
    COMMAND_NULL,
    COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_REMOVE,
    COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_MAX,
    COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_SHIFT,
    COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_RANDOM,
    COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_COPY_OTHER_PIXEL,
    COMMAND_SWITCH_TOOL_TO_PENCIL,
    COMMAND_SWITCH_TOOL_TO_ERASER,
    COMMAND_SWITCH_TOOL_TO_SPAYCAN,
    COMMAND_SWITCH_TOOL_TO_TEST,
    COMMAND_EXPORT_IMAGE,
    COMMAND_PAINT_ON_CANVAS_BETWEEN_POSITIONS,
    COMMAND_COUNT,
};

struct AppCommand
{
	COMMAND command;
	v2 value1V2;
	v2 value2V2;
	u32 value3U32;
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

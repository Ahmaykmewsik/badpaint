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
    COMMAND_IMPORT_FILE,
    COMMAND_PAINT_ON_CANVAS_BETWEEN_POSITIONS,
    COMMAND_COUNT,
};

struct AppCommand
{
	u32 command;
	union
	{
		v2 value1V2;
		String value1String;
	};
	union
	{
		v2 value2V2;
	};
	union
	{
		u32 value3U32;
	};
};

struct AppCommandBuffer
{
	AppCommand *appCommands;
	u32 count;
	u32 size;
	ArenaMarker arenaMarker;
};

AppCommand *PushAppCommand(AppCommandBuffer *appCommandBuffer);
b32 IsCommandKeyBindingDown(COMMAND command);
b32 IsCommandKeyBindingPressed(COMMAND command);

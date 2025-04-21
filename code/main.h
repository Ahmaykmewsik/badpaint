#pragma once

#include <base.h>
#include <ui/ui_core.h>
#include "../includes/raylib/src/raylib.h"
#include "image.h"
#include "platform_main.h"

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
    COMMAND_COUNT,
};

struct AppCommand
{
	COMMAND command;
};

struct AppCommandBuffer
{
	AppCommand *appCommands;
	u32 count;
	u32 size;
	ArenaMarker arenaMarker;
};

enum BADPAINT_TOOL_TYPE
{
	BADPAINT_TOOL_PENCIL,
	BADPAINT_TOOL_ERASER,
	BADPAINT_TOOL_COUNT,
};

struct Tool
{
	UiTextureView uiTextureViews[7]; //INTERACTION_STATE_COUNT
};

struct AppState
{
	UiFont defaultUiFont;

	Tool tools[BADPAINT_TOOL_COUNT];
    BADPAINT_BRUSH_EFFECT currentBrushEffect;
	BADPAINT_TOOL_TYPE currentTool;
	u32 toolSize;
	Texture loadedTexture;
	Canvas canvas;
};

b32 IsCommandKeyBindingDown(COMMAND command);
b32 IsCommandKeyBindingPressed(COMMAND command);

AppCommand *PushAppCommand(AppCommandBuffer *appCommandBuffer);

//TODO: (Ahmayk) Replace this stupid shit once we have better UI
struct NotificationMessage
{
	String string;
	f32 alpha;
};

void InitNotificationMessage(String string, Arena *circularNotificationBuffer);

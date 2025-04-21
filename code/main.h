#pragma once 

#include <base.h>
#include <ui/ui_core.h>
#include "../includes/raylib/src/raylib.h"
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

enum BADPAINT_BRUSH_EFFECT : u32
{
    BADPAINT_BRUSH_EFFECT_REMOVE = 1,
    BADPAINT_BRUSH_EFFECT_MAX = 2,
    BADPAINT_BRUSH_EFFECT_SHIFT = 3,
    BADPAINT_BRUSH_EFFECT_RANDOM = 4,
};

enum BADPAINT_TOOL_TYPE
{
	BADPAINT_TOOL_PENCIL,
	BADPAINT_TOOL_ERASER,
	BADPAINT_TOOL_COUNT,
};

static ColorU32 BRUSH_EFFECT_COLORS_PRIMARY[] =
{
	{},
	{ 230, 41, 55, 187 }, // RED
	{ 230, 41, 55, 187 }, // RED
	{ 253, 249, 0, 187 }, // YELLOW
	{ 0, 121, 241, 187 }, //BLUE
	{ 200, 122, 255, 187 }, //PURPLE
};
static ColorU32 BRUSH_EFFECT_COLORS_PROCESSING[] =
{
	{},
	{ 230, 41, 55, 127 }, // RED
	{ 253, 249, 0, 127 }, // YELLOW
	{ 0, 121, 241, 127 }, //BLUE
	{ 200, 122, 255, 127 }, //PURPLE
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

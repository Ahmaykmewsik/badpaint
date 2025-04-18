#pragma once 

#include <base.h>
#include <ui/ui_core.h>

#define VERSION_NUMBER "v0.0.3"

static String G_NOTIFICATION_MESSAGE = {};
static float G_NOTIFICATION_ALPHA = 0.0f;

//NOTE: (Ahmayk) Leftovers from vn_intrinsic.h
struct GameMemory
{
	Arena permanentArena;
	Arena temporaryArena;
	Arena mouseClickArena;
	Arena circularNotificationBuffer;
	Arena twoFrameArenaModIndex0;
	Arena twoFrameArenaModIndex1;

	Arena rootImageArena;
	Arena canvasArena;
	ArenaGroup conversionArenaGroup;
	Arena canvasRollbackArena;
};

static const char *buildDate = __DATE__;
static const char *buildTime = __TIME__;

struct PlatformWorkQueue;
void RunApp(PlatformWorkQueue *platformWorkQueue, GameMemory gameMemory, unsigned int threadCount);

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

enum BRUSH_EFFECT : u32
{
    BRUSH_EFFECT_ERASE = 0,
    BRUSH_EFFECT_REMOVE = 1,
    BRUSH_EFFECT_MAX = 2,
    BRUSH_EFFECT_SHIFT = 3,
    BRUSH_EFFECT_RANDOM = 4,
};

static ColorU32 BRUSH_EFFECT_COLORS_PRIMARY[] =
{
	{ 0, 0, 0, 0},
	{ 230, 41, 55, 187 }, // RED
	{ 253, 249, 0, 187 }, // YELLOW
	{ 0, 121, 241, 187 }, //BLUE
	{ 200, 122, 255, 187 }, //PURPLE
};
static ColorU32 BRUSH_EFFECT_COLORS_PROCESSING[] =
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

struct AppState
{
	Brush currentBrush;
	UiFont defaultUiFont;
};

b32 IsCommandKeyBindingDown(COMMAND command);
b32 IsCommandKeyBindingPressed(COMMAND command);

AppCommand *PushAppCommand(AppCommandBuffer *appCommandBuffer);

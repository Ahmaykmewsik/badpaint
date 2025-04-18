#pragma once 

#include <base.h>

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

AppCommand *PushAppCommand(AppCommandBuffer *appCommandBuffer);

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

#pragma once

#include <base.h>

#define VERSION_NUMBER "v0.0.3"

//NOTE: (Ahmayk) Leftovers from vn_intrinsic.h
struct GameMemory
{
	Arena permanentArena;
	Arena temporaryArena;
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
void RunApp(PlatformWorkQueue *threadWorkQueue, GameMemory *gameMemory, unsigned int threadCount);

#pragma once
#include <macros.h>
#include <base/memory.h>
#include <base/vn_math.h>
#include <base/vn_string.h>
#include <base/multithreading.h>

#include <stdio.h>
#include <stdint.h>
#include <cstdlib> //for malloc
#include <cstring> //for memcpy
#include <math.h>

//NOTE: (Ahmayk) hack to allow raw openGL calls in our code 
#include "../includes/raylib/src/external/glad.h" // GLAD extensions loading library, includes OpenGL headers

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

static unsigned int G_CURRENT_FRAME = {};

inline int GetFrameModIndexThisFrame()
{
	int result = G_CURRENT_FRAME % 2;
	return result;
}

inline int GetFrameModIndexLastFrame()
{
	int result = {};

	if (G_CURRENT_FRAME)
		result = (G_CURRENT_FRAME - 1) % 2;

	return result;
}

inline Arena *GetTwoFrameArenaThisFrame(GameMemory *gameMemory)
{
	Arena *result = (GetFrameModIndexThisFrame())
		? &gameMemory->twoFrameArenaModIndex0
		: &gameMemory->twoFrameArenaModIndex1;

	return result;
}

inline Arena *GetTwoFrameArenaLastFrame(GameMemory *gameMemory)
{
	Arena *result = (GetFrameModIndexLastFrame())
		? &gameMemory->twoFrameArenaModIndex1
		: &gameMemory->twoFrameArenaModIndex0;

	return result;
}

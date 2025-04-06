
#include <base/macros.h>
#include <base/memory.h>

//TODO: (Marc) If OS_WINDOWS
#if OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

Arena ArenaInit(u64 size)
{
	Arena result = {};
	//TODO: (Marc) Control for reserve or commit if you need it
#if OS_WINDOWS
    result.memory = (u8*) VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
#elif OS_WEB
    result.memory = (u8*) malloc(size);
#endif

    result.size = size;
	return result;
}

ArenaMarker ArenaPushMarker(Arena *arena)
{
	ArenaMarker result;
	result.arena = arena;
	result.used = arena->used;
	return result;
}

#define AlignPow2(value, alignment) *value = ((*value + ((alignment) - 1)) & ~((alignment) - 1))
#define Align4(value) AlignPow2(value, 4)
#define Align8(value) AlignPow2(value, 8)
#define Align16(value) AlignPow2(value, 16)

//TODO: (Marc) alignment input?
//TODO: (Marc) snap sizes to page size?
//TODO: (Marc) Dummy arena that pushes the stack on fail
u8 *ArenaPushSize(Arena *arena, u64 size, ArenaMarker *arenaMarker)
{
	u8 *result = {};
	ASSERT(arena);
	ASSERT(arena->memory);
	if (arenaMarker)
	{
		*arenaMarker = ArenaPushMarker(arena);
	}
	if (size)
	{
		// NOTE: We assume that the base value is already aligned
		Align8(&arena->used);

		if (arena->circular && arena->used + size >= arena->size)
		{
			arena->used = 0;
		}

		if (arena->used + size >= arena->size)
		{
			//TODO: (Marc) What to do when our arena runs out? Reallocate?
			InvalidCodePath;
		}

		result = arena->used + arena->memory;
    	arena->used += size;
	}

	return result;
}

Arena ArenaInitFromArena(Arena *inputArena, u64 size)
{
	Arena result = {};
	result.memory = ArenaPushSize(inputArena, size, {});
	result.size = size;
	return result;
}

void ArenaPopMarker(ArenaMarker arenaMarker)
{
	if (arenaMarker.arena)
	{
		arenaMarker.arena->used = arenaMarker.used;
	}
}

void ArenaReset(Arena *arena)
{
	arena->used = 0;
}

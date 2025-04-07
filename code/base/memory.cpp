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
	if (ASSERT(arena) && ASSERT(!arena->readyForAssignment) && ASSERT(arena->memory))
	{
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

u64 GetPageSize()
{
	u64 result = 0;
#if OS_WINDOWS
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    result = systemInfo.dwPageSize;
#endif
	return result;
}

ArenaGroup ArenaGroupInit(u64 size)
{
	ArenaGroup result = {};
	result.masterArena = ArenaInit(size);
	return result;
}

u64 RoundUpToMultipleU64(u64 n, u32 multiple)
{
	u64 result = n;
	u32 remainder = n % multiple;
	if (remainder > 0)
	{
		result = (result - remainder) + multiple;
	}
	return result;
}

void FillArenaGroup(ArenaGroup *arenaGroup, u32 blockSize)
{
	if (ASSERT(arenaGroup->masterArena.size))
	{
		arenaGroup->count = 0;
		arenaGroup->masterArena.used = 0;
		u64 pageSize = GetPageSize();
		u64 blockSizeRounded = RoundUpToMultipleU64(blockSize, pageSize);
		u32 count = Floor(SafeDivide(arenaGroup->masterArena.size, blockSizeRounded));
		arenaGroup->arenas = (Arena*) ARENA_PUSH_ARRAY(&arenaGroup->masterArena, count, Arena*);
		arenaGroup->masterArena.used = RoundUpToMultipleU64(arenaGroup->masterArena.used, pageSize);
		while (arenaGroup->masterArena.used + blockSizeRounded <= arenaGroup->masterArena.size)
		{
			Arena *arena = &arenaGroup->arenas[arenaGroup->count++];
			*arena = ArenaInitFromArena(&arenaGroup->masterArena, blockSizeRounded);
			arena->readyForAssignment = true;
		}
	}
}

ArenaPair ArenaPairAssign(ArenaGroup *arenaGroup)
{
	ArenaPair result = {};
	for (u32 i = 0; i < arenaGroup->count; i++)
	{
		if (arenaGroup->arenas[i].readyForAssignment)
		{
			if (!result.arena1)
			{
				result.arena1 = &arenaGroup->arenas[i];
			}
			else if (!result.arena2)
			{
				result.arena2 = &arenaGroup->arenas[i];
				break;
			}
		}
	}

	if (ASSERT(result.arena1 && result.arena2))
	{
		result.arena1->readyForAssignment = false;
		result.arena2->readyForAssignment = false;
	}
	else
	{
		result = {};
	}

	return result;
}

Arena *ArenaPairPushOldest(ArenaPair *alternatingAreans, Arena *finishedArena)
{
	Arena *result = {};

	ASSERT(alternatingAreans->arena1);
	ASSERT(alternatingAreans->arena2);

	if (finishedArena)
	{
		ASSERT(alternatingAreans->previouslyPoppedArena != finishedArena);
		if (alternatingAreans->arena1 == finishedArena)
		{
			result = alternatingAreans->arena1;
		}
		if (alternatingAreans->arena2 == finishedArena)
		{
			result = alternatingAreans->arena2;
		}
	}
	else
	{
		if (!alternatingAreans->arena1->used)
		{
			result = alternatingAreans->arena1;
		}
		if (!alternatingAreans->arena2->used)
		{
			result = alternatingAreans->arena2;
		}
	}

	if (ASSERT(result))
	{
		result->used = 0;
		alternatingAreans->previouslyPoppedArena = result;
		ASSERT(!result->readyForAssignment);
	}
	else
	{
		//TODO: (Ahmayk) Log that we fucked up
		result = alternatingAreans->arena1;
	}

	return result;
}

void ArenaPairFreeOldest(ArenaPair *arenaPair)
{
	if (arenaPair->previouslyPoppedArena)
	{
		if (arenaPair->arena1 == arenaPair->previouslyPoppedArena && arenaPair->arena2)
		{
			arenaPair->arena2->used = 0;
			arenaPair->arena2->readyForAssignment = true;
			arenaPair->arena2 = {};
		}
		if (arenaPair->arena2 == arenaPair->previouslyPoppedArena && arenaPair->arena1)
		{
			arenaPair->arena1->used = 0;
			arenaPair->arena1->readyForAssignment = true;
			arenaPair->arena1 = {};
		}
	}
}

void ArenaPairFreeAll(ArenaPair *arenaPair)
{
	if (arenaPair->arena1)
	{
		arenaPair->arena1->used = 0;
		arenaPair->arena1->readyForAssignment = true;
	}
	if (arenaPair->arena2)
	{
		arenaPair->arena2->used = 0;
		arenaPair->arena2->readyForAssignment = true;
	}
	*arenaPair = {};
}

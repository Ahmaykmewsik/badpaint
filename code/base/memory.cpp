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
#else
	InvalidCodePath
#endif

	ASSERT(result.memory);

    result.size = size;
	return result;
}

u64 GetPageSize()
{
	u64 result = 0;
#if OS_WINDOWS
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    result = systemInfo.dwPageSize;
#else
	InvalidCodePath
#endif
	return result;
}

ArenaMarker ArenaPushMarker(Arena *arena)
{
	ArenaMarker result;
	result.arena = arena;
	result.used = arena->used;
	return result;
}

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
			ALIGN_POW2(&arena->used, 8);

			if (arena->circular && arena->used + size >= arena->size)
			{
				arena->used = 0;
			}

			if (arena->used + size >= arena->size)
			{
				//TODO: (Marc) What to do when our arena runs out? Reallocate?
				//Force crash for now, we don't want to continue!
#if OS_WINDOWS
				__debugbreak();
#endif
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

ArenaGroup ArenaGroupInit(u64 size)
{
	ArenaGroup result = {};
	result.masterArena = ArenaInit(size);
	return result;
}

void FillArenaGroup(ArenaGroup *arenaGroup, u32 blockSize)
{
	if (ASSERT(arenaGroup->masterArena.size))
	{
		arenaGroup->count = 0;
		arenaGroup->masterArena.used = 0;
		u64 pageSize = GetPageSize();
		ALIGN_POW2(&blockSize, pageSize);

		u32 count = Floor(SafeDivide(arenaGroup->masterArena.size, blockSize));
		arenaGroup->arenas = (Arena*) ARENA_PUSH_ARRAY(&arenaGroup->masterArena, count, Arena*);

		u64 usedRounded = arenaGroup->masterArena.used;
		ALIGN_POW2(&usedRounded, pageSize);
		if (usedRounded <= arenaGroup->masterArena.size)
		{
			arenaGroup->masterArena.used = usedRounded;
		}

		while (arenaGroup->masterArena.used + blockSize <= arenaGroup->masterArena.size)
		{
			Arena *arena = &arenaGroup->arenas[arenaGroup->count++];
			*arena = ArenaInitFromArena(&arenaGroup->masterArena, blockSize);
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
		ASSERT(alternatingAreans->lastPushedArena != finishedArena);
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
		alternatingAreans->lastPushedArena = result;
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
	if (arenaPair->lastPushedArena)
	{
		if (arenaPair->arena1 == arenaPair->lastPushedArena && arenaPair->arena2)
		{
			arenaPair->arena2->used = 0;
			arenaPair->arena2->readyForAssignment = true;
			arenaPair->arena2 = {};
		}
		if (arenaPair->arena2 == arenaPair->lastPushedArena && arenaPair->arena1)
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

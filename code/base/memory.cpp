#include <base/macros.h>
#include <base/memory.h>

//TODO: (Marc) If OS_WINDOWS
#if OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

u64 GetPageSize()
{
	u64 result = 0;
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	result = systemInfo.dwPageSize;
	return result;
}

#if DEBUG_MODE
void MemoryProtectWrite(void *memory, u64 size)
{
	if (size)
	{
		DWORD oldProtect;
		ASSERT(VirtualProtect(memory, size, PAGE_READONLY, &oldProtect));
	}
}

void MemoryProtectReadWrite(void *memory, u64 size)
{
	if (size)
	{
		DWORD oldProtect;
		ASSERT(VirtualProtect(memory, size, PAGE_NOACCESS, &oldProtect));
	}
}

void MemoryUnprotect(void *memory, u64 size)
{
	if (size)
	{
		DWORD oldProtect;
		ASSERT(VirtualProtect(memory, size, PAGE_READWRITE, &oldProtect));
	}
}
#endif

Arena ArenaInit(u64 size)
{
#if DEBUG_MODE
	size += GetPageSize();
#endif

	Arena result = {};
	//TODO: (Marc) Control for reserve or commit if you need it
	result.memory = (u8*) VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);

	if (ASSERT(result.memory))
	{
		//TODO: (Marc) What do we do when we fail?
	}
	result.size = size;

#if DEBUG_MODE
	result.size -= GetPageSize();
	MemoryProtectReadWrite(result.memory + result.size, GetPageSize());
#endif

	return result;
}
#endif

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
			ALIGN_POW2_LIMIT(&arena->used, 16, arena->size);

			if (arena->circular && arena->used + size > arena->size)
			{
				arena->used = 0;
			}

			if (arena->used + size > arena->size)
			{
				//TODO: (Marc) What to do when our arena runs out? Reallocate?
				//Force crash for now, we don't want to continue!
#if OS_WINDOWS
				__debugbreak();
#endif
			}

			result = arena->used + arena->memory;
			arena->used += size;

#if DEBUG_MODE
			u64 pageSize = GetPageSize();
			ALIGN_POW2_LIMIT(&arena->used, pageSize, arena->size);
			if (arena->used + pageSize < arena->size)
			{
				MemoryProtectReadWrite(arena->memory + arena->used, pageSize);
				arena->used += pageSize;
			}
#endif
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
#if DEBUG_MODE
		ASSERT(arenaMarker.used <= arenaMarker.arena->used);
		MemoryUnprotect(arenaMarker.arena->memory + arenaMarker.used, arenaMarker.arena->used - arenaMarker.used);
#endif
		arenaMarker.arena->used = arenaMarker.used;
	}
}

void ArenaReset(Arena *arena)
{
	arena->used = 0;
#if DEBUG_MODE
	MemoryUnprotect(arena->memory, arena->size);
#endif
}

ArenaGroup ArenaGroupInit(u64 size)
{
	ArenaGroup result = {};
	result.masterArena = ArenaInit(size);
	return result;
}

void ArenaGroupFill(ArenaGroup *arenaGroup, u32 blockSize)
{
	if (ASSERT(arenaGroup->masterArena.size))
	{
		arenaGroup->count = 0;
		ArenaReset(&arenaGroup->masterArena);
		u64 pageSize = GetPageSize();
		ALIGN_POW2(&blockSize, pageSize);

		u32 count = Floor(SafeDivide(arenaGroup->masterArena.size, blockSize));
		arenaGroup->arenas = ARENA_PUSH_ARRAY(&arenaGroup->masterArena, count, Arena);

		ALIGN_POW2_LIMIT(&arenaGroup->masterArena.used, pageSize, arenaGroup->masterArena.size);
		while (arenaGroup->masterArena.used + blockSize + 16 <= arenaGroup->masterArena.size)
		{
			Arena *arena = &arenaGroup->arenas[arenaGroup->count++];
			*arena = ArenaInitFromArena(&arenaGroup->masterArena, blockSize);
			arena->readyForAssignment = true;
		}
	}
}

Arena *ArenaGroupPushArena(ArenaGroup *arenaGroup)
{
	Arena *result = {};
	for (u32 i = 0; i < arenaGroup->count; i++)
	{
		if (arenaGroup->arenas[i].readyForAssignment)
		{
			result = &arenaGroup->arenas[i];
			result->readyForAssignment = false;
			break;
		}
	}

	ASSERT(result);
	ASSERT(result->used == 0);

	return result;
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
		ASSERT(result.arena1->used == 0);
		ASSERT(result.arena2->used == 0);
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
		alternatingAreans->lastPushedArena = result;
		ASSERT(!result->readyForAssignment);
	}
	else
	{
		//TODO: (Ahmayk) Log that we fucked up
		result = alternatingAreans->arena1;
	}
	ArenaReset(result);

	return result;
}

void ArenaPairFreeOldest(ArenaPair *arenaPair)
{
	if (arenaPair->lastPushedArena)
	{
		if (arenaPair->arena1 == arenaPair->lastPushedArena && arenaPair->arena2)
		{
			ArenaReset(arenaPair->arena2);
			arenaPair->arena2->readyForAssignment = true;
			arenaPair->arena2 = {};
		}
		if (arenaPair->arena2 == arenaPair->lastPushedArena && arenaPair->arena1)
		{
			ArenaReset(arenaPair->arena1);
			arenaPair->arena1->readyForAssignment = true;
			arenaPair->arena1 = {};
		}
	}
}

void ArenaPairFreeAll(ArenaPair *arenaPair)
{
	if (arenaPair->arena1)
	{
		ArenaReset(arenaPair->arena1);
		arenaPair->arena1->readyForAssignment = true;
	}
	if (arenaPair->arena2)
	{
		ArenaReset(arenaPair->arena2);
		arenaPair->arena2->readyForAssignment = true;
	}
	*arenaPair = {};
}

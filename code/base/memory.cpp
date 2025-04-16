#include <base/macros.h>
#include <base/memory.h>
#include <base/vn_math.h>

void AlignPow2U32(u32 *value, u32 alignment)
{
	*value = ((*value + ((alignment) - 1)) & ~((alignment) - 1));
}

void AlignPow2U64(u64 *value, u64 alignment)
{
	*value = ((*value + ((alignment) - 1)) & ~((alignment) - 1));
}

void AlignPow2LimitU64(u64 *value, u64 alignment, u64 limit)
{
	if ((*value + alignment) < limit)
	{
		AlignPow2U64(value, alignment);
	}
}

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
	Arena result = {};

	u64 pageSize = GetPageSize();
	AlignPow2U64(&size, pageSize);

#if DEBUG_MODE
	size += pageSize;
#endif

	//TODO: (Marc) Control for reserve or commit if you need it
	result.memory = (u8*) VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);

	if (ASSERT(result.memory))
	{
		result.size = size;
		result.flags |= ARENA_FLAG_IS_BASE;
#if DEBUG_MODE
		result.size -= pageSize;
		MemoryProtectReadWrite(result.memory + result.size, pageSize);
#endif
	}

	return result;
}

void ArenaFree(Arena *arena)
{
	if (arena->memory && ASSERT(arena->flags & ARENA_FLAG_IS_BASE))
	{
		ASSERT(VirtualFree(arena->memory, 0, MEM_RELEASE));
		*arena = {};
	}
}

#else
#error Platform not implemented in memory.cpp
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
	if (ASSERT(arena) &&
		ASSERT(size) &&
		ASSERT(arena->memory) &&
		ASSERT(!(arena->flags & ARENA_FLAG_READY_FOR_ASSIGNMENT)))
	{
		if (arenaMarker)
		{
			*arenaMarker = ArenaPushMarker(arena);
		}
		//NOTE: (Marc) Assume that something has gone wrong if we try to commit more than a gigabyte
		//And the arena wasn't initialized at that size
		//(can happen if an unsigned int underflows due to faulty math)
		if (size && ASSERT(size < GigaByte || (arena->size >= size)))
		{
			AlignPow2LimitU64(&arena->used, 16, arena->size);

			if ((arena->flags & ARENA_FLAG_CIRCULAR) && arena->used + size > arena->size)
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
			AlignPow2LimitU64(&arena->used, pageSize, arena->size);
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

void ArenaGroupReset(ArenaGroup *arenaGroup)
{
	ArenaReset(&arenaGroup->masterArena);
	arenaGroup->arenas = {};
	arenaGroup->count = 0;
}

void ArenaGroupFree(ArenaGroup *arenaGroup)
{
	ArenaFree(&arenaGroup->masterArena);
	*arenaGroup = {};
}

void ArenaGroupResetAndFill(ArenaGroup *arenaGroup, u32 blockSize)
{
	if (ASSERT(arenaGroup->masterArena.memory))
	{
		ArenaGroupReset(arenaGroup);
		u64 pageSize = GetPageSize();
		AlignPow2U32(&blockSize, (u32) pageSize);

		if (blockSize > 0)
		{
			u32 count = (u32) FloorF32((f32)(arenaGroup->masterArena.size / blockSize));
			arenaGroup->arenas = ARENA_PUSH_ARRAY(&arenaGroup->masterArena, count, Arena);

			AlignPow2LimitU64(&arenaGroup->masterArena.used, pageSize, arenaGroup->masterArena.size);
			while (arenaGroup->masterArena.used + blockSize + 16 <= arenaGroup->masterArena.size)
			{
				Arena *arena = &arenaGroup->arenas[arenaGroup->count++];
				*arena = ArenaInitFromArena(&arenaGroup->masterArena, blockSize);
				arena->flags |= ARENA_FLAG_READY_FOR_ASSIGNMENT;
			}
		}
	}
}

Arena *ArenaGroupPushArena(ArenaGroup *arenaGroup)
{
	Arena *result = {};
	for (u32 i = 0; i < arenaGroup->count; i++)
	{
		if (arenaGroup->arenas[i].flags & ARENA_FLAG_READY_FOR_ASSIGNMENT)
		{
			result = &arenaGroup->arenas[i];
			result->flags |= ARENA_FLAG_READY_FOR_ASSIGNMENT;
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
		if (arenaGroup->arenas[i].flags & ARENA_FLAG_READY_FOR_ASSIGNMENT)
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

	if (result.arena1 && result.arena2)
	{
		result.arena1->flags &= ~ARENA_FLAG_READY_FOR_ASSIGNMENT;
		result.arena2->flags &= ~ARENA_FLAG_READY_FOR_ASSIGNMENT;
		ASSERT(result.arena1->used == 0);
		ASSERT(result.arena2->used == 0);
	}
	else
	{
		//NOTE: (Ahmayk) pair of areans not free, so no luck
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
		ASSERT(!(result->flags & ARENA_FLAG_READY_FOR_ASSIGNMENT));
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
			arenaPair->arena2->flags |= ARENA_FLAG_READY_FOR_ASSIGNMENT;
			arenaPair->arena2 = {};
		}
		if (arenaPair->arena2 == arenaPair->lastPushedArena && arenaPair->arena1)
		{
			ArenaReset(arenaPair->arena1);
			arenaPair->arena2->flags |= ARENA_FLAG_READY_FOR_ASSIGNMENT;
			arenaPair->arena1 = {};
		}
	}
}

void ArenaPairFreeAll(ArenaPair *arenaPair)
{
	if (arenaPair->arena1)
	{
		ArenaReset(arenaPair->arena1);
		arenaPair->arena1->flags |= ARENA_FLAG_READY_FOR_ASSIGNMENT;
	}
	if (arenaPair->arena2)
	{
		ArenaReset(arenaPair->arena2);
		arenaPair->arena2->flags |= ARENA_FLAG_READY_FOR_ASSIGNMENT;
	}
	*arenaPair = {};
}

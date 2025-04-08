#pragma once

#include <base/macros.h>

struct Arena
{
	u8 *memory;
	u64 size;
	u64 used;
	b8 circular;
	b8 readyForAssignment;
};

struct ArenaMarker
{
	Arena *arena;
	u64 used;
};

Arena ArenaInit(u64 size);
ArenaMarker ArenaPushMarker(Arena *arena);
u8 *ArenaPushSize(Arena *arena, u64 size, ArenaMarker *arenaMarker);
Arena ArenaInitFromArena(Arena *inputArena, u64 size);
void ArenaPopMarker(ArenaMarker arenaMarker);
void ArenaReset(Arena *arena);

#define ALIGN_POW2(value, alignment) *value = ((*value + ((alignment) - 1)) & ~((alignment) - 1))
#define ALIGN_POW2_LIMIT(value, alignment, limit) if ((*value + alignment) < limit) ALIGN_POW2(value, alignment)

#define ARENA_PUSH_STRUCT(arena, type) (type *)ArenaPushSize(arena, sizeof(type), {})
#define ARENA_PUSH_ARRAY(arena, count, type) (type *)ArenaPushSize(arena, count * sizeof(type), {})

#define ARENA_PUSH_STRUCT_MARKER(arena, type, marker) (type *)ArenaPushSize(arena, sizeof(type), marker)
#define ARENA_PUSH_ARRAY_MARKER(arena, count, type, marker) (type *)ArenaPushSize(arena, count * sizeof(type), marker)

struct ArenaGroup
{
	Arena masterArena;
	Arena *arenas;
	u32 count;
};

ArenaGroup ArenaGroupInit(u64 size);
void ArenaGroupFill(ArenaGroup *arenaGroup, u32 blockSize);
Arena *ArenaGroupPushArena(ArenaGroup *arenaGroup);

struct ArenaPair 
{
	Arena *arena1;
	Arena *arena2;
	Arena *lastPushedArena;
};

ArenaPair ArenaPairAssign(ArenaGroup *arenaGroup);
Arena *ArenaPairPushOldest(ArenaPair *alternatingAreans, Arena *finishedArena);
void ArenaPairFreeOldest(ArenaPair *alternatingAreans);
void ArenaPairFreeAll(ArenaPair *alternatingAreans);

#if DEBUG_MODE
void MemoryProtectReadWrite(void *memory, u64 size);
void MemoryProtectWrite(void *memory, u64 size);
void MemoryUnprotect(void *memory, u64 size);
#endif

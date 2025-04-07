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

void FillArenaGroup(ArenaGroup *arenaGroup, u32 blockSize);

struct ArenaPair 
{
	Arena *arena1;
	Arena *arena2;
	Arena *previouslyPoppedArena;
};

ArenaPair ArenaPairAssign(ArenaGroup *arenaGroup);
Arena *ArenaPairPushOldest(ArenaPair *alternatingAreans, Arena *finishedArena);
void ArenaPairFreeOldest(ArenaPair *alternatingAreans);
void ArenaPairFreeAll(ArenaPair *alternatingAreans);

#pragma once

#include <base/macros.h>

void AlignPow2U32(u32 *value, u32 alignment);
void AlignPow2U64(u64 *value, u64 alignment);
void AlignPow2LimitU64(u64 *value, u64 alignment, u64 limit);

#if DEBUG_MODE
void MemoryProtectReadWrite(void *memory, u64 size);
void MemoryProtectWrite(void *memory, u64 size);
void MemoryUnprotect(void *memory, u64 size);
#endif

enum ARENA_FLAGS : u32
{
	ARENA_FLAG_IS_BASE              = 1 << 0,
	ARENA_FLAG_CIRCULAR             = 1 << 1,
	ARENA_FLAG_READY_FOR_ASSIGNMENT = 1 << 2,
};

struct Arena
{
	u8 *memory;
	u64 size;
	u64 used;
	u32 flags;
};

Arena ArenaInit(u64 size);
void ArenaFree(Arena *arena); //NOTE: (Ahmayk) Can only free base arenas! Not arenas initialized from others
Arena ArenaInitFromArena(Arena *inputArena, u64 size);
void ArenaReset(Arena *arena);

struct ArenaMarker
{
	Arena *arena;
	u64 used;
};

#define ARENA_PUSH_STRUCT(arena, type) (type *)ArenaPushSize(arena, sizeof(type), {})
#define ARENA_PUSH_ARRAY(arena, count, type) (type *)ArenaPushSize(arena, count * sizeof(type), {})

#define ARENA_PUSH_STRUCT_MARKER(arena, type, marker) (type *)ArenaPushSize(arena, sizeof(type), marker)
#define ARENA_PUSH_ARRAY_MARKER(arena, count, type, marker) (type *)ArenaPushSize(arena, count * sizeof(type), marker)

ArenaMarker ArenaPushMarker(Arena *arena);
u8 *ArenaPushSize(Arena *arena, u64 size, ArenaMarker *arenaMarker);
void ArenaPopMarker(ArenaMarker arenaMarker);

struct ArenaGroup
{
	Arena masterArena;
	Arena *arenas;
	u32 count;
};

ArenaGroup ArenaGroupInit(u64 size);
void ArenaGroupFree(ArenaGroup *arenaGroup);
void ArenaGroupReset(ArenaGroup *arenaGroup);
void ArenaGroupResetAndFill(ArenaGroup *arenaGroup, u32 blockSize);
Arena *ArenaGroupPushArena(ArenaGroup *arenaGroup);
void ArenaResetAndMarkAsReadyForAssignment(Arena *arena);

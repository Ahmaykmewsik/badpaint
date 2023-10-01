#pragma once

#include <vcruntime_string.h>
#if __clang__
#pragma clang diagnostic ignored "-Wnull-dereference"
#pragma clang diagnostic ignored "-Wstring-plus-int"
#pragma clang diagnostic warning "-Wshadow"
#endif

#define _CRT_SECURE_NO_DEPRECATE

#include <stdint.h>
#include <cstdlib> //for malloc
#include <cstring> //for memcpy
#include <math.h>
#include <cstdio>

#define ArrayCount(Array) (int)(sizeof(Array) / sizeof((Array)[0]))

#define Assert(expression) \
    if (!(expression))     \
    {                      \
        *(int *)0 = 0;     \
    }

// if (!!(expression))
//     __debugbreak()

#define InvalidCodePath Assert(!"InvalidCodePath");
#define UnimplementedCodePath InvalidCodePath
#define InvalidDefaultCase \
    default:               \
    {                      \
        InvalidCodePath;   \
    }                      \
    break;

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)

#define Largest32Int 2147483647

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

#define CREATE_ENUM(xMacroList, name) \
    enum name                         \
    {                                 \
        xMacroList(GENERATE_ENUM)     \
    };

#define CREATE_STRINGS(xMacroList, name) static const char *name[] = {xMacroList(GENERATE_STRING)};
#define ADD_STRING(type) type##_STRINGS

#define CREATE_ENUM_AND_STRINGS(enumName) \
    CREATE_ENUM(X_##enumName, enumName)   \
    CREATE_STRINGS(X_##enumName, enumName##_STRINGS)

// TODO: Switch to this
#if 0
#define GENERATE_ENUM(enumName, enum) enumName##_##enum,
#define GENERATE_STRING(enumName, string) #string,

#define CREATE_ENUM_AND_STRINGS(enumName) \
    enum enumName                         \
    {                                     \
        X_##enumName(GENERATE_ENUM)       \
    };                                    \
    static const char *enumName##_STRINGS[] = {X_##enumName(GENERATE_STRING)};
#endif

#define CONSOLE_DEFAULT "\033[0m"
#define CONSOLE_BLACK "\033[0;30m"
#define CONSOLE_RED "\033[0;31m"
#define CONSOLE_GREEN "\033[0;32m"
#define CONSOLE_YELLOW "\033[0;33m"
#define CONSOLE_BLUE "\033[0;34m"
#define CONSOLE_PURPLE "\033[0;35m"
#define CONSOLE_CYAN "\033[0;36m"
#define CONSOLE_WHITE "\033[0;37m"

struct MemoryArena
{
    unsigned int size;
    unsigned int *base;
    unsigned int used;
    bool circular;
};

struct GameMemory
{
    MemoryArena permanentArena;
    MemoryArena temporaryArena;
    MemoryArena rootImageArena;
    MemoryArena canvasArena;
    MemoryArena mouseClickArena;
    MemoryArena circularScratchBuffer;
    MemoryArena canvasRollbackArena;
    MemoryArena latestCompletedImageArena;

    MemoryArena twoFrameArenaModIndex0;
    MemoryArena twoFrameArenaModIndex1;
};

#define ZeroArray(array) ZeroSize(sizeof(array), array)
#define ZeroArrayType(array, count, type) ZeroSize(TypeSize(count, type), array)
#define ZeroStruct(instance) ZeroSize(sizeof(instance), &(instance))
inline void ZeroSize(unsigned int size, void *ptr)
{
    memset(ptr, 0, size);
}

inline void InitializeArena(MemoryArena *Arena, unsigned int size, bool circular = false)
{
    // TODO: Having malloc in here forces every memory arena to have a seperate call to the os
    //  Is that a good idea? Consider refactoring so that there's just one call.
    unsigned int *buffer = (unsigned int *)malloc(size * sizeof *buffer);
    if (!buffer)
        InvalidCodePath

            Arena->size = size;
    Arena->base = buffer;
    Arena->used = 0;
    Arena->circular = circular;

    // ZeroSize(Arena->size, Arena->base);
}

// TODO: does this need to be any more complex?
//  Consider refactoring to be similar to HMH if you find it getting used a lot
inline void ResetMemoryArena(MemoryArena *arena)
{
    arena->used = 0;
}

inline void Align(unsigned int *input, unsigned int align)
{
    if (*input & (align - 1))
        *input += (align - (*input % align));
}

inline void Align8(unsigned int *input)
{
    Align(input, 8);
}

inline void Align16(unsigned int *input)
{
    Align(input, 16);
}

struct ArrayInfo
{
    unsigned int size;
    unsigned int count;
    unsigned int lastIndex;
};

#define TypeSize(count, type) (count * sizeof(type))

#define PushStruct(arena, type) (type *)PushSize_(arena, sizeof(type))
#define PushArray(arena, count, type) (type *)PushSize_(arena, TypeSize(count, type))
#define PushSize(arena, size) PushSize_(arena, size)

inline void *PushSize_(MemoryArena *arena, unsigned int size)
{
    Assert(size > 0);
    Assert(arena);

    if ((arena->used + size) >= arena->size)
    {
        if (!arena->circular)
            InvalidCodePath
                // TODO: proper DEBUG logging for when a circular buffer wraps around
                arena->used = 0;
    }

    // NOTE: Assumes that the base value is already aligned
    // Align(&arena->used, 8);

    Assert((arena->used + size) < arena->size);

    void *result = arena->base + arena->used;
    arena->used += size;

    return (result);
}

#define CopyArray(dest, source, destLastIndex, sourceCount, destCountSize, Type) \
    Assert(sourceCount < destCountSize);                                         \
    destLastIndex = sourceCount - 1;                                             \
    memcpy(dest, source, TypeSize(sourceCount, Type));

#define AllocateAndCopyArray(arena, dest, source, destCount, sourceCount, Type) \
    dest = PushArray(arena, sourceCount, Type);                                 \
    destCount = sourceCount;                                                    \
    memcpy(dest, source, TypeSize(sourceCount, Type));

#define GetNextInArray(result, array, arrayCount) \
    if (arrayCount < ArrayCount(array))           \
    {                                             \
        result = &##array[arrayCount];            \
        arrayCount++;                             \
    }                                             \
    else                                          \
    {                                             \
        InvalidCodePath                           \
    }

#define GetLowestNonActive(result, array, lastIndex, arraySize) \
    for (int i = 1;                                             \
         i < arraySize;                                         \
         i++)                                                   \
    {                                                           \
        if (!array[i].active)                                   \
        {                                                       \
            result = &array[i];                                 \
            result->index = i;                                  \
            if (i >= lastIndex)                                 \
                lastIndex = i;                                  \
            break;                                              \
        }                                                       \
    }                                                           \
                                                                \
    if (!result)                                                \
    {                                                           \
        InvalidCodePath                                         \
    }

typedef uint64_t Flags;
typedef uint64_t Key;

inline int FlagToBit(Flags flag)
{
    int result = 0;

    while (flag > 1)
    {
        flag >>= 1;
        result++;
    }

    return result;
}

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

inline MemoryArena *GetTwoFrameArenaThisFrame(GameMemory *gameMemory)
{
    MemoryArena *result = (GetFrameModIndexThisFrame())
                              ? &gameMemory->twoFrameArenaModIndex0
                              : &gameMemory->twoFrameArenaModIndex1;

    return result;
}

inline MemoryArena *GetTwoFrameArenaLastFrame(GameMemory *gameMemory)
{
    MemoryArena *result = (GetFrameModIndexLastFrame())
                              ? &gameMemory->twoFrameArenaModIndex1
                              : &gameMemory->twoFrameArenaModIndex0;

    return result;
}

static MemoryArena *_G_CIRCULAR_ARENA_DONT_FUCKING_USE_THIS_EXCEPT_IN_A_MACRO = {};

inline void *_TempALLOC(int size)
{
    Assert(_G_CIRCULAR_ARENA_DONT_FUCKING_USE_THIS_EXCEPT_IN_A_MACRO);
    void *result = PushSize(_G_CIRCULAR_ARENA_DONT_FUCKING_USE_THIS_EXCEPT_IN_A_MACRO, size);
    return result;
}

inline void *_TempREALLOC(void *p, int oldSize, int newSize)
{
    void *result = NULL;
    if (newSize != 0)
    {
        result = _TempALLOC(newSize);

        if (p != NULL)
        {
            if (oldSize <= newSize)
                memcpy(result, p, oldSize);
            else
                memcpy(result, p, newSize);
        }
    }
    return result;
}

inline void _TempFREE(void *p)
{
}

static const char *buildDate = __DATE__;
static const char *buildTime = __TIME__;
#pragma once

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdint.h>

#if !defined(OS_WINDOWS) && !defined(OS_WEB)
#error No Platform Defined! Must pass in platform flag (example: OS_WINDOWS)
#endif

#define KiloByte (((u64)1) << 10)
#define MegaByte (((u64)1) << 20)
#define GigaByte (((u64)1) << 30)

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef uint32_t b32;
typedef uint8_t b8;

#define STRINGIFY(a) STRINGIFY_IMPL(a)
#define STRINGIFY_IMPL(a) #a
#define ARRAY_COUNT(Array) (int)(sizeof(Array) / sizeof((Array)[0]))
#define InvalidCodePath ASSERT(!"InvalidCodePath")
#define UnimplementedCodePath InvalidCodePath
#define InvalidDefaultCase default: { InvalidCodePath; } break;
#define ZeroArray(array) memset(array, 0, ARRAY_COUNT(array));

#if DEBUG_MODE
#if OS_WINDOWS
#define ASSERT(expression) ((expression) || (__debugbreak(), 0))
#elif OS_WEB
#include "signal.h"
#define ASSERT(expression) ((expression) || raise(SIGTRAP))
#endif
#else
#define ASSERT(expression) (expression)
#endif

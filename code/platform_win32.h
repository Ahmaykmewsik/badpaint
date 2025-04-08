#pragma once

#if __clang__
#include "vn_intrinsics.h"
#endif

struct PlatformWorkQueue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(PlatformWorkQueue *queue, void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(PlatformWorkQueueCallback);

PlatformWorkQueue *SetupThreads(unsigned int threadCount, GameMemory *gameMemory);

void PlatformAddThreadWorkEntry(PlatformWorkQueue *queue, PlatformWorkQueueCallback *callback, void *data);

bool GetPngImageFilePathFromUser(char *buffer, unsigned int bufferSize, unsigned int *filePathLength);

#pragma once

struct PlatformWorkQueue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(PlatformWorkQueue *queue, void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(PlatformWorkQueueCallback);

struct GameMemory;

PlatformWorkQueue *SetupThreads(unsigned int threadCount, GameMemory *gameMemory);

void PlatformAddThreadWorkEntry(PlatformWorkQueue *queue, PlatformWorkQueueCallback *callback, void *data);

bool GetPngImageFilePathFromUser(char *buffer, unsigned int bufferSize, unsigned int *filePathLength);

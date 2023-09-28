
#include "vn_intrinsics.h"
#include "vn_math.h"
#include "platform_win32.h"

#include <intrin.h>
#include "Windows.h"

struct WorkQueueEntry
{
    void *data;
    PlatformWorkQueueCallback *callback;
};

struct PlatformWorkQueue
{
    unsigned int volatile completionGoalIndex;
    unsigned int volatile nextInQueueIndex;
    HANDLE semaphoreHandle;
    WorkQueueEntry entries[256];
};

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    for (;;)
    {
        PlatformWorkQueue *queue = (PlatformWorkQueue *)lpParameter;

        unsigned int nextInQueueIndex = queue->nextInQueueIndex;
        if (queue->completionGoalIndex != nextInQueueIndex)
        {
            Assert(queue->nextInQueueIndex < ArrayCount(queue->entries));
            unsigned int newNextEntryToRead = (queue->nextInQueueIndex + 1) % ArrayCount(queue->entries);
            unsigned int index = InterlockedCompareExchange((LONG volatile *)&queue->nextInQueueIndex, newNextEntryToRead, nextInQueueIndex);
            if (index == nextInQueueIndex)
            {
                WorkQueueEntry entry = queue->entries[index];
                entry.callback(queue, entry.data);
                // InterlockedIncrement((LONG volatile *)&queue->entryCompletionCount);
            }
        }
        else
        {
            WaitForSingleObjectEx(queue->semaphoreHandle, INFINITE, FALSE);
        }
    }
}

void PlatformAddThreadWorkEntry(PlatformWorkQueue *queue, PlatformWorkQueueCallback *callback, void *data)
{
    unsigned int countFoo = ArrayCount(queue->entries);

    Assert(queue->nextInQueueIndex < ArrayCount(queue->entries));

    WorkQueueEntry *entry = &queue->entries[queue->nextInQueueIndex];
    entry->data = data;
    entry->callback = callback;

    ModNext(queue->completionGoalIndex, ArrayCount(queue->entries));

    _WriteBarrier();

    ReleaseSemaphore(queue->semaphoreHandle, 1, 0);
}

PlatformWorkQueue *SetupThreads(unsigned int threadCount, GameMemory *gameMemory)
{
    Assert(threadCount > 0);

    PlatformWorkQueue *result = PushStruct(&gameMemory->permanentArena, PlatformWorkQueue);

    unsigned int initialThreadCount = 0;
    result->semaphoreHandle = CreateSemaphoreExA(0, initialThreadCount, threadCount, 0, 0, SEMAPHORE_ALL_ACCESS);

    for (int i = 0;
         i < threadCount;
         i++)
    {
        DWORD threadID;
        HANDLE threadHandle = CreateThread(0, 0, ThreadProc, result, 0, &threadID);
        CloseHandle(threadHandle);
    }

    return result;
}
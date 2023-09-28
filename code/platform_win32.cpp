
#include "vn_intrinsics.h"
#include "vn_string.h"
#include "platform_win32.h"

#include <intrin.h>
#include "Windows.h"

struct WorkQueueEntryStorage
{
    void *userPointer;
};

struct WorkQueue
{
    unsigned int volatile entryCompletionCount;
    unsigned int volatile nextEntryToDo;
    unsigned int volatile entryCount;
    HANDLE semaphoreHandle;

    WorkQueueEntryStorage entries[256];
};

struct WorkQueueEntry
{
    void *data;
    bool isValid;
};

struct Win32ThreadInfo
{
    int logicalThreadIndex;
    WorkQueue *queue;
};

void AddWorkQueueEntry(WorkQueue *Queue, void *Pointer)
{
    Assert(Queue->entryCount < ArrayCount(Queue->entries));
    Queue->entries[Queue->entryCount].userPointer = Pointer;
    _WriteBarrier();
    _mm_sfence();
    ++Queue->entryCount;
    ReleaseSemaphore(Queue->semaphoreHandle, 1, 0);
}

WorkQueueEntry CompleteAndGetNextWorkQueueEntry(WorkQueue *Queue, WorkQueueEntry Completed)
{
    WorkQueueEntry Result;
    Result.isValid = false;

    if (Completed.isValid)
    {
        InterlockedIncrement((LONG volatile *)&Queue->entryCompletionCount);
    }

    if (Queue->nextEntryToDo < Queue->entryCount)
    {
        unsigned int Index = InterlockedIncrement((LONG volatile *)&Queue->nextEntryToDo) - 1;
        Result.data = Queue->entries[Index].userPointer;
        Result.isValid = true;
        _ReadBarrier();
    }

    return (Result);
}

bool QueueWorkStillInProgress(WorkQueue *Queue)
{
    bool Result = (Queue->entryCount != Queue->entryCompletionCount);
    return (Result);
}

void DoWorkerWork(WorkQueueEntry Entry, int LogicalThreadIndex)
{
    Assert(Entry.isValid);

    char Buffer[256];
    printf(Buffer, "Thread %u: %s\n", LogicalThreadIndex, (char *)Entry.data);
    String threads = CreateString("Thread: ") +  + LogicalThreadIndex + " says " + (char *) Entry.data;
    Print(threads);
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    Win32ThreadInfo *threadInfo = (Win32ThreadInfo *)lpParameter;
    WorkQueueEntry entry = {};
    for (;;)
    {
        entry = CompleteAndGetNextWorkQueueEntry(threadInfo->queue, entry);
        if (entry.isValid)
        {
            DoWorkerWork(entry, threadInfo->logicalThreadIndex);
        }
        else
        {
            WaitForSingleObjectEx(threadInfo->queue->semaphoreHandle, INFINITE, FALSE);
        }
    }
}

void PushString(WorkQueue *Queue, char *String)
{
    AddWorkQueueEntry(Queue, String);
}

void SetupThreads(GameMemory *gameMemory)
{
    unsigned int threadCount = 8; 
    Win32ThreadInfo *threadInfos = PushArray(&gameMemory->permanentArena, threadCount, Win32ThreadInfo);
    WorkQueue *queue = PushStruct(&gameMemory->permanentArena, WorkQueue);
    G_STRING_TEMP_MEM_ARENA = &gameMemory->temporaryArena;

    unsigned int initialThreadCount = 0;
    queue->semaphoreHandle = CreateSemaphoreExA(0, initialThreadCount, threadCount, 0, 0, SEMAPHORE_ALL_ACCESS);

    for (int i = 0;
         i < threadCount;
         i++)
    {
        Win32ThreadInfo *threadInfoOfIndex = threadInfos + i;
        threadInfoOfIndex->queue = queue;
        threadInfoOfIndex->logicalThreadIndex = i;

        DWORD threadID;
        HANDLE threadHandle = CreateThread(0, 0, ThreadProc, threadInfoOfIndex, 0, &threadID);
        CloseHandle(threadHandle);
    }
    
    PushString(queue, "FUCK");
    PushString(queue, "FUCK");
    PushString(queue, "FUCK");
    PushString(queue, "FUCK");
    PushString(queue, "FUCK");
    PushString(queue, "FUCK");
    PushString(queue, "FUCK");
    PushString(queue, "FUCK");
    PushString(queue, "FUCK");
}
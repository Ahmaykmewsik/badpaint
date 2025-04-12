
#include <base/macros.h>

//NOTE: (Ahmayk) this is just something simple to work in a pinch for now

#if OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

static LARGE_INTEGER g_timeStart = {};
static u64 g_frequency = 0;

void ProfilerPrintTimeStart()
{
    QueryPerformanceCounter((LARGE_INTEGER*) &g_timeStart);
}

void ProfilerPrintTimeEnd(const char *label)
{
	LARGE_INTEGER timeEnd;
    QueryPerformanceCounter((LARGE_INTEGER*) &timeEnd);
	if (!g_frequency)
	{
    	QueryPerformanceFrequency((LARGE_INTEGER*) &g_frequency);
	}

	f64 durationMs = 1000 * (timeEnd.QuadPart - g_timeStart.QuadPart) / (f64) g_frequency;
	printf("%s: %f\n", label, durationMs);
}
#else
#error Platform not implemented in profiler.cpp
#endif

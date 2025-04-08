#include <base/multithreading.h>

#if OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

void LockOnBool(b32 *lock)
{
#if OS_WINDOWS
	while (InterlockedCompareExchange(lock, TRUE, FALSE) != FALSE)
	{
		LONG locked = FALSE;
		WaitOnAddress(lock, &locked, sizeof(locked), INFINITE);
	}
#endif
}

void UnlockOnBool(b32 *lock)
{
#if OS_WINDOWS
	InterlockedExchange(lock, FALSE);
	WakeByAddressSingle(lock);
#endif
}

void InterlockedExchangeAddU32(volatile u32 *addend, u32 value)
{
#if OS_WINDOWS
	InterlockedExchangeAdd((volatile LONG*)addend, value);
#endif
}

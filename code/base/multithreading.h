#pragma once

#include <base/macros.h>

void LockOnBool(b32 *lock);
void UnlockOnBool(b32 *lock);
void InterlockedExchangeAddU32(volatile u32 *addend, u32 value);

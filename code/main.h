
#if __clang__
#include "vn_intrinsics.h"
#include "vn_string.h"
#include "vn_math.h"
#include "platform_win32.h"
#endif

void RunApp(PlatformWorkQueue *platformWorkQueue, GameMemory gameMemory, unsigned int threadCount);

#define VERSION_NUMBER "0.0.1"

static String G_NOTIFICATION_MESSAGE = {};
static float G_NOTIFICATION_ALPHA = 0.0f;
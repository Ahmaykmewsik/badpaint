
#if __clang__
#include "vn_intrinsics.h"
#include "vn_string.h"
#include "vn_math.h"
#endif

struct GameState
{
    V2 windowDim;
};

void RunApp(GameMemory gameMemory);

#define VERSION_NUMBER "0.0.1"

static String G_NOTIFICATION_MESSAGE = {};
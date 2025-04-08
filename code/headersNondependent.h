#pragma once

#include "base/base.h"
#include "vn_math.h"

#include "../includes/raylib/src/external/stb_image.h"

#include "../includes/raylib/src/raylib.h"
#include "../includes/raylib/src/rlgl.h"
#include "../includes//raylib//src/external/glfw/include/GLFW/glfw3.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#if 0
#define STBIW_MALLOC(size) _TempALLOC(size)
#define STBIW_REALLOC_SIZED(p, oldSize, newSize) _TempREALLOC(p, oldSize, newSize)
#define STBIW_FREE(p) _TempFREE(p)
#endif

#include "../includes/raylib/src/external/stb_image_write.h"

#include "../includes/lodepng.h"
#include "../includes/lodepng.c"

#include "../includes/fpng.cpp"
#include "../includes/fpng.h"

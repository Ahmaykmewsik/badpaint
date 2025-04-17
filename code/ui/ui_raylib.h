#pragma once

//TODO: (Ahmayk) only define raylib here, not in ui.h
#include <ui/ui.h>
#include "../includes/raylib/src/raylib.h"

struct RaylibRenderData
{
	Font *fonts;
	u32 fontCount;
};

void UiRaylibProcessStrings(UiBuffer *uiBuffer, RaylibRenderData *raylibRenderData);
void UiRenderBlocksRaylib(UiBuffer *uiBuffer, RaylibRenderData *raylibRenderData);

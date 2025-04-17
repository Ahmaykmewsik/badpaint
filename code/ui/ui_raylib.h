#pragma once

//TODO: (Ahmayk) only define raylib here, not in ui.h
#include <ui/ui.h>
#include "../includes/raylib/src/raylib.h"

UiTexture UiRaylibTextureToUiTexture(Texture *texture);
void UiRaylibProcessStrings(UiBuffer *uiBuffer);
void UiRenderBlocksRaylib(UiBuffer *uiBuffer);

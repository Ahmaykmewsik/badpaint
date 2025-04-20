#pragma once

//TODO: (Ahmayk) only define raylib here, not in ui.h
#include <ui/ui_core.h>
#include "../includes/raylib/src/raylib.h"

UiTextureView UiRaylibTextureToUiTextureView(Texture *texture);
void UiRaylibProcessStrings(UiBuffer *uiBuffer);
void UiRaylibRenderBlocks(UiBuffer *uiBuffer);

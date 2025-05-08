#pragma once

#include <ui/ui_core.h>
struct Texture;

UiTextureView UiRaylibTextureToUiTextureView(Texture *texture);
void UiRaylibProcessStrings(UiBuffer *uiBuffer);
void UiRaylibRenderBlocks(UiBuffer *uiBuffer);
void UiRaylibSetCursor(UI_CURSOR_TYPE uiCursorType);

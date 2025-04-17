#pragma once

#include <ui/ui.h>
#include <input.h>

static float G_TOOLBOX_WIDTH_AND_HEIGHT = 35;

UiBlock *CreateBrushEffectButton(BRUSH_EFFECT brushEffect, String string, UiFont uiFont, ColorU32 baseColor, COMMAND command, Brush *currentBrush);

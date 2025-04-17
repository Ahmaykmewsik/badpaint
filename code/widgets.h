#pragma once

#include <ui/ui_core.h>
#include <ui/ui_base_widgets.h>
#include <input.h>

static float G_TOOLBOX_WIDTH_AND_HEIGHT = 35;

UiBlock *CreateBrushEffectButton(BRUSH_EFFECT brushEffect, String string, UiFont uiFont, ColorU32 baseColor, COMMAND command, Brush *currentBrush);

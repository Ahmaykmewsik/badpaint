#pragma once

#include <ui/ui.h>
#include <input.h>

static float G_TOOLBOX_WIDTH_AND_HEIGHT = 35;

void CreateBrushEffectButton(BRUSH_EFFECT brushEffect, String string, Color baseColor, COMMAND command, Brush *currentBrush);

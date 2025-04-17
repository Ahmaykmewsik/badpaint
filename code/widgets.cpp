
#include "widgets.h"

UiBlock *CreateBrushEffectButton(BRUSH_EFFECT brushEffect, String string, UiFont uiFont, Color baseColor, COMMAND command, Brush *currentBrush)
{
	u32 hash = Murmur3String("brushEffect", brushEffect);
	ReactiveUiColorState uiColorState = CreateButtonReactiveUiColorState(baseColor);
	bool active = currentBrush->brushEffect == brushEffect;

	UiBlock *b = CreateUiButton(string, hash, uiFont, uiColorState, active, false);
	b->command = command;
	b->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
	b->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};

	return b;
}


#include "widgets.h"

void CreateBrushEffectButton(BRUSH_EFFECT brushEffect, String string, Color baseColor, COMMAND command, Brush *currentBrush)
{
	SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
	u32 hash = Murmur3String("brushEffect", brushEffect);
	ReactiveUiColorState uiColorState = CreateButtonReactiveUiColorState(baseColor);
	bool active = currentBrush->brushEffect == brushEffect;

	UiInputs *G_UI_INPUTS = GetUiInputs();

	G_UI_INPUTS->command = command;
	CreateUiButton(string, hash, uiColorState, active, false);
}

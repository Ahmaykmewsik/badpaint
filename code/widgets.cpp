
#include "widgets.h"

void CreateBrushEffectButton(BRUSH_EFFECT brushEffect, String string, Color baseColor, COMMAND command, Brush *currentBrush)
{
	SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
	String stringButton = string + G_UI_HASH_TAG + U32ToString(brushEffect, StringArena());
	ReactiveUiColorState uiColorState = CreateButtonReactiveUiColorState(baseColor);
	bool active = currentBrush->brushEffect == brushEffect;

	UiInputs *G_UI_INPUTS = GetUiInputs();

	G_UI_INPUTS->command = command;
	CreateUiButton(stringButton, uiColorState, active, false);
}

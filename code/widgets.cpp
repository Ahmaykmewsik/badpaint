
#include "widgets.h"

UiBlock *CreateBrushEffectButton(BRUSH_EFFECT brushEffect, String string, UiFont uiFont, ColorU32 baseColor, COMMAND command, Brush *currentBrush)
{
	u32 hash = Murmur3String("brushEffect", brushEffect);
	UiReactiveColorStates uiColorStates = CreateButtonUiReactiveColorStates(baseColor);
	bool active = currentBrush->brushEffect == brushEffect;

	UiBlock *result = CreateUiButton(string, hash, uiFont, uiColorStates, active, false);
	result->command = command;
	result->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
	result->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};

	return result;
}

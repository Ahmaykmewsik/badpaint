
#include "widgets.h"
#include "vn_math_external.h"

ColorU32 AddConstantToColor(ColorU32 color, i8 constant)
{
	ColorU32 result = color;
	result.r = (u8) ClampI32(0, result.r + constant, 255);
	result.g = (u8) ClampI32(0, result.g + constant, 255);
	result.b = (u8) ClampI32(0, result.b + constant, 255);
	return result;
}

UiReactiveColorStates CreateButtonUiReactiveColorStates(ColorU32 color)
{
	UiReactiveColorStates result = {};

	result.active.down = AddConstantToColor(color, -100);
	result.active.hovered = AddConstantToColor(color, 10);
	result.active.neutral = color;
	result.nonActive.down = AddConstantToColor(color, -100);
	result.nonActive.hovered = AddConstantToColor(color, -20);
	result.nonActive.neutral = AddConstantToColor(color, -50);
	return result;
}

ColorU32 GetReactiveColorU32(u32 hash, UiInteractionHashes *uiInteractionHashes, UiReactiveColors *uiReactiveColors, b32 isDisabled, b32 downOverride)
{
	ColorU32 result = uiReactiveColors->neutral;
	if (isDisabled)
	{
		result = uiReactiveColors->disabled;
	}
	else
	{
		if (hash == uiInteractionHashes->hashMouseDown || downOverride)
		{
			result = uiReactiveColors->down;
		}
		else if (hash == uiInteractionHashes->hashMouseHover)
		{
			result = uiReactiveColors->hovered;
		}
	}
	return result;
}

b32 WidgetBrushEffectButton(UiState *uiState, AppState *appState, UiInteractionHashes *uiInteractionHashes, BRUSH_EFFECT brushEffect, String string, COMMAND command)
{
	u32 hash = Murmur3String("brushEffect", brushEffect);
	b32 active = appState->currentBrush.brushEffect == brushEffect;
	b32 isDisabled = false;
	b32 downOverride = IsCommandKeyBindingDown(command);

	UiBlock *block = UiCreateBlock(uiState);
	block->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER | UI_FLAG_DRAW_TEXT | UI_FLAG_INTERACTABLE;
	block->hash = hash;
	block->string = string;
	block->uiFont = appState->defaultUiFont;
	block->uiBlockColors.frontColor = COLORU32_BLACK;
	block->uiBlockColors.borderColor = (active) ? COLORU32_BLACK: COLORU32_GRAY;
	block->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
	block->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
	block->uiTextAlignTypes[UI_AXIS_X] = UI_TEXT_ALIGN_CENTER;
	block->uiTextAlignTypes[UI_AXIS_Y] = UI_TEXT_ALIGN_CENTER;

	ColorU32 baseColor = BRUSH_EFFECT_COLORS_PRIMARY[brushEffect];
	if (brushEffect == BRUSH_EFFECT_ERASE)
	{
		baseColor = ColorU32{245, 245, 245, 255};
	}
	UiReactiveColorStates uiReactiveColorStates = CreateButtonUiReactiveColorStates(baseColor);
	UiReactiveColors uiReactiveColors = uiReactiveColorStates.nonActive;
	if (active)
	{
		uiReactiveColors = uiReactiveColorStates.active;
	}
	block->uiBlockColors.backColor = GetReactiveColorU32(hash, uiInteractionHashes, &uiReactiveColors, isDisabled, downOverride);

	b32 result = (hash == uiInteractionHashes->hashMousePressed);
	return result;
}

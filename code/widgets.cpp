
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

b32 WidgetBrushEffectButton(UiState *uiState, AppState *appState, UiInteractionHashes *uiInteractionHashes, BRUSH_EFFECT brushEffect, String string, COMMAND command)
{
	ColorU32 baseColor = BRUSH_EFFECT_COLORS_PRIMARY[brushEffect];
	u32 hash = Murmur3String("brushEffect", brushEffect);
	UiReactiveColorStates uiReactiveColorStates = CreateButtonUiReactiveColorStates(baseColor);
	b32 active = appState->currentBrush.brushEffect == brushEffect;
	b32 disabled = false;
	b32 downOverride = IsCommandKeyBindingDown(command);

	UiBlock *button = UiCreateBlock(uiState);
	button->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER | UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_CENTERED | UI_FLAG_INTERACTABLE;
	button->hash = hash;
	button->string = string;
	button->uiFont = appState->defaultUiFont;
	button->uiBlockColors.frontColor = COLORU32_BLACK;
	button->uiBlockColors.borderColor = (active) ? COLORU32_BLACK: COLORU32_GRAY;
	button->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
	button->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};

	UiReactiveColors uiReactiveColors = (active)
		? uiReactiveColorStates.active
		: uiReactiveColorStates.nonActive;

	UiBlock *uiBlockLastFrame = UiGetBlockOfHashLastFrame(uiState, hash);

	button->uiBlockColors.backColor = uiReactiveColors.neutral;

	b32 isPressed = 0;
	if (disabled)
	{
		button->uiBlockColors.backColor = uiReactiveColors.disabled;
	}
	else
	{
		if (uiBlockLastFrame->hash == uiInteractionHashes->hashMousePressed)
		{
			isPressed = true;
		}

		if (uiBlockLastFrame->hash == uiInteractionHashes->hashMouseDown || downOverride)
		{
			button->uiBlockColors.backColor = uiReactiveColors.down;
		}
		else if (uiBlockLastFrame->hash == uiInteractionHashes->hashMouseHover)
		{
			button->uiBlockColors.backColor = uiReactiveColors.hovered;
		}
	}

	return isPressed;
}

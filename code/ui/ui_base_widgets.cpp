#include "ui/ui_base_widgets.h"

ColorU32 GetReactiveColorU32(CommandInput *commandInputs, UiBlock *uiBlockLastFrame, UiReactiveColors uiReactiveColors, b32 disabled)
{
	ColorU32 result = uiReactiveColors.neutral;

	if (disabled)
	{
		result = uiReactiveColors.disabled;
	}
	else if (uiBlockLastFrame)
	{
		b32 down = uiBlockLastFrame->down;
		b32 hovered = uiBlockLastFrame->hovered;

		COMMAND command = uiBlockLastFrame->command;
		if (down || (command && IsCommandDown(commandInputs, command)))
		{
			result = uiReactiveColors.down;
		}
		else if (hovered)
		{
			result = uiReactiveColors.hovered;
		}
	}

	return result;
}

UiBlock *CreateUiButton(UiState *uiState, String string, u32 hash, UiFont uiFont, UiReactiveColorStates uiReactiveColorStates, b32 active, b32 disabled, CommandInput *commandInputs)
{
	UiBlock *result = CreateUiBlock(uiState);
	result->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER | UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_CENTERED | UI_FLAG_INTERACTABLE;
	result->hash = hash;
	result->string = string;
	result->uiFont = uiFont;
	result->uiBlockColors.frontColor = COLORU32_BLACK;
	result->uiBlockColors.borderColor = (active) ? COLORU32_BLACK: COLORU32_GRAY;

	UiReactiveColors uiReactiveColors = (active)
		? uiReactiveColorStates.active
		: uiReactiveColorStates.nonActive;
	UiBlock *uiBlockLastFrame = GetUiBlockOfHashLastFrame(uiState, hash);
	result->uiBlockColors.backColor = GetReactiveColorU32(commandInputs, uiBlockLastFrame, uiReactiveColors, disabled);
	return result;
}

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

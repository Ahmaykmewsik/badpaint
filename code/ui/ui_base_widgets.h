#pragma once

#include "ui/ui_core.h"

struct UiReactiveColors
{
	ColorU32 neutral;
	ColorU32 hovered;
	ColorU32 down;
	ColorU32 disabled;
};

struct UiReactiveColorStates
{
	UiReactiveColors active;
	UiReactiveColors nonActive;
};

ColorU32 GetReactiveColorU32(CommandInput *commandInputs, UiBlock *uiBlockLastFrame, UiReactiveColors uiReactiveColors, b32 disabled);
UiBlock *CreateUiButton(UiState *uiState, String string, u32 hash, UiFont uiFont, UiReactiveColorStates uiReactiveColorStates, b32 active, b32 disabled, CommandInput *commandInputs);
UiReactiveColorStates CreateButtonUiReactiveColorStates(ColorU32 color);

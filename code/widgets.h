#pragma once

#include <ui/ui_core.h>
#include <main.h>

static float G_TOOLBOX_WIDTH_AND_HEIGHT = 35;

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

struct UiInteractionHashes
{
	u32 hashMouseHover;
	u32 hashMouseDown;
	u32 hashMousePressed;
};

UiReactiveColorStates CreateButtonUiReactiveColorStates(ColorU32 color);

b32 WidgetBrushEffectButton(UiState *uiState, AppState *appState, UiInteractionHashes *uiInteractionHashes, BRUSH_EFFECT brushEffect, String string, COMMAND command);




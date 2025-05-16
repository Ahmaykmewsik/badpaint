#pragma once

#include <ui/ui_core.h>

struct UiPanel
{
	u32 hash;
	UI_AXIS childSplitAxis;
	f32 percentOfParent;

	UiPanel *firstChild;
	UiPanel *lastChild;
	UiPanel *next;
	UiPanel *prev;
	UiPanel *parent;

	u32 uiPanelType;
};

struct UiPanelPair
{
	UiPanel *uiPanel1;
	UiPanel *uiPanel2;
};

UiPanelPair SplitPanel(UiPanel *uiPanel, Arena *arena, UI_AXIS uiAxis, f32 percentOfParent);

UiBlock *WidgetMenuButton(UiState *uiState, String string, u32 hash, UiFont uiFont, u32 command);

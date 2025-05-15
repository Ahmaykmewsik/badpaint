#pragma once

#include <ui/ui_core.h>

enum INTERACTION_STATE
{
	INTERACTION_STATE_NONACTIVE_NEUTRAL,
	INTERACTION_STATE_NONACTIVE_HOVERED,
	INTERACTION_STATE_DOWN,
	INTERACTION_STATE_ACTIVE_NEUTRAL,
	INTERACTION_STATE_ACTIVE_HOVERED,
	INTERACTION_STATE_DISABLED,
	INTERACTION_STATE_COUNT,
};

struct UiInteractionHashes
{
	u32 hashMouseHover;
	u32 hashMouseDown;
	u32 hashMousePressed;
};

INTERACTION_STATE GetInteractionState(u32 hash, UiInteractionHashes *uiInteractionHashes, b32 isActive, b32 isDisabled, b32 downOverride);

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

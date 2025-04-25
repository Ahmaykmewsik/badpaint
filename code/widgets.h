#pragma once

#include <ui/ui_core.h>
#include <image.h>
#include <appCommands.h>

static float G_TOOLBOX_WIDTH_AND_HEIGHT = 35;

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

struct AppState;
struct FrameState;

UiBlock *WidgetBrushEffectButton(UiState *uiState, AppState *appState, FrameState *frameState, BADPAINT_PIXEL_TYPE badpaintPixelType, String string, COMMAND command);
UiBlock *WidgetToolButton(UiState *uiState, AppState *appState, FrameState *frameState, BADPAINT_TOOL_TYPE badpaintToolType, COMMAND command);

enum UI_PANEL_TYPE : u32
{
	UI_PANEL_TYPE_NULL,
	UI_PANEL_TYPE_ROOT_IMAGE,
	UI_PANEL_TYPE_PNG_FILTERED,
	UI_PANEL_TYPE_FINAL_IMAGE,
	UI_PANEL_TYPE_LAYERS,
};

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
void BuildPanelTree(UiState *uiState, AppState *appState, FrameState *frameState, UiPanel *uiPanel);

//TODO: (Ahmayk) Replace this stupid shit once we have better UI
struct NotificationMessage
{
	String string;
	f32 alpha;
};

void InitNotificationMessage(String string, Arena *circularNotificationBuffer);

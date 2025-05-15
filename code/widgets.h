#pragma once

#include <ui/ui_core.h>
#include <ui/ui_base_widgets.h>
#include <image.h>
#include <appCommands.h>

static float G_TOOLBOX_WIDTH_AND_HEIGHT = 35;

#define UI_APP_DEPTH_LAYER_ABOVE 100
#define UI_APP_DEPTH_LAYER_CURSOR_GUI 200
#define UI_APP_DEPTH_HOVERING_WINDOW 300

struct AppState;
struct FrameState;

UiBlock *WidgetBrushEffectButton(UiState *uiState, AppState *appState, FrameState *frameState, BADPAINT_PIXEL_TYPE badpaintPixelType, String string, COMMAND command);
UiBlock *WidgetToolButton(UiState *uiState, AppState *appState, FrameState *frameState, String label, BADPAINT_TOOL_TYPE badpaintToolType, COMMAND command);

enum UI_PANEL_TYPE : u32
{
	UI_PANEL_TYPE_NULL,
	UI_PANEL_TYPE_ROOT_IMAGE,
	UI_PANEL_TYPE_PNG_FILTERED,
	UI_PANEL_TYPE_FINAL_IMAGE,
	UI_PANEL_TYPE_LAYERS,
};
void BuildPanelTree(UiState *uiState, AppState *appState, FrameState *frameState, UiPanel *uiPanel);

//TODO: (Ahmayk) Replace this stupid shit once we have better UI
struct NotificationMessage
{
	String string;
	f32 alpha;
};

void InitNotificationMessage(String string, Arena *circularNotificationBuffer);


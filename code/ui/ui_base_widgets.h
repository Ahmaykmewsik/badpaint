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

struct AppCommand
{
	u32 command;
	union
	{
		v2 value1V2;
		String value1String;
	};
	union
	{
		v2 value2V2;
	};
	union
	{
		u32 value3U32;
	};
};

struct AppCommandBuffer
{
	AppCommand *appCommands;
	u32 count;
	u32 size;
	ArenaMarker arenaMarker;
};

AppCommand *PushAppCommand(AppCommandBuffer *appCommandBuffer);

struct MenuButtonStyleDesc
{
	UiFont uiFont; 
	ColorU32 baseColor; 
};

UiBlock *WidgetMenuButton(UiState *uiState, String string, u32 hash, AppCommandBuffer *appCommandBuffer, u32 command, MenuButtonStyleDesc *menuButtonStyleDesc);

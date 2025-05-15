#pragma once

#include <base/base.h>

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

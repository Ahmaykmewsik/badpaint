#include "ui/ui_interaction.h"

INTERACTION_STATE GetInteractionState(u32 hash, UiInteractionHashes *uiInteractionHashes, b32 isActive, b32 isDisabled, b32 downOverride)
{
	INTERACTION_STATE result = INTERACTION_STATE_NONACTIVE_NEUTRAL;
	if (isDisabled)
	{
		result = INTERACTION_STATE_DISABLED;
	}
	else
	{
		if (isActive)
		{
			result = INTERACTION_STATE_ACTIVE_NEUTRAL;
		}

		if (hash == uiInteractionHashes->hashMouseDown || downOverride)
		{
			result = INTERACTION_STATE_DOWN;
		}
		else if (hash == uiInteractionHashes->hashMouseHover)
		{
			result = INTERACTION_STATE_NONACTIVE_HOVERED;
			if (isActive)
			{
				result = INTERACTION_STATE_ACTIVE_HOVERED;
			}
		}
	}
	return result;
}

AppCommand *PushAppCommand(AppCommandBuffer *appCommandBuffer)
{
	AppCommand *result;
	if (ASSERT(appCommandBuffer->count < appCommandBuffer->size))
	{
		result = &appCommandBuffer->appCommands[appCommandBuffer->count++];
	}
	else
	{
		static AppCommand stub = {};
		result = &stub;
	}
	*result = {};
	return result;
}


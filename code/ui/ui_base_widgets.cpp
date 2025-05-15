#include <ui/ui_core.h>
#include <ui/ui_base_widgets.h>

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

UiPanel *UiPanelCreateAndParent(Arena *arena, UiPanel *parent)
{
	UiPanel *result = ARENA_PUSH_STRUCT(arena, UiPanel);
	*result = {};
	result->parent = parent;
	if (result->parent->firstChild)
	{
		ASSERT(result->parent->lastChild);
		result->prev = result->parent->lastChild;
		result->parent->lastChild->next = result;
	}
	else
	{
		result->parent->firstChild = result;
	}
	result->parent->lastChild = result;
	parent->uiPanelType = {};

	//NOTE: (Ahmayk) Generate a hash for a panel based on the root and all other hashes in the tree.
	//Not 100% sure if this checks out but what is life worth living for without a little risk and experimentation?
	//root panel must be manually assigned hash for this to work
	ASSERT(parent->hash);
	UiPanel *parentOf = parent;
	while(parentOf)
	{
		u32 seed = Murmur3String("Parent", result->hash);
		result->hash = Murmur3U32(parentOf->hash, seed, result->hash);
		parentOf = parentOf->parent;
	}
	UiPanel *prevSibling = result->prev;
	while (prevSibling)
	{
		u32 seed = Murmur3String("Sibling", result->hash);
		result->hash = Murmur3U32(prevSibling->hash, seed, result->hash);
		prevSibling = prevSibling->prev;
	}

	return result;
}

UiPanelPair SplitPanel(UiPanel *uiPanel, Arena *arena, UI_AXIS uiAxis, f32 percentOfParent)
{
	UiPanelPair result = {};
	uiPanel->childSplitAxis = uiAxis;
	uiPanel->uiPanelType = {};
	result.uiPanel1 = UiPanelCreateAndParent(arena, uiPanel);
	result.uiPanel1->percentOfParent = percentOfParent;
	result.uiPanel2 = UiPanelCreateAndParent(arena, uiPanel);
	result.uiPanel2->percentOfParent = 1 - percentOfParent;
	return result;
}


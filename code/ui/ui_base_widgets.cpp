
#include <ui/ui_base_widgets.h>

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

UiBlock *WidgetMenuButton(UiState *uiState, String string, u32 hash, AppCommandBuffer *appCommandBuffer, u32 command, MenuButtonStyleDesc *menuButtonStyleDesc)
{
	UiBlock *result = UiCreateBlock(uiState);
	result->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_INTERACTABLE;
	result->hash = hash;
	result->uiSizes[UI_AXIS_X] = {UI_SIZE_FILL};
	result->uiSizes[UI_AXIS_Y] = {UI_SIZE_FIT_CHILDREN};

	ColorU32 baseColor = menuButtonStyleDesc->baseColor;
	ColorU32 colors[INTERACTION_STATE_COUNT];
	colors[INTERACTION_STATE_NONACTIVE_NEUTRAL] = AddConstantToColor(baseColor, -50);
	colors[INTERACTION_STATE_NONACTIVE_HOVERED] = AddConstantToColor(baseColor, -20);
	colors[INTERACTION_STATE_DOWN] = AddConstantToColor(baseColor, -100);
	colors[INTERACTION_STATE_ACTIVE_NEUTRAL] = AddConstantToColor(baseColor, 0);
	colors[INTERACTION_STATE_ACTIVE_HOVERED] = AddConstantToColor(baseColor, 10);
	INTERACTION_STATE interactionState = GetInteractionState(&uiState->uiInteractionState, hash, true, false, false);
	result->uiBlockColors.backColor = colors[interactionState];

	UI_PARENT_SCOPE(uiState, result)
	{
		UiBlock *t = UiCreateBlock(uiState);
		t->flags = UI_FLAG_DRAW_TEXT;
		t->uiSizes[UI_AXIS_X] = {UI_SIZE_TEXT};
		t->uiSizes[UI_AXIS_Y] = {UI_SIZE_TEXT};
		t->uiTextAlignTypes[UI_AXIS_X] = UI_TEXT_ALIGN_CENTER;
		t->uiTextAlignTypes[UI_AXIS_Y] = UI_TEXT_ALIGN_CENTER;
		t->string = string;
		t->uiFont = menuButtonStyleDesc->uiFont;
		t->uiBlockColors.frontColor = COLORU32_BLACK;
	}

	if (uiState->uiInteractionState.hashMousePressed == hash)
	{
		AppCommand *appCommand = PushAppCommand(appCommandBuffer);
		appCommand->command = command;
	}

	return result;
}

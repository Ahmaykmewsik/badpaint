
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

UiBlock *WidgetMenuBar(UiState *uiState, MenuBarState *menuBarState, u32 hash)
{
	UiBlock *menuBar = UiCreateBlock(uiState);
	menuBar->flags = UI_FLAG_DRAW_BORDER;
	menuBar->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
	menuBar->uiSizes[UI_AXIS_Y] = {UI_SIZE_FIT_CHILDREN};
	menuBar->uiChildAlignTypes[UI_AXIS_Y] = UI_CHILD_ALIGN_CENTER;
	menuBar->uiBlockColors.borderColor = COLORU32_BLACK;
	menuBar->hash = hash;
	menuBarState->hashMenuBar = hash;
	return menuBar;
}

void UiBlockCopyStyle(UiBlock *uiBlock, UiBlock *styleBlock)
{
	uiBlock->uiSizes[UI_AXIS_X] = styleBlock->uiSizes[UI_AXIS_X];
	uiBlock->uiSizes[UI_AXIS_Y] = styleBlock->uiSizes[UI_AXIS_Y];
	uiBlock->uiPosition[UI_AXIS_X] = styleBlock->uiPosition[UI_AXIS_X];
	uiBlock->uiPosition[UI_AXIS_Y] = styleBlock->uiPosition[UI_AXIS_Y];
	uiBlock->uiPositionOffset[UI_AXIS_X] = styleBlock->uiPositionOffset[UI_AXIS_X];
	uiBlock->uiPositionOffset[UI_AXIS_Y] = styleBlock->uiPositionOffset[UI_AXIS_Y];
	uiBlock->uiChildAlignTypes[UI_AXIS_X] = styleBlock->uiChildAlignTypes[UI_AXIS_X];
	uiBlock->uiChildAlignTypes[UI_AXIS_Y] = styleBlock->uiChildAlignTypes[UI_AXIS_Y];
	uiBlock->uiChildLayoutType = styleBlock->uiChildLayoutType;
	uiBlock->uiTextAlignTypes[UI_AXIS_X] = styleBlock->uiTextAlignTypes[UI_AXIS_X];
	uiBlock->uiTextAlignTypes[UI_AXIS_Y] = styleBlock->uiTextAlignTypes[UI_AXIS_Y];
	uiBlock->padding = styleBlock->padding;
	uiBlock->uiFont = styleBlock->uiFont;
	uiBlock->uiBlockColors = styleBlock->uiBlockColors;
	uiBlock->depthLayer = styleBlock->depthLayer;
}

UiBlock *WidgetMenuBarButton(UiState *uiState, MenuBarState *menuBarState, String string, UiBlock *styleBlock)
{
	UiBlock *dropdownButton = UiCreateBlock(uiState);
	UiBlockCopyStyle(dropdownButton, styleBlock);
	dropdownButton->flags = UI_FLAG_DRAW_TEXT | UI_FLAG_DRAW_BACKGROUND | UI_FLAG_INTERACTABLE;
	dropdownButton->uiSizes[UI_AXIS_X] = {UI_SIZE_TEXT};
	dropdownButton->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, 16};
	dropdownButton->uiTextAlignTypes[UI_AXIS_X] = UI_TEXT_ALIGN_CENTER;
	dropdownButton->uiTextAlignTypes[UI_AXIS_Y] = UI_TEXT_ALIGN_CENTER;
	dropdownButton->string = string;
	dropdownButton->hash = Murmur3StringLength(string.chars, string.length, menuBarState->hashMenuBar);
	dropdownButton->padding = iv2{8, 4};

	ColorU32 baseColor = styleBlock->uiBlockColors.backColor;
	ColorU32 colors[INTERACTION_STATE_COUNT];
	colors[INTERACTION_STATE_NONACTIVE_NEUTRAL] = AddConstantToColor(baseColor, -50);
	colors[INTERACTION_STATE_NONACTIVE_HOVERED] = AddConstantToColor(baseColor, -20);
	colors[INTERACTION_STATE_DOWN] = AddConstantToColor(baseColor, -100);
	colors[INTERACTION_STATE_ACTIVE_NEUTRAL] = AddConstantToColor(baseColor, 0);
	colors[INTERACTION_STATE_ACTIVE_HOVERED] = AddConstantToColor(baseColor, 10);
	INTERACTION_STATE interactionState = GetInteractionState(&uiState->uiInteractionState, dropdownButton->hash, true, false, false);
	dropdownButton->uiBlockColors.backColor = colors[interactionState];

	b32 pressedMenu = uiState->uiInteractionState.hashMousePressed == dropdownButton->hash;
	b32 hoveredMenuWhileMenuActive = menuBarState->hashOpenMenuBarButton && uiState->uiInteractionState.hashMouseHover == dropdownButton->hash;
	if (pressedMenu || hoveredMenuWhileMenuActive)
	{
		menuBarState->hashOpenMenuBarButton = dropdownButton->hash;
	}
	else if (menuBarState->hashOpenMenuBarButton == dropdownButton->hash && uiState->uiInteractionState.uiInteractionFrameInput.isMouseLeftPressed)
	{
		UiBlock *dropdownButtonPrev = UiGetBlockOfHashLastFrame(uiState, dropdownButton->hash);
		if (dropdownButtonPrev->hash && ASSERT(dropdownButtonPrev->firstChild))
		{
			//NOTE: (Ahmayk) assumes that first child is menu panel and that there's only one (this will change when we do nested menus)
			if (!IsInRectV2(uiState->uiInteractionState.uiInteractionFrameInput.mousePixelPos, dropdownButtonPrev->firstChild->rect))
			{
				menuBarState->hashOpenMenuBarButton = {};
			}
		}
	}
	return dropdownButton;
}

UiBlock *WidgetMenuPanel(UiState *uiState, UiBlock *styleBlock)
{
	UiBlock *b = UiCreateBlock(uiState);
	UiBlockCopyStyle(b, styleBlock);
	b->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER;
	b->uiPosition[UI_AXIS_X] = {UI_POSITION_RELATIVE, 0};
	b->uiPosition[UI_AXIS_Y] = {UI_POSITION_PERCENT_OF_PARENT, 1};
	b->uiSizes[UI_AXIS_X] = {UI_SIZE_FIT_CHILDREN};
	b->uiSizes[UI_AXIS_Y] = {UI_SIZE_FIT_CHILDREN};
	b->uiChildLayoutType = UI_CHILD_LAYOUT_TOP_TO_BOTTOM;
	return b;
}

UiBlock *WidgetMenuOptionButton(UiState *uiState, MenuBarState *menuBarState, String string, u32 uiPanelHash, AppCommandBuffer *appCommandBuffer, u32 command, UiBlock *styleBlock)
{
	UiBlock *result = UiCreateBlock(uiState);
	UiBlockCopyStyle(result, styleBlock);
	result->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_INTERACTABLE;
	result->hash = Murmur3StringLength(string.chars, string.length, uiPanelHash);
	result->uiSizes[UI_AXIS_X] = {UI_SIZE_FILL};
	result->uiSizes[UI_AXIS_Y] = {UI_SIZE_FIT_CHILDREN};
	result->uiChildAlignTypes[UI_AXIS_Y] = UI_CHILD_ALIGN_CENTER;

	ColorU32 baseColor = styleBlock->uiBlockColors.backColor;
	ColorU32 colors[INTERACTION_STATE_COUNT];
	colors[INTERACTION_STATE_NONACTIVE_NEUTRAL] = AddConstantToColor(baseColor, -50);
	colors[INTERACTION_STATE_NONACTIVE_HOVERED] = AddConstantToColor(baseColor, -20);
	colors[INTERACTION_STATE_DOWN] = AddConstantToColor(baseColor, -100);
	colors[INTERACTION_STATE_ACTIVE_NEUTRAL] = AddConstantToColor(baseColor, 0);
	colors[INTERACTION_STATE_ACTIVE_HOVERED] = AddConstantToColor(baseColor, 10);
	INTERACTION_STATE interactionState = GetInteractionState(&uiState->uiInteractionState, result->hash, true, false, false);
	result->uiBlockColors.backColor = colors[interactionState];

	UI_PARENT_SCOPE(uiState, result)
	{
		UiBlock *m = UiCreateBlock(uiState);
		m->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, 8};
		m->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 0.5};

		UiBlock *t = UiCreateBlock(uiState);
		t->flags = UI_FLAG_DRAW_TEXT;
		t->uiSizes[UI_AXIS_X] = {UI_SIZE_TEXT};
		t->uiSizes[UI_AXIS_Y] = {UI_SIZE_TEXT};
		t->uiTextAlignTypes[UI_AXIS_Y] = UI_TEXT_ALIGN_CENTER;
		t->string = string;
		t->uiFont = styleBlock->uiFont;
		t->uiBlockColors.frontColor = styleBlock->uiBlockColors.frontColor;
		t->padding = styleBlock->padding;
	}

	if (uiState->uiInteractionState.hashMousePressed == result->hash)
	{
		AppCommand *appCommand = PushAppCommand(appCommandBuffer);
		appCommand->command = command;
	}
	if (uiState->uiInteractionState.hashMouseReleased == result->hash)
	{
		menuBarState->hashOpenMenuBarButton = {};
	}

	return result;
}

UiBlock *WidgetSeperatorX(UiState *uiState, u32 thickness, ColorU32 color, u32 paddingY)
{
	UiBlock *b = UiCreateBlock(uiState);
	b->uiSizes[UI_AXIS_X] = {UI_SIZE_FILL};
	b->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, (f32) (paddingY * 2) + thickness};
	b->uiChildAlignTypes[UI_AXIS_X] = UI_CHILD_ALIGN_CENTER;
	b->uiChildAlignTypes[UI_AXIS_Y] = UI_CHILD_ALIGN_CENTER;
	b->uiChildLayoutType = UI_CHILD_LAYOUT_TOP_TO_BOTTOM;
	UI_PARENT_SCOPE(uiState, b)
	{
		UiBlock *s = UiCreateBlock(uiState);
		s->flags = UI_FLAG_DRAW_BACKGROUND;
		s->uiSizes[UI_AXIS_X] = {UI_SIZE_FILL};
		s->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, (f32) thickness};
		s->uiBlockColors.backColor = color;
	}
	return b;
}

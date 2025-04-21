
#include "widgets.h"
#include "vn_math_external.h"
#include "ui/ui_raylib.h"

ColorU32 AddConstantToColor(ColorU32 color, i8 constant)
{
	ColorU32 result = color;
	result.r = (u8) ClampI32(0, result.r + constant, 255);
	result.g = (u8) ClampI32(0, result.g + constant, 255);
	result.b = (u8) ClampI32(0, result.b + constant, 255);
	return result;
}

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

b32 WidgetBrushEffectButton(UiState *uiState, AppState *appState, UiInteractionHashes *uiInteractionHashes, BADPAINT_BRUSH_EFFECT brushEffect, String string, COMMAND command)
{
	u32 hash = Murmur3String("brushEffect", brushEffect);
	b32 active = appState->currentBrushEffect == brushEffect;
	b32 isDisabled = false;
	b32 downOverride = IsCommandKeyBindingDown(command);

	UiBlock *block = UiCreateBlock(uiState);
	block->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER | UI_FLAG_DRAW_TEXT | UI_FLAG_INTERACTABLE;
	block->hash = hash;
	block->string = string;
	block->uiFont = appState->defaultUiFont;
	block->uiBlockColors.frontColor = COLORU32_BLACK;
	block->uiBlockColors.borderColor = (active) ? COLORU32_BLACK: COLORU32_GRAY;
	block->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
	block->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
	block->uiTextAlignTypes[UI_AXIS_X] = UI_TEXT_ALIGN_CENTER;
	block->uiTextAlignTypes[UI_AXIS_Y] = UI_TEXT_ALIGN_CENTER;

	ColorU32 baseColor = BRUSH_EFFECT_COLORS_PRIMARY[brushEffect];

	ColorU32 colors[INTERACTION_STATE_COUNT];
	colors[INTERACTION_STATE_NONACTIVE_NEUTRAL] = AddConstantToColor(baseColor, -50);
	colors[INTERACTION_STATE_NONACTIVE_HOVERED] = AddConstantToColor(baseColor, -20);
	colors[INTERACTION_STATE_DOWN] = AddConstantToColor(baseColor, -100);
	colors[INTERACTION_STATE_ACTIVE_NEUTRAL] = AddConstantToColor(baseColor, 0);
	colors[INTERACTION_STATE_ACTIVE_HOVERED] = AddConstantToColor(baseColor, 10);
	INTERACTION_STATE interactionState = GetInteractionState(hash, uiInteractionHashes, active, isDisabled, downOverride);
	block->uiBlockColors.backColor = colors[interactionState];

	b32 result = (hash == uiInteractionHashes->hashMousePressed);
	return result;
}

b32 WidgetToolButton(UiState *uiState, AppState *appState, UiInteractionHashes *uiInteractionHashes, BADPAINT_TOOL_TYPE badpaintToolType, COMMAND command)
{
	UiBlock *b = UiCreateBlock(uiState);
	b->hash = Murmur3String("badpainttool", badpaintToolType);
	b->flags = UI_FLAG_DRAW_TEXTURE | UI_FLAG_INTERACTABLE;
	b->uiSizes[UI_AXIS_X] = {UI_SIZE_TEXTURE};
	b->uiSizes[UI_AXIS_Y] = {UI_SIZE_TEXTURE};
	b32 active = appState->currentTool == badpaintToolType;
	b32 isDisabled = false;
	b32 downOverride = IsCommandKeyBindingDown(command);
	INTERACTION_STATE interactionState = GetInteractionState(b->hash, uiInteractionHashes, active, isDisabled, downOverride);
	b->uiTextureView = appState->tools[badpaintToolType].uiTextureViews[interactionState];

	b32 result = (b->hash == uiInteractionHashes->hashMousePressed);
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

void BuildPanelTree(UiState *uiState, AppState *appState, UiInteractionHashes *uiInteractionHashes, UiPanel *uiPanel)
{
	if (uiPanel)
	{
		if (uiPanel->firstChild)
		{
			UiBlock *panelBlock = UiCreateBlock(uiState);
			panelBlock->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
			panelBlock->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
			if (uiPanel->parent)
			{
				panelBlock->uiSizes[uiPanel->parent->childSplitAxis] = {UI_SIZE_PERCENT_OF_PARENT, uiPanel->percentOfParent};
			}
			panelBlock->uiChildLayoutType = UI_CHILD_LAYOUT_LEFT_TO_RIGHT;
			if (uiPanel->childSplitAxis == UI_AXIS_Y)
			{
				panelBlock->uiChildLayoutType = UI_CHILD_LAYOUT_TOP_TO_BOTTOM;
			}
			UI_PARENT_SCOPE(uiState, panelBlock)
			{
				BuildPanelTree(uiState, appState, uiInteractionHashes, uiPanel->firstChild);
			}
		}
		else
		{
			UiBlock *panelBlock = UiCreateBlock(uiState);
			panelBlock->uiSizes[UI_AXIS_X] = {UI_SIZE_FILL};
			panelBlock->uiSizes[UI_AXIS_Y] = {UI_SIZE_FILL};
			if (uiPanel->parent)
			{
				panelBlock->uiSizes[uiPanel->parent->childSplitAxis] = {UI_SIZE_PERCENT_OF_PARENT, uiPanel->percentOfParent};
			}
			UI_PARENT_SCOPE(uiState, panelBlock)
			{
				switch(uiPanel->uiPanelType)
				{
					case UI_PANEL_TYPE_NULL:
					{
						panelBlock->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_TEXT;
						panelBlock->uiBlockColors.backColor = ColorU32{125, 125, 125, 255};
						panelBlock->uiBlockColors.frontColor = ColorU32{0, 0, 0, 255};
						panelBlock->uiTextAlignTypes[UI_AXIS_X] = UI_TEXT_ALIGN_CENTER;
						panelBlock->uiTextAlignTypes[UI_AXIS_Y] = UI_TEXT_ALIGN_CENTER;
						panelBlock->string = STRING("EMPTY PANEL");
						panelBlock->uiFont = appState->defaultUiFont;
					} break;
					case UI_PANEL_TYPE_FINAL_TEXTURE:
					{
						UiBlock *b= UiCreateBlock(uiState);
						//b->hash = leftPanelHash;
						b->flags = UI_FLAG_DRAW_BACKGROUND;
						b->uiSizes[UI_AXIS_X] = {UI_SIZE_FILL};
						b->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
						b->uiBlockColors.backColor = ColorU32{100, 100, 100, 255};
						b->uiChildAlignTypes[UI_AXIS_X] = UI_CHILD_ALIGN_CENTER;
						b->uiChildAlignTypes[UI_AXIS_Y] = UI_CHILD_ALIGN_CENTER;
						UI_PARENT_SCOPE(uiState, b)
						{
							UiBlock *finalTexture = UiCreateBlock(uiState);
							finalTexture->flags = UI_FLAG_DRAW_TEXTURE | UI_FLAG_INTERACTABLE;
							finalTexture->hash = Murmur3String("finalTexture", (u32) (u64) uiPanel);
							finalTexture->uiTextureView = UiRaylibTextureToUiTextureView(&appState->loadedTexture);
							finalTexture->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
							finalTexture->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_OTHER_AXIS, SafeDivideI32(appState->loadedTexture.height, appState->loadedTexture.width)};
						}
					} break;
					case UI_PANEL_TYPE_CANVAS:
					{
						panelBlock->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_TEXT;
						panelBlock->uiBlockColors.backColor = ColorU32{100, 0, 0, 255};
						panelBlock->uiBlockColors.frontColor = ColorU32{0, 0, 0, 255};
						panelBlock->uiTextAlignTypes[UI_AXIS_X] = UI_TEXT_ALIGN_CENTER;
						panelBlock->uiTextAlignTypes[UI_AXIS_Y] = UI_TEXT_ALIGN_CENTER;
						panelBlock->string = STRING("CANVAS");
						panelBlock->uiFont = appState->defaultUiFont;
					} break;
					case UI_PANEL_TYPE_LAYERS:
					{
						panelBlock->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_TEXT;
						panelBlock->uiBlockColors.backColor = ColorU32{100, 100, 100, 255};
						panelBlock->uiBlockColors.frontColor = ColorU32{0, 0, 0, 255};
						panelBlock->uiTextAlignTypes[UI_AXIS_X] = UI_TEXT_ALIGN_CENTER;
						panelBlock->uiTextAlignTypes[UI_AXIS_Y] = UI_TEXT_ALIGN_CENTER;
						panelBlock->string = STRING("LAYERS");
						panelBlock->uiFont = appState->defaultUiFont;
					} break;
					InvalidDefaultCase;
				}
			}
		}
		BuildPanelTree(uiState, appState, uiInteractionHashes, uiPanel->next);
	}
}

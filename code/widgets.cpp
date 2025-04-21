
#include "widgets.h"
#include "vn_math_external.h"

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


#include "widgets.h"
#include "vn_math_external.h"
#include "ui/ui_raylib.h"
#include "main.h"

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

UiBlock *WidgetBrushEffectButton(UiState *uiState, AppState *appState, FrameState *frameState, BADPAINT_PIXEL_TYPE brushEffect, String string, COMMAND command)
{
	u32 hash = Murmur3String("brushEffect", brushEffect);
	b32 active = appState->currentBadpaintPixelType == brushEffect;
	b32 isDisabled = false;
	b32 downOverride = IsCommandKeyBindingDown(command);

	UiBlock *result = UiCreateBlock(uiState);
	result->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER | UI_FLAG_DRAW_TEXT | UI_FLAG_INTERACTABLE;
	result->hash = hash;
	result->string = string;
	result->uiFont = appState->defaultUiFont;
	result->uiBlockColors.frontColor = COLORU32_BLACK;
	result->uiBlockColors.borderColor = (active) ? COLORU32_BLACK: COLORU32_GRAY;
	result->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
	result->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
	result->uiTextAlignTypes[UI_AXIS_X] = UI_TEXT_ALIGN_CENTER;
	result->uiTextAlignTypes[UI_AXIS_Y] = UI_TEXT_ALIGN_CENTER;

	ColorU32 baseColor = BADPAINT_PIXEL_TYPE_COLORS_PRIMARY[brushEffect];

	ColorU32 colors[INTERACTION_STATE_COUNT];
	colors[INTERACTION_STATE_NONACTIVE_NEUTRAL] = AddConstantToColor(baseColor, -50);
	colors[INTERACTION_STATE_NONACTIVE_HOVERED] = AddConstantToColor(baseColor, -20);
	colors[INTERACTION_STATE_DOWN] = AddConstantToColor(baseColor, -100);
	colors[INTERACTION_STATE_ACTIVE_NEUTRAL] = AddConstantToColor(baseColor, 0);
	colors[INTERACTION_STATE_ACTIVE_HOVERED] = AddConstantToColor(baseColor, 10);
	INTERACTION_STATE interactionState = GetInteractionState(hash, &frameState->uiInteractionHashes, active, isDisabled, downOverride);
	result->uiBlockColors.backColor = colors[interactionState];

	if (result->hash == frameState->uiInteractionHashes.hashMousePressed)
	{
		AppCommand *appCommand = PushAppCommand(&frameState->appCommandBuffer);
		appCommand->command = command;
	}
	return result;
}

UiBlock *WidgetToolButton(UiState *uiState, AppState *appState, FrameState *frameState, BADPAINT_TOOL_TYPE badpaintToolType, COMMAND command)
{
	UiBlock *result = UiCreateBlock(uiState);
	result->hash = Murmur3String("badpainttool", badpaintToolType);
	result->flags = UI_FLAG_DRAW_TEXTURE | UI_FLAG_INTERACTABLE;
	result->uiSizes[UI_AXIS_X] = {UI_SIZE_TEXTURE};
	result->uiSizes[UI_AXIS_Y] = {UI_SIZE_TEXTURE};
	b32 active = appState->currentTool == badpaintToolType;
	b32 isDisabled = false;
	b32 downOverride = IsCommandKeyBindingDown(command);
	INTERACTION_STATE interactionState = GetInteractionState(result->hash, &frameState->uiInteractionHashes, active, isDisabled, downOverride);
	result->uiTextureView = appState->tools[badpaintToolType].uiTextureViews[interactionState];
	if (result->hash == frameState->uiInteractionHashes.hashMousePressed)
	{
		AppCommand *appCommand = PushAppCommand(&frameState->appCommandBuffer);
		appCommand->command = command;
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

void DrawToolInCanvasOnPanelAndChildren(UiState *uiState, AppState *appState, v2 posInDrawnCanvas)
{
	UiPanel *stack[1024];
	i32 stackIndex = 0;
	stack[stackIndex] = &appState->rootUiPanel;
	while (stackIndex >= 0)
	{
		UiPanel *uiPanel = stack[stackIndex--];

		if (uiPanel->uiPanelType == UI_PANEL_TYPE_ROOT_IMAGE)
		{
			u32 hash = Murmur3String("root", uiPanel->hash);
			UiBlock *canvasBlockPrev = UiGetBlockOfHashLastFrame(uiState, hash);
			if (canvasBlockPrev->hash)
			{
				v2 posTopLeft = (posInDrawnCanvas - (appState->toolSize * 0.5f));
				v2 normalizedPos;
				normalizedPos.x = SafeDivideF32(posTopLeft.x, (f32) appState->canvas.badpaintPixelsRootImage.dim.x);
				normalizedPos.y = SafeDivideF32(posTopLeft.y, (f32) appState->canvas.badpaintPixelsRootImage.dim.y);
				v2 absolutePos = (normalizedPos * canvasBlockPrev->rect.dim) + canvasBlockPrev->rect.pos;

				f32 sizeNormalized = SafeDivideF32((f32)appState->toolSize, (f32) appState->canvas.badpaintPixelsRootImage.dim.x);
				f32 sizeAbsolute = sizeNormalized * canvasBlockPrev->rect.dim.x;

				UiBlock *t = UiCreateRootBlock(uiState);
				t->flags = UI_FLAG_DRAW_BACKGROUND;
				t->uiPosition[UI_AXIS_X] = {UI_POSITION_ABSOLUTE, absolutePos.x};
				t->uiPosition[UI_AXIS_Y] = {UI_POSITION_ABSOLUTE, absolutePos.y};
				t->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, (f32) sizeAbsolute};
				t->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, (f32) sizeAbsolute};
				t->uiBlockColors.backColor = BADPAINT_PIXEL_TYPE_COLORS_PROCESSING[appState->currentBadpaintPixelType];
			}
		}
		if (uiPanel->uiPanelType == UI_PANEL_TYPE_PNG_FILTERED)
		{
			u32 hash = Murmur3String("canvas", uiPanel->hash);
			UiBlock *canvasBlockPrev = UiGetBlockOfHashLastFrame(uiState, hash);
			if (canvasBlockPrev->hash)
			{
				v2 posTopLeft = (posInDrawnCanvas - (appState->toolSize * 0.5f));
				v2 normalizedPos;
				normalizedPos.x = SafeDivideF32(posTopLeft.x, (f32) appState->canvas.badpaintPixelsRootImage.dim.x);
				normalizedPos.y = SafeDivideF32(posTopLeft.y, (f32) appState->canvas.badpaintPixelsRootImage.dim.y);
				v2 absolutePos = (normalizedPos * canvasBlockPrev->rect.dim) + canvasBlockPrev->rect.pos;

				f32 sizeNormalized = SafeDivideF32((f32)appState->toolSize, (f32) appState->canvas.badpaintPixelsRootImage.dim.x);
				f32 sizeAbsolute = sizeNormalized * canvasBlockPrev->rect.dim.x;

				UiBlock *t = UiCreateRootBlock(uiState);
				t->flags = UI_FLAG_DRAW_BACKGROUND;
				t->uiPosition[UI_AXIS_X] = {UI_POSITION_ABSOLUTE, absolutePos.x};
				t->uiPosition[UI_AXIS_Y] = {UI_POSITION_ABSOLUTE, absolutePos.y};
				t->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, (f32) sizeAbsolute};
				t->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, (f32) sizeAbsolute};
				t->uiBlockColors.backColor = BADPAINT_PIXEL_TYPE_COLORS_PROCESSING[appState->currentBadpaintPixelType];
			}
		}
		if (uiPanel->uiPanelType == UI_PANEL_TYPE_FINAL_IMAGE)
		{
			u32 hash = Murmur3String("finalTexture", uiPanel->hash);
			UiBlock *canvasBlockPrev = UiGetBlockOfHashLastFrame(uiState, hash);
			if (canvasBlockPrev->hash)
			{
				v2 posTopLeft = (posInDrawnCanvas - (appState->toolSize * 0.5f));
				v2 normalizedPos;
				normalizedPos.x = SafeDivideF32(posTopLeft.x, (f32) appState->canvas.badpaintPixelsRootImage.dim.x);
				normalizedPos.y = SafeDivideF32(posTopLeft.y, (f32) appState->canvas.badpaintPixelsRootImage.dim.y);
				v2 absolutePos = (normalizedPos * canvasBlockPrev->rect.dim) + canvasBlockPrev->rect.pos;

				f32 sizeNormalized = SafeDivideF32((f32)appState->toolSize, (f32) appState->canvas.badpaintPixelsRootImage.dim.x);
				f32 sizeAbsolute = sizeNormalized * canvasBlockPrev->rect.dim.x;

				UiBlock *t = UiCreateRootBlock(uiState);
				t->flags = UI_FLAG_DRAW_BACKGROUND;
				t->uiPosition[UI_AXIS_X] = {UI_POSITION_ABSOLUTE, absolutePos.x};
				t->uiPosition[UI_AXIS_Y] = {UI_POSITION_ABSOLUTE, absolutePos.y};
				t->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, (f32) sizeAbsolute};
				t->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, (f32) sizeAbsolute};
				t->uiBlockColors.backColor = ColorU32{0, 0, 0, 100};
			}
		}

		for (UiPanel *child = uiPanel->lastChild; child; child = child->prev)
		{
			if (ASSERT(stackIndex < ARRAY_COUNT(stack)))
			{
				stack[++stackIndex] = child;
			}
		}
	}

}

v2 ScreenPosToCanvasPos(iv2 mousePos, RectV2 *blockRect, iv2 canvasDim)
{
	v2 relativeMousePos = mousePos - blockRect->pos;
	v2 normalizedPos;
	normalizedPos.x = SafeDivideF32(relativeMousePos.x, blockRect->dim.x);
	normalizedPos.y = SafeDivideF32(relativeMousePos.y, blockRect->dim.y);
	v2 result = normalizedPos * canvasDim;
	return result;
}

void ProcessActiveInputInDrawableArea(UiState *uiState, AppState *appState, FrameState *frameState, UiBlock *drawableBlock, UI_PANEL_TYPE uiPanelType)
{
	UiBlock *drawableBlockPrev = UiGetBlockOfHashLastFrame(uiState, drawableBlock->hash);
	if (drawableBlockPrev->hash)
	{
		v2 posInCanvas = ScreenPosToCanvasPos(frameState->mousePixelPos, &drawableBlockPrev->rect, appState->canvas.badpaintPixelsRootImage.dim);
		DrawToolInCanvasOnPanelAndChildren(uiState, appState, posInCanvas);
		if (drawableBlock->hash == frameState->uiInteractionHashes.hashMouseDown)
		{
			v2 posInCanvasPrevious = ScreenPosToCanvasPos(frameState->mousePixelPosPrevious, &drawableBlockPrev->rect, appState->canvas.badpaintPixelsRootImage.dim);
			AppCommand *appCommand = PushAppCommand(&frameState->appCommandBuffer);
			appCommand->command = COMMAND_PAINT_ON_CANVAS_BETWEEN_POSITIONS;
			appCommand->value1V2 = posInCanvasPrevious;
			appCommand->value2V2 = posInCanvas;
			appCommand->value3U32 = uiPanelType;
		}
	}

}

void WidgetImageCanvas(UiState *uiState, AppState *appState, FrameState *frameState, UiPanel *uiPanel, u32 hash,
		TextureGPU *textureGPUImage, TextureGPU *textureGPUDrawable)
{
	if (textureGPUImage->texture.id)
	{
		UiBlock *b = UiCreateBlock(uiState);
		b->flags = UI_FLAG_DRAW_TEXTURE;
		b->uiTextureView = UiRaylibTextureToUiTextureView(&textureGPUImage->texture);
		b->uiSizes[UI_AXIS_X] = {UI_SIZE_FILL_FIXED, SafeDivideI32(b->uiTextureView.dim.x, b->uiTextureView.dim.y)};
		b->uiSizes[UI_AXIS_Y] = {UI_SIZE_FILL_FIXED, SafeDivideI32(b->uiTextureView.dim.y, b->uiTextureView.dim.x)};
		UI_PARENT_SCOPE(uiState, b)
		{
			UiBlock *canvasBlock = UiCreateBlock(uiState);
			canvasBlock->flags = UI_FLAG_DRAW_TEXTURE | UI_FLAG_INTERACTABLE;
			canvasBlock->hash = hash;
			canvasBlock->uiTextureView = UiRaylibTextureToUiTextureView(&textureGPUDrawable->texture);
			canvasBlock->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
			canvasBlock->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
			//TODO: (Ahmayk) This check needs to be more sophisticated regarding drawing just outside the canvas
			if (canvasBlock->hash == frameState->uiInteractionHashes.hashMouseHover)
			{
				ProcessActiveInputInDrawableArea(uiState, appState, frameState, canvasBlock, uiPanel->uiPanelType);
			}
		}
	}
}

void BuildPanelTree(UiState *uiState, AppState *appState, FrameState *frameState, UiPanel *uiPanel)
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
				BuildPanelTree(uiState, appState, frameState, uiPanel->firstChild);
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
				Canvas *canvas = &appState->canvas;
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
					case UI_PANEL_TYPE_ROOT_IMAGE:
					{
						panelBlock->uiChildAlignTypes[UI_AXIS_X] = UI_CHILD_ALIGN_CENTER;
						panelBlock->uiChildAlignTypes[UI_AXIS_Y] = UI_CHILD_ALIGN_CENTER;
 						u32 hash = Murmur3String("root", uiPanel->hash);
						WidgetImageCanvas(uiState, appState, frameState, uiPanel, hash, &canvas->textureGPURoot, &canvas->badpaintPixelsRootImage.textureGPU);
					} break;
					case UI_PANEL_TYPE_FINAL_IMAGE:
					{
						panelBlock->uiChildAlignTypes[UI_AXIS_X] = UI_CHILD_ALIGN_CENTER;
						panelBlock->uiChildAlignTypes[UI_AXIS_Y] = UI_CHILD_ALIGN_CENTER;
						u32 hash = Murmur3String("finalTexture", uiPanel->hash);
						WidgetImageCanvas(uiState, appState, frameState, uiPanel, hash, &canvas->textureGPUFinal, &canvas->badpaintPixelsFinalImage.textureGPU);
					} break;
					case UI_PANEL_TYPE_PNG_FILTERED:
					{

						panelBlock->uiChildAlignTypes[UI_AXIS_X] = UI_CHILD_ALIGN_CENTER;
						panelBlock->uiChildAlignTypes[UI_AXIS_Y] = UI_CHILD_ALIGN_CENTER;
						if (!appState->imageIsBroken)
						{
							u32 hash = Murmur3String("canvas", uiPanel->hash);
							WidgetImageCanvas(uiState, appState, frameState, uiPanel, hash, &canvas->textureGPUPNGFiltered, &canvas->badpaintPixelsPNGFiltered.textureGPU);
						}
						else
						{
							UiBlock *canvasBlock = UiCreateBlock(uiState);
							canvasBlock->flags = UI_FLAG_DRAW_TEXT;
							canvasBlock->uiSizes[UI_AXIS_X] = {UI_SIZE_TEXT};
							canvasBlock->uiSizes[UI_AXIS_Y] = {UI_SIZE_TEXT};
							canvasBlock->string = STRING("Congratulations! You broke the image. (Press Ctrl-Z to undo)");
							canvasBlock->uiFont = appState->defaultUiFont;
							canvasBlock->uiBlockColors.frontColor = COLORU32_BLACK;
						}
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
		BuildPanelTree(uiState, appState, frameState, uiPanel->next);
	}
}

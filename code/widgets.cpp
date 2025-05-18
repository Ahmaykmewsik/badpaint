
#include "widgets.h"
#include "vn_math_external.h"
#include "ui/ui_raylib.h"
#include "main.h"

UiBlock *WidgetBrushEffectButton(UiState *uiState, AppState *appState, AppCommandBuffer *appCommandBuffer, BADPAINT_PIXEL_TYPE brushEffect, String string, COMMAND command)
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
	result->padding = UiPadding{4, 4, 4, 4};

	ColorU32 baseColor = BADPAINT_PIXEL_TYPE_COLORS_PRIMARY[brushEffect];

	ColorU32 colors[INTERACTION_STATE_COUNT];
	colors[INTERACTION_STATE_NONACTIVE_NEUTRAL] = AddConstantToColor(baseColor, -50);
	colors[INTERACTION_STATE_NONACTIVE_HOVERED] = AddConstantToColor(baseColor, -20);
	colors[INTERACTION_STATE_DOWN] = AddConstantToColor(baseColor, -100);
	colors[INTERACTION_STATE_ACTIVE_NEUTRAL] = AddConstantToColor(baseColor, 0);
	colors[INTERACTION_STATE_ACTIVE_HOVERED] = AddConstantToColor(baseColor, 10);
	INTERACTION_STATE interactionState = GetInteractionState(&uiState->uiInteractionState, hash, active, isDisabled, downOverride);
	result->uiBlockColors.backColor = colors[interactionState];

	if (result->hash == uiState->uiInteractionState.hashMousePressed)
	{
		AppCommand *appCommand = PushAppCommand(appCommandBuffer);
		appCommand->command = command;
	}
	return result;
}

UiBlock *WidgetToolButton(UiState *uiState, AppState *appState, AppCommandBuffer *appCommandBuffer, String label, BADPAINT_TOOL_TYPE badpaintToolType, COMMAND command)
{
	UiBlock *result = UiCreateBlock(uiState);
	result->hash = Murmur3String("badpainttool", badpaintToolType);
	result->flags = UI_FLAG_DRAW_TEXTURE | UI_FLAG_DRAW_TEXT | UI_FLAG_INTERACTABLE;
	result->uiSizes[UI_AXIS_X] = {UI_SIZE_TEXTURE};
	result->uiSizes[UI_AXIS_Y] = {UI_SIZE_TEXTURE};
	result->padding = UiPadding{8, 8, 8, 8};
	b32 active = appState->currentTool == badpaintToolType;
	b32 isDisabled = false;
	b32 downOverride = IsCommandKeyBindingDown(command);
	INTERACTION_STATE interactionState = GetInteractionState(&uiState->uiInteractionState, result->hash, active, isDisabled, downOverride);
	result->uiTextureView = appState->tools[BADPAINT_TOOL_TEST].uiTextureViews[interactionState];
	result->string = label;
	result->uiFont = appState->defaultUiFont;
	result->uiBlockColors.frontColor = COLORU32_BLACK;
	result->uiTextAlignTypes[UI_AXIS_X] = UI_TEXT_ALIGN_CENTER;
	result->uiTextAlignTypes[UI_AXIS_Y] = UI_TEXT_ALIGN_CENTER;

	if (result->hash == uiState->uiInteractionState.hashMousePressed)
	{
		AppCommand *appCommand = PushAppCommand(appCommandBuffer);
		appCommand->command = command;
	}
	return result;
}

void DrawToolInCanvasOnPanelAndChildren(UiState *uiState, AppState *appState, v2 posInDrawnCanvas, u32 hoveredUiPanelType)
{
	UiPanel *stack[1024];
	i32 stackIndex = 0;
	stack[stackIndex] = &appState->rootUiPanel;
	while (stackIndex >= 0)
	{
		UiPanel *uiPanel = stack[stackIndex--];

		b32 isHovered = uiPanel->uiPanelType == hoveredUiPanelType;

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
			t->uiBlockColors.backColor = ColorU32{0, 0, 0, 100};
			t->depthLayer = UI_APP_DEPTH_LAYER_CURSOR_GUI; 
			if (isHovered)
			{
				t->uiBlockColors.backColor = BADPAINT_PIXEL_TYPE_COLORS_PROCESSING[appState->currentBadpaintPixelType];
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

struct CanvasPanelState
{
	v2 scrollOffset;
	f32 zoomAmount;
	f32 badpaintImageAlpha;
};

void WidgetImageCanvas(UiState *uiState, AppState *appState, AppCommandBuffer *appCommandBuffer, UiPanel *uiPanel, UiBlock *rootPanelBlock,
		TextureGPU *textureGPUImage, TextureGPU *textureGPUDrawable)
{
	if (textureGPUImage->texture.id)
	{
		UiBlock *underWhite = UiCreateBlock(uiState);
		underWhite->flags = UI_FLAG_DRAW_BACKGROUND;
		underWhite->uiBlockColors.backColor = COLORU32_WHITE;
		underWhite->uiSizes[UI_AXIS_X] = {UI_SIZE_FILL_FIXED, SafeDivideI32(textureGPUImage->dim.x, textureGPUImage->dim.y)};
		underWhite->uiSizes[UI_AXIS_Y] = {UI_SIZE_FILL_FIXED, SafeDivideI32(textureGPUImage->dim.y, textureGPUImage->dim.x)};

		u32 canvasBlockHash = Murmur3String("canvasBlock", rootPanelBlock->hash);
		UiBlock *rootPanelBlockPrev = UiGetBlockOfHashLastFrame(uiState, rootPanelBlock->hash);
		UiBlock *canvasBlockPrev = UiGetBlockOfHashLastFrame(uiState, canvasBlockHash);

		CanvasPanelState *canvasPanelState = (CanvasPanelState*) UiGetOrAllocateWidgetMemory(uiState, rootPanelBlock->hash, sizeof(CanvasPanelState));
		if (rootPanelBlockPrev->hash && !canvasPanelState->zoomAmount)
		{
			if (rootPanelBlockPrev->rect.dim.x < rootPanelBlockPrev->rect.dim.y)
			{
				canvasPanelState->zoomAmount = SafeDivideF32(rootPanelBlockPrev->rect.dim.x, (f32) textureGPUImage->dim.x);
			}
			else
			{
				canvasPanelState->zoomAmount = SafeDivideF32(rootPanelBlockPrev->rect.dim.y, (f32) textureGPUImage->dim.y);
			}
		}

		iv2 mousePos = uiState->uiInteractionState.uiInteractionFrameInput.mousePixelPos;
		if (rootPanelBlockPrev->hash && IsInRectV2(mousePos, rootPanelBlockPrev->rect))
		{
			if (!IsZeroV2(uiState->uiInteractionState.uiInteractionFrameInput.mouseWheelDelta))
			{
				f32 oldZoom = canvasPanelState->zoomAmount;
				v2 delta = uiState->uiInteractionState.uiInteractionFrameInput.mouseWheelDelta;
				canvasPanelState->zoomAmount *= PowF32(1.2f, delta.x + delta.y);

				f32 minPixelSize = 50;
				f32 dimX = textureGPUImage->dim.x * canvasPanelState->zoomAmount;
				if (minPixelSize > dimX)
				{
					canvasPanelState->zoomAmount *= minPixelSize / dimX;
				}
				f32 dimY = textureGPUImage->dim.y * canvasPanelState->zoomAmount;
				if (minPixelSize > dimY)
				{
					canvasPanelState->zoomAmount *= minPixelSize / dimY;
				}

				canvasPanelState->zoomAmount = MinF32(canvasPanelState->zoomAmount, 200.0f);

				//NOTE: (Ahmayk) Assumes that canvas block and underWhite have same pos
				if (canvasBlockPrev->hash)
				{
					v2 relativeMousePos = mousePos - canvasBlockPrev->rect.pos;
					v2 mousePosDelta = relativeMousePos - (relativeMousePos * (canvasPanelState->zoomAmount / oldZoom));
					canvasPanelState->scrollOffset += mousePosDelta;
				}
			}
		}
		underWhite->uiPosition[UI_AXIS_X] = UiPosition{UI_POSITION_RELATIVE, canvasPanelState->scrollOffset.x};
		underWhite->uiPosition[UI_AXIS_Y] = UiPosition{UI_POSITION_RELATIVE, canvasPanelState->scrollOffset.y};

		if (canvasPanelState->zoomAmount)
		{
			v2 textureSize = textureGPUImage->dim * canvasPanelState->zoomAmount;
			underWhite->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, textureSize.x};
			underWhite->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, textureSize.y};
		}

		UI_PARENT_SCOPE(uiState, underWhite)
		{
			UiBlock *b = UiCreateBlock(uiState);
			b->flags = UI_FLAG_DRAW_TEXTURE;
			b->uiTextureView = UiRaylibTextureToUiTextureView(&textureGPUImage->texture);
			b->uiSizes[UI_AXIS_X] = {UI_SIZE_FILL};
			b->uiSizes[UI_AXIS_Y] = {UI_SIZE_FILL};
			UI_PARENT_SCOPE(uiState, b)
			{
				UiBlock *canvasBlock = UiCreateBlock(uiState);
				canvasBlock->flags = UI_FLAG_INTERACTABLE;
				canvasBlock->hash = canvasBlockHash;
				canvasBlock->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
				canvasBlock->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
				//TODO: (Ahmayk) This check needs to be more sophisticated regarding drawing just outside the canvas

				b32 isHoveringCanvas = (canvasBlock->hash == uiState->uiInteractionState.hashMouseHover &&
					(uiState->uiInteractionState.lastPressedUiHash == 0 || uiState->uiInteractionState.lastPressedUiHash == canvasBlock->hash));

				if (isHoveringCanvas)
				{
					canvasPanelState->badpaintImageAlpha = ClampF32(0, canvasPanelState->badpaintImageAlpha + (GetFrameTime() * (1000 / 80)) ,1);
				}
				else
				{
					canvasPanelState->badpaintImageAlpha = ClampF32(0, canvasPanelState->badpaintImageAlpha - (GetFrameTime() * (1000 / 80)) ,1);
				}

				if (canvasPanelState->badpaintImageAlpha > 0)
				{
					canvasBlock->flags |= UI_FLAG_DRAW_TEXTURE | UI_FLAG_TINT_TEXTURE;
					canvasBlock->uiTextureView = UiRaylibTextureToUiTextureView(&textureGPUDrawable->texture);
					canvasBlock->uiBlockColors.frontColor = ColorU32{255, 255, 255, (u8) (canvasPanelState->badpaintImageAlpha * 255)};

					b->flags |= UI_FLAG_TINT_TEXTURE;
					u8 value = (u8) LerpF32(225, 1 - canvasPanelState->badpaintImageAlpha, 255);
					u8 valueAlpha = (u8) LerpF32(245, 1 - canvasPanelState->badpaintImageAlpha, 255);
					b->uiBlockColors.frontColor = ColorU32{value, value, value, valueAlpha};
				}

				if (isHoveringCanvas)
				{
					if (canvasBlockPrev->hash)
					{
						v2 posInCanvas = ScreenPosToCanvasPos(mousePos, &canvasBlockPrev->rect, appState->canvas.badpaintPixelsRootImage.dim);
						//DrawToolInCanvasOnPanelAndChildren(uiState, appState, posInCanvas, uiPanel->uiPanelType);

						if (uiState->uiInteractionState.lastPressedUiHash == canvasBlockPrev->hash)
						{
							v2 posInCanvasPrevious = ScreenPosToCanvasPos(uiState->uiInteractionState.mousePixelPosPrevious, &canvasBlockPrev->rect, appState->canvas.badpaintPixelsRootImage.dim);
							AppCommand *appCommand = PushAppCommand(appCommandBuffer);
							appCommand->command = COMMAND_PAINT_ON_CANVAS_BETWEEN_POSITIONS;
							appCommand->value1V2 = posInCanvasPrevious;
							appCommand->value2V2 = posInCanvas;
							appCommand->value3U32 = uiPanel->uiPanelType;
						}
					}
				}

			}
		}
	}

}

void BuildPanelTree(UiState *uiState, AppState *appState, AppCommandBuffer *appCommandBuffer, UiPanel *uiPanel)
{
	if (uiPanel)
	{
		UiBlock *panelBlock = UiCreateBlock(uiState);
		if (uiPanel->firstChild)
		{
			panelBlock->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
			panelBlock->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
			panelBlock->hash = Murmur3String("parentPanel", uiPanel->hash);
			if (uiPanel->parent)
			{
				panelBlock->uiSizes[uiPanel->parent->childSplitAxis] = {UI_SIZE_PERCENT_OF_PARENT, uiPanel->percentOfParent};
			}
			if (uiPanel->childSplitAxis == UI_AXIS_X)
			{
				panelBlock->uiChildLayoutType = UI_CHILD_LAYOUT_LEFT_TO_RIGHT;
			}
			if (uiPanel->childSplitAxis == UI_AXIS_Y)
			{
				panelBlock->uiChildLayoutType = UI_CHILD_LAYOUT_TOP_TO_BOTTOM;
			}
			UI_PARENT_SCOPE(uiState, panelBlock)
			{
				BuildPanelTree(uiState, appState, appCommandBuffer, uiPanel->firstChild);
			}
		}
		else
		{
			panelBlock->uiSizes[UI_AXIS_X] = {UI_SIZE_FILL};
			panelBlock->uiSizes[UI_AXIS_Y] = {UI_SIZE_FILL};
			panelBlock->hash = Murmur3String("mainPanel", uiPanel->hash);
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
						panelBlock->hash = Murmur3String("root", uiPanel->hash);
						WidgetImageCanvas(uiState, appState, appCommandBuffer, uiPanel, panelBlock, &canvas->textureGPURoot, &canvas->badpaintPixelsRootImage.textureGPU);
					} break;
					case UI_PANEL_TYPE_FINAL_IMAGE:
					{
						panelBlock->uiChildAlignTypes[UI_AXIS_X] = UI_CHILD_ALIGN_CENTER;
						panelBlock->uiChildAlignTypes[UI_AXIS_Y] = UI_CHILD_ALIGN_CENTER;
						panelBlock->hash = Murmur3String("finalTexture", uiPanel->hash);
						WidgetImageCanvas(uiState, appState, appCommandBuffer, uiPanel, panelBlock, &canvas->textureGPUFinal, &canvas->badpaintPixelsFinalImage.textureGPU);
					} break;
					case UI_PANEL_TYPE_PNG_FILTERED:
					{
						panelBlock->uiChildAlignTypes[UI_AXIS_X] = UI_CHILD_ALIGN_CENTER;
						panelBlock->uiChildAlignTypes[UI_AXIS_Y] = UI_CHILD_ALIGN_CENTER;
						panelBlock->hash = Murmur3String("canvas", uiPanel->hash);
						if (!appState->imageIsBroken)
						{
							WidgetImageCanvas(uiState, appState, appCommandBuffer, uiPanel, panelBlock, &canvas->textureGPUPNGFiltered, &canvas->badpaintPixelsPNGFiltered.textureGPU);
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

		if (uiPanel->parent && uiPanel->next)
		{
			UiBlock *resizeHitbox = UiCreateBlock(uiState);
			resizeHitbox->flags = UI_FLAG_INTERACTABLE;
			resizeHitbox->hash = Murmur3String("resizePanelHitbox", uiPanel->hash);
			resizeHitbox->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
			resizeHitbox->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
			resizeHitbox->uiSizes[uiPanel->parent->childSplitAxis] = {UI_SIZE_PIXELS, 8};
			resizeHitbox->uiPosition[UI_AXIS_X] = {UI_POSITION_PERCENT_OF_PARENT, 0};
			resizeHitbox->uiPosition[UI_AXIS_Y] = {UI_POSITION_PERCENT_OF_PARENT, 0};
			resizeHitbox->uiPositionOffset[uiPanel->parent->childSplitAxis] = {UI_POSITION_OFFSET_PERCENT_OF_SELF, -0.5f};
			UI_PARENT_SCOPE(uiState, resizeHitbox)
			{
				UiBlock *resizeBorder = UiCreateBlock(uiState);
				resizeBorder->flags = UI_FLAG_DRAW_BACKGROUND;
				resizeBorder->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
				resizeBorder->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
				resizeBorder->uiPosition[uiPanel->parent->childSplitAxis] = {UI_POSITION_PERCENT_OF_PARENT, 0.5f};
				resizeBorder->uiSizes[uiPanel->parent->childSplitAxis] = {UI_SIZE_PIXELS, 1};
				resizeBorder->uiBlockColors.backColor = COLORU32_GRAY;
				resizeBorder->depthLayer = UI_APP_DEPTH_LAYER_ABOVE;
				if (uiState->uiInteractionState.hashMouseHover == resizeHitbox->hash || uiState->uiInteractionState.lastPressedUiHash == resizeHitbox->hash)
				{
					resizeBorder->uiBlockColors.backColor = HexToColorU32(0xf2fa00);
					if (uiPanel->parent->childSplitAxis == UI_AXIS_X)
					{
						uiState->uiInteractionState.currentUiCursorType = UI_CURSOR_TYPE_RESIZE_LEFT_RIGHT;
					}
					if (uiPanel->parent->childSplitAxis == UI_AXIS_Y)
					{
						uiState->uiInteractionState.currentUiCursorType = UI_CURSOR_TYPE_RESIZE_UP_DOWN;
					}
				}
				if (uiState->uiInteractionState.lastPressedUiHash == resizeHitbox->hash)
				{
					resizeBorder->uiSizes[uiPanel->parent->childSplitAxis] = {UI_SIZE_PIXELS, 2};
					UiBlock *prevParentPanelBlock = UiGetBlockOfHashLastFrame(uiState, panelBlock->parent->hash);
					if (ASSERT(prevParentPanelBlock->hash))
					{
						f32 relativePosInBox = uiState->uiInteractionState.uiInteractionFrameInput.mousePixelPos.elements[uiPanel->parent->childSplitAxis] - prevParentPanelBlock->rect.pos.elements[uiPanel->parent->childSplitAxis];
						f32 percentOfBoxSize = SafeDivideF32(relativePosInBox, prevParentPanelBlock->rect.dim.elements[uiPanel->parent->childSplitAxis]);
						u32 minSizePixels = 32;
						f32 minSizePercent = SafeDivideF32((f32)minSizePixels, prevParentPanelBlock->rect.dim.elements[uiPanel->parent->childSplitAxis]);
						percentOfBoxSize = ClampF32(0 + minSizePercent, percentOfBoxSize, 1 - minSizePercent);
						f32 oldPercentOfParent = uiPanel->percentOfParent;
						uiPanel->percentOfParent = percentOfBoxSize;
						uiPanel->next->percentOfParent -= (percentOfBoxSize - oldPercentOfParent);
						panelBlock->uiSizes[uiPanel->parent->childSplitAxis].value = uiPanel->percentOfParent;
					}
				}
			}

			f32 percentPositionSum = uiPanel->percentOfParent;
			for (UiPanel *siblingPanel = uiPanel->prev; siblingPanel; siblingPanel= siblingPanel->prev)
			{
				percentPositionSum += siblingPanel->percentOfParent;
			}
			resizeHitbox->uiPosition[uiPanel->parent->childSplitAxis] = {UI_POSITION_PERCENT_OF_PARENT, percentPositionSum};
		}

		BuildPanelTree(uiState, appState, appCommandBuffer, uiPanel->next);
	}
}

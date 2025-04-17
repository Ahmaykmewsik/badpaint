#pragma once

#include "ui/ui.h"

//TODO: (Ahmayk) remove dependency
#include "main.h"
#include "vn_math_external.h"

static UiState G_UI_STATE = {};

//TODO: (Ahmayk) Temporary hack 
UiState *GetUiState()
{
	return &G_UI_STATE;
}

void UiInit(Arena *arena)
{
	G_UI_STATE.uiBuffers[0].arena = ArenaInitFromArena(arena, MegaByte * 5);
	G_UI_STATE.uiBuffers[1].arena = ArenaInitFromArena(arena, MegaByte * 5);
}

UiBlock *CreateUiBlock(UiState *uiState)
{
	UiBlock *result = {};
	UiBuffer *uiBuffer = &uiState->uiBuffers[uiState->uiBufferIndex];
	if (ASSERT(uiBuffer->uiBlockCount < ARRAY_COUNT(uiBuffer->uiBlocks)))
	{
		result = &uiBuffer->uiBlocks[uiBuffer->uiBlockCount++];
		*result = {};
		if (G_UI_STATE.parentStackCount)
		{
			result->parent = G_UI_STATE.parentStack[G_UI_STATE.parentStackCount - 1];
			if (ASSERT(result->parent))
			{
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
			}
		}
	}

	if (!result)
	{
		static UiBlock stub = {};
		result = &stub;
		*result = {};
	}

	return result;
}

bool IsFlag(UiBlock *uiBlock, unsigned int flags)
{
	bool result = uiBlock->flags & flags;
	return result;
}

void PushUiParent()
{
	if (ASSERT(G_UI_STATE.parentStackCount < ARRAY_COUNT(G_UI_STATE.parentStack)))
	{
		UiBuffer *uiBuffer = &G_UI_STATE.uiBuffers[G_UI_STATE.uiBufferIndex];
		G_UI_STATE.parentStack[G_UI_STATE.parentStackCount] = &uiBuffer->uiBlocks[uiBuffer->uiBlockCount - 1];
		G_UI_STATE.parentStackCount++;
	}
}

void PopUiParent()
{
	if (ASSERT(G_UI_STATE.parentStackCount > 0))
	{
		G_UI_STATE.parentStackCount--;
	}
}

void CalculateUiUpwardsDependentSizes(UiBlock *uiBlock)
{
	if (uiBlock)
	{
		if (uiBlock->uiSizes[0].kind == UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT ||
				uiBlock->uiSizes[1].kind == UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT)
		{
			ASSERT(uiBlock->parent);
			ASSERT(uiBlock->uiTexture.dim.x && uiBlock->uiTexture.dim.y);

			float scaleX = 1;
			float scaleY = 1;
			if (uiBlock->parent->rect.dim.x)
				scaleX = uiBlock->parent->rect.dim.x / uiBlock->uiTexture.dim.x;
			if (uiBlock->parent->rect.dim.y)
				scaleY = uiBlock->parent->rect.dim.y / uiBlock->uiTexture.dim.y;

			float scale = MinF32(scaleX, scaleY);

			uiBlock->rect.dim = uiBlock->uiTexture.dim * scale;
		}
		else
		{
			for (int j = 0;
					j < ARRAY_COUNT(uiBlock->uiSizes);
					j++)
			{
				UiSize uiSize = uiBlock->uiSizes[j];
				switch (uiSize.kind)
				{
					case UI_SIZE_KIND_PERCENT_OF_PARENT:
						{
							if (uiBlock->parent && uiBlock->parent->rect.dim.elements[j])
							{
								uiBlock->rect.dim.elements[j] = (uiBlock->parent->rect.dim.elements[j] * uiSize.value);
							}
							break;
						}
					case UI_SIZE_KIND_CHILDREN_OF_SUM:
					case UI_SIZE_KIND_PIXELS:
					case UI_SIZE_KIND_TEXT:
					case UI_SIZE_KIND_TEXTURE:
						break;
						InvalidDefaultCase
				}
			}
		}

		CalculateUiUpwardsDependentSizes(uiBlock->firstChild);
		CalculateUiUpwardsDependentSizes(uiBlock->next);
	}
}

void CalculateUiDownwardsDependentSizes(UiBlock *uiBlock)
{
	if (uiBlock)
	{
		CalculateUiDownwardsDependentSizes(uiBlock->firstChild);
		CalculateUiDownwardsDependentSizes(uiBlock->next);

		bool isHorizontal = IsFlag(uiBlock, UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);

		for (int j = 0;
				j < ARRAY_COUNT(uiBlock->uiSizes);
				j++)
		{
			UiSize uiSize = uiBlock->uiSizes[j];
			switch (uiSize.kind)
			{
				case UI_SIZE_KIND_CHILDREN_OF_SUM:
					{
						float sumOrMaxOfChildren = 0;
						UiBlock *child = uiBlock->firstChild;
						while (child)
						{
							ASSERT(child != uiBlock);

							((j == 0 && isHorizontal) || (j == 1 && !isHorizontal))
								? sumOrMaxOfChildren += child->rect.dim.elements[j]
								: sumOrMaxOfChildren = MaxF32(child->rect.dim.elements[j], sumOrMaxOfChildren);

							child = child->next;
						}
						uiBlock->rect.dim.elements[j] = sumOrMaxOfChildren;

						if (j == ARRAY_COUNT(uiBlock->uiSizes) - 1)
							CalculateUiUpwardsDependentSizes(uiBlock->firstChild);
						break;
					}
				case UI_SIZE_KIND_PERCENT_OF_PARENT:
				case UI_SIZE_KIND_PIXELS:
				case UI_SIZE_KIND_TEXT:
				case UI_SIZE_KIND_TEXTURE:
				case UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT:
					break;
					InvalidDefaultCase
			}
		}
	}
}

void CalculateUiRelativePositions(UiBlock *uiBlock)
{
	if (uiBlock)
	{
		if (IsFlag(uiBlock, UI_FLAG_MANUAL_POSITION) || (!uiBlock->parent || IsFlag(uiBlock->parent, UI_FLAG_CHILDREN_MANUAL_POSITION)))
		{
			uiBlock->computedRelativePixelPos = uiBlock->relativePixelPosition;
		}
		else if (uiBlock->parent && IsFlag(uiBlock, UI_FLAG_CENTER_IN_PARENT))
		{
			uiBlock->computedRelativePixelPos = PositionInCenterV2(uiBlock->parent->rect.dim, uiBlock->rect.dim);
		}
		else if (uiBlock->prev)
		{
			uiBlock->computedRelativePixelPos = uiBlock->prev->computedRelativePixelPos;

			if (uiBlock->parent && IsFlag(uiBlock->parent, UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT))
				uiBlock->computedRelativePixelPos.x += uiBlock->prev->rect.dim.x;
			else
				uiBlock->computedRelativePixelPos.y += uiBlock->prev->rect.dim.y;
		}

		CalculateUiRelativePositions(uiBlock->firstChild);
		CalculateUiRelativePositions(uiBlock->next);
	}
}

void CalculateUiPosGivenReletativePositions(UiBlock *uiBlock)
{
	if (uiBlock)
	{
		if (uiBlock->parent)
			uiBlock->computedRelativePixelPos += uiBlock->parent->computedRelativePixelPos;

		uiBlock->rect.pos = uiBlock->computedRelativePixelPos;

		CalculateUiPosGivenReletativePositions(uiBlock->firstChild);
		CalculateUiPosGivenReletativePositions(uiBlock->next);
	}
}

ColorU32 GetReactiveColorU32(CommandInput *commandInputs, UiBlock *uiBlockLastFrame, UiReactiveColors uiReactiveColors, b32 disabled)
{
	ColorU32 result = uiReactiveColors.neutral;

	if (disabled)
	{
		result = uiReactiveColors.disabled;
	}
	else if (uiBlockLastFrame)
	{
		b32 down = uiBlockLastFrame->down;
		b32 hovered = uiBlockLastFrame->hovered;

		COMMAND command = uiBlockLastFrame->command;
		if (down || (command && IsCommandDown(commandInputs, command)))
		{
			result = uiReactiveColors.down;
		}
		else if (hovered)
		{
			result = uiReactiveColors.hovered;
		}
	}

	return result;
}

UiBlock *GetUiBlockOfHashLastFrame(u32 hash)
{
	UiBlock *result = {};

	UiBuffer *uiBuffer = &G_UI_STATE.uiBuffers[1 - G_UI_STATE.uiBufferIndex];
	for (u32 i = 0; i < ARRAY_COUNT(uiBuffer->uiBlocks); i++)
	{
		if (uiBuffer->uiBlocks[i].hash == hash)
		{
			result = &uiBuffer->uiBlocks[i];
			break;
		}
	}

	if (!result)
	{
		static UiBlock stub = {};
		result = &stub;
		stub = {};
    }

    return result;
}

UiBlock *CreateUiButton(String string, u32 hash, UiFont uiFont, UiReactiveColorStates uiReactiveColorStates, b32 active, b32 disabled)
{
	UiBlock *result = CreateUiBlock(&G_UI_STATE);
	result->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER | UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_CENTERED | UI_FLAG_INTERACTABLE;
	result->hash = hash;
	result->string = string;
	result->uiFont = uiFont;
	result->uiBlockColors.frontColor = COLORU32_BLACK;
	result->uiBlockColors.borderColor = (active) ? COLORU32_BLACK: COLORU32_GRAY;

	UiReactiveColors uiReactiveColors = (active)
		? uiReactiveColorStates.active
		: uiReactiveColorStates.nonActive;
	UiBlock *uiBlockLastFrame = GetUiBlockOfHashLastFrame(hash);
	result->uiBlockColors.backColor = GetReactiveColorU32(G_UI_STATE.commandInputs, uiBlockLastFrame, uiReactiveColors, disabled);
	return result;
}

ColorU32 AddConstantToColor(ColorU32 color, i8 constant)
{
	ColorU32 result = color;
	result.r = (u8) ClampI32(0, result.r + constant, 255);
	result.g = (u8) ClampI32(0, result.g + constant, 255);
	result.b = (u8) ClampI32(0, result.b + constant, 255);
	return result;
}

UiReactiveColorStates CreateButtonUiReactiveColorStates(ColorU32 color)
{
	UiReactiveColorStates result = {};

	result.active.down = AddConstantToColor(color, -100);
	result.active.hovered = AddConstantToColor(color, 10);
	result.active.neutral = color;
	result.nonActive.down = AddConstantToColor(color, -100);
	result.nonActive.hovered = AddConstantToColor(color, -20);
	result.nonActive.neutral = AddConstantToColor(color, -50);
	return result;
}

void UiLayoutBlocks(UiBuffer *uiBuffer)
{
	for (u32 i = 1; i < uiBuffer->uiBlockCount; i++)
	{
		UiBlock *uiBlock = &uiBuffer->uiBlocks[i];
		for (u32 j = 0; j < ARRAY_COUNT(uiBlock->uiSizes); j++)
		{
			UiSize uiSize = uiBlock->uiSizes[j];
			switch (uiSize.kind)
			{
				case UI_SIZE_KIND_TEXTURE:
					{
						//NOTE: (Ahmayk) :(
						uiBlock->rect.dim.elements[j] = (f32) uiBlock->uiTexture.dim.elements[j];
						break;
					}
				case UI_SIZE_KIND_PIXELS:
					{
						uiBlock->rect.dim.elements[j] = uiSize.value;
						break;
					}
				case UI_SIZE_KIND_TEXT:
					{
						if (ASSERT(uiBlock->string.length && !IsZeroV2(uiBlock->textDim)))
						{
							uiBlock->rect.dim.elements[j] = uiBlock->textDim.elements[j];
						}
					}
				case UI_SIZE_KIND_PERCENT_OF_PARENT:
				case UI_SIZE_KIND_CHILDREN_OF_SUM:
				case UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT:
					break;

					InvalidDefaultCase
			}
		}
	}

	for (u32 i = 1; i < uiBuffer->uiBlockCount; i++)
	{
		UiBlock *uiBlock = &uiBuffer->uiBlocks[i];

		if (!uiBlock->parent)
		{
			CalculateUiUpwardsDependentSizes(uiBlock);
			CalculateUiDownwardsDependentSizes(uiBlock);
			CalculateUiRelativePositions(uiBlock);
			CalculateUiPosGivenReletativePositions(uiBlock);
		}
	}
}

void UiEndFrame()
{
	UiBuffer *uiBufferLastFrame = &G_UI_STATE.uiBuffers[1 - G_UI_STATE.uiBufferIndex];
	ArenaReset(&uiBufferLastFrame->arena);
	G_UI_STATE.uiBufferIndex = 1 - G_UI_STATE.uiBufferIndex;
}

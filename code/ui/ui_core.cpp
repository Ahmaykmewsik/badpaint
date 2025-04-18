#pragma once

#include "ui/ui_core.h"

//TODO: (Ahmayk) remove dependency
#include "main.h"
#include "vn_math_external.h"

UiState *UiInit(Arena *arena)
{
	UiState *result = ARENA_PUSH_STRUCT(arena, UiState);
	result->uiBuffers[0].arena = ArenaInitFromArena(arena, MegaByte * 5);
	result->uiBuffers[1].arena = ArenaInitFromArena(arena, MegaByte * 5);
	return result;
}

UiBlock *UiCreateBlock(UiState *uiState)
{
	UiBlock *result = {};
	UiBuffer *uiBuffer = &uiState->uiBuffers[uiState->uiBufferIndex];
	if (ASSERT(uiBuffer->uiBlockCount < ARRAY_COUNT(uiBuffer->uiBlocks)))
	{
		result = &uiBuffer->uiBlocks[uiBuffer->uiBlockCount++];
		*result = {};
		if (uiState->parentStackCount)
		{
			result->parent = uiState->parentStack[uiState->parentStackCount - 1];
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

void UiPushParent(UiState *uiState, UiBlock *uiBlock)
{
	if (ASSERT(uiState->parentStackCount < ARRAY_COUNT(uiState->parentStack)))
	{
		uiState->parentStack[uiState->parentStackCount] = uiBlock;
		uiState->parentStackCount++;
	}
}

void UiPopParent(UiState *uiState, UiBlock *uiBlock)
{
	if (ASSERT(uiState->parentStackCount > 0) &&
		ASSERT(uiState->parentStack[uiState->parentStackCount - 1] == uiBlock))
	{
		uiState->parentStackCount--;
	}
}

void CalculateUiUpwardsDependentSizes(UiBlock *uiBlock)
{
	if (uiBlock)
	{
		for (int i = 0; i < ARRAY_COUNT(uiBlock->uiSizes); i++)
		{
			switch (uiBlock->uiSizes[i].kind)
			{
			case UI_SIZE_KIND_PERCENT_OF_PARENT:
			{
				if (ASSERT(uiBlock->parent))
				{
					uiBlock->rect.dim.elements[i] = (uiBlock->parent->rect.dim.elements[i] * uiBlock->uiSizes[i].value);
				}
			} break;
			case UI_SIZE_KIND_PERCENT_OF_OTHER_AXIS:
			case UI_SIZE_KIND_SUM_OF_CHILDREN:
			case UI_SIZE_KIND_PIXELS:
			case UI_SIZE_KIND_TEXTURE:
			case UI_SIZE_KIND_TEXT:
				break;
				InvalidDefaultCase
			}
		}

		//NOTE: (Ahmayk) percent of other axis done in a 2nd pass so we solve other axis first (if solveable).
		for (int i = 0; i < ARRAY_COUNT(uiBlock->uiSizes); i++)
		{
			if (uiBlock->uiSizes[i].kind == UI_SIZE_KIND_PERCENT_OF_OTHER_AXIS)
			{
				uiBlock->rect.dim.elements[i] = uiBlock->rect.dim.elements[1 - i] * uiBlock->uiSizes[i].value;
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

		bool isHorizontal = uiBlock->flags & UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT;

		b32 childrenNeedUpwardRebuild = false;

		for (int i = 0; i < ARRAY_COUNT(uiBlock->uiSizes); i++)
		{
			switch (uiBlock->uiSizes[i].kind)
			{
				case UI_SIZE_KIND_SUM_OF_CHILDREN:
				{
					float sumOrMaxOfChildren = 0;
					UiBlock *child = uiBlock->firstChild;
					while (child)
					{
						ASSERT(child != uiBlock);

						((i == 0 && isHorizontal) || (i == 1 && !isHorizontal))
							? sumOrMaxOfChildren += child->rect.dim.elements[i]
							: sumOrMaxOfChildren = MaxF32(child->rect.dim.elements[i], sumOrMaxOfChildren);

						child = child->next;
					}
					uiBlock->rect.dim.elements[i] = sumOrMaxOfChildren;
					childrenNeedUpwardRebuild = true;
				} break;
				case UI_SIZE_KIND_PIXELS:
				case UI_SIZE_KIND_TEXTURE:
				case UI_SIZE_KIND_PERCENT_OF_PARENT:
				case UI_SIZE_KIND_PERCENT_OF_OTHER_AXIS:
				case UI_SIZE_KIND_TEXT:
				break;
				InvalidDefaultCase
			}
		}

		if (childrenNeedUpwardRebuild)
		{
			CalculateUiUpwardsDependentSizes(uiBlock->firstChild);
		}
	}
}

void CalculateUiRelativePositions(UiBlock *uiBlock)
{
	if (uiBlock)
	{
		if ((uiBlock->flags & UI_FLAG_MANUAL_POSITION) || (!uiBlock->parent || uiBlock->parent->flags & UI_FLAG_CHILDREN_MANUAL_POSITION))
		{
			uiBlock->computedRelativePixelPos = uiBlock->relativePixelPosition;
		}
		else if (uiBlock->parent && (uiBlock->flags & UI_FLAG_CENTER_IN_PARENT))
		{
			uiBlock->computedRelativePixelPos = PositionInCenterV2(uiBlock->parent->rect.dim, uiBlock->rect.dim);
		}
		else if (uiBlock->prev)
		{
			uiBlock->computedRelativePixelPos = uiBlock->prev->computedRelativePixelPos;

			if (uiBlock->parent && (uiBlock->parent->flags & UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT))
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

UiBlock *UiGetBlockOfHashLastFrame(UiState *uiState, u32 hash)
{
	UiBlock *result = {};

	UiBuffer *uiBuffer = &uiState->uiBuffers[1 - uiState->uiBufferIndex];
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
				} break;
				case UI_SIZE_KIND_PIXELS:
				{
					uiBlock->rect.dim.elements[j] = uiSize.value;
				} break;
				case UI_SIZE_KIND_TEXT:
				{
					if (ASSERT(uiBlock->string.length && !IsZeroV2(uiBlock->textDim)))
					{
						uiBlock->rect.dim.elements[j] = uiBlock->textDim.elements[j];
					}
				}
				case UI_SIZE_KIND_PERCENT_OF_PARENT:
				case UI_SIZE_KIND_SUM_OF_CHILDREN:
				case UI_SIZE_KIND_PERCENT_OF_OTHER_AXIS:
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

void UiEndFrame(UiState *uiState)
{
	UiBuffer *uiBufferLastFrame = &uiState->uiBuffers[1 - uiState->uiBufferIndex];
	ArenaReset(&uiBufferLastFrame->arena);
	uiState->uiBufferIndex = 1 - uiState->uiBufferIndex;
}

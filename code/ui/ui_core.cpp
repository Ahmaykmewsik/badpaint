#pragma once

#include "ui/ui_core.h"

//TODO: (Ahmayk) remove dependency
#include "main.h"
#include "vn_math_external.h"

#include <cstring>  //memset

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
			switch (uiBlock->uiSizes[i].type)
			{
			case UI_SIZE_PERCENT_OF_PARENT:
			{
				if (ASSERT(uiBlock->parent))
				{
					uiBlock->rect.dim.elements[i] = (uiBlock->parent->rect.dim.elements[i] * uiBlock->uiSizes[i].value);
				}
			} break;
			case UI_SIZE_PERCENT_OF_OTHER_AXIS:
			case UI_SIZE_SUM_OF_CHILDREN:
			case UI_SIZE_PIXELS:
			case UI_SIZE_TEXTURE:
			case UI_SIZE_TEXT:
			case UI_SIZE_FILL:
				break;
				InvalidDefaultCase
			}
		}

		//NOTE: (Ahmayk) percent of other axis done in a 2nd pass so we solve other axis first (if solveable).
		for (int i = 0; i < ARRAY_COUNT(uiBlock->uiSizes); i++)
		{
			if (uiBlock->uiSizes[i].type == UI_SIZE_PERCENT_OF_OTHER_AXIS)
			{
				uiBlock->rect.dim.elements[i] = uiBlock->rect.dim.elements[1 - i] * uiBlock->uiSizes[i].value;
			}
		}

		CalculateUiUpwardsDependentSizes(uiBlock->firstChild);
		CalculateUiUpwardsDependentSizes(uiBlock->next);
	}
}

f32 GetSumOrMaxOfAllAutoSiblingsAndSelf(UiBlock *uiBlock, UI_AXIS uiAxis)
{
	f32 result = 0;
	if (uiBlock)
	{
		UiBlock *siblingBlock = uiBlock;

		UI_CHILD_LAYOUT_TYPE parentLayoutType = UI_CHILD_LAYOUT_LEFT_TO_RIGHT;
		if (uiBlock->parent)
		{
			parentLayoutType = uiBlock->parent->uiChildLayoutType;
		}
		b32 inParentLayoutType = (uiAxis == UI_AXIS_X && parentLayoutType == UI_CHILD_LAYOUT_LEFT_TO_RIGHT) ||
								 (uiAxis == UI_AXIS_Y && parentLayoutType == UI_CHILD_LAYOUT_TOP_TO_BOTTOM);

		while(siblingBlock->prev)
		{
			siblingBlock = siblingBlock->prev;
		}
		for (; siblingBlock; siblingBlock = siblingBlock->next)
		{
			if (siblingBlock->uiPosition[uiAxis].type == UI_POSITION_AUTO)
			{
				if (inParentLayoutType)
				{
					result += siblingBlock->rect.dim.elements[uiAxis];
				}
				else
				{
					result = MaxF32(siblingBlock->rect.dim.elements[uiAxis], result);
				}
			}
		}
	}
	return result;
}

void CalculateUiDownwardsDependentSizes(UiBlock *uiBlock, v2 windowDim)
{
	if (uiBlock)
	{
		CalculateUiDownwardsDependentSizes(uiBlock->firstChild, windowDim);
		CalculateUiDownwardsDependentSizes(uiBlock->next, windowDim);

		b32 childrenNeedUpwardRebuild = false;

		for (int i = 0; i < ARRAY_COUNT(uiBlock->uiSizes); i++)
		{
			switch (uiBlock->uiSizes[i].type)
			{
				case UI_SIZE_SUM_OF_CHILDREN:
				{
					float sumOrMaxOfChildren = GetSumOrMaxOfAllAutoSiblingsAndSelf(uiBlock->firstChild, (UI_AXIS) i);
					uiBlock->rect.dim.elements[i] = sumOrMaxOfChildren;
					childrenNeedUpwardRebuild = true;
				} break;
				case UI_SIZE_FILL:
				{
					u32 fillBlockCount = 0;
					f32 parentSize = (f32) windowDim.elements[i];
					UI_CHILD_LAYOUT_TYPE parentLayoutType = UI_CHILD_LAYOUT_LEFT_TO_RIGHT;
					if (uiBlock->parent)
					{
						parentLayoutType = uiBlock->parent->uiChildLayoutType;
						parentSize = uiBlock->parent->rect.dim.elements[i];
					}

					UiBlock *siblingBlock = uiBlock;

					b32 inParentLayoutType = (i == UI_AXIS_X && parentLayoutType == UI_CHILD_LAYOUT_LEFT_TO_RIGHT) ||
						(i == UI_AXIS_Y && parentLayoutType == UI_CHILD_LAYOUT_TOP_TO_BOTTOM);

					f32 sumOrMax = 0;
					while(siblingBlock->prev)
					{
						siblingBlock = siblingBlock->prev;
					}
					for (; siblingBlock; siblingBlock = siblingBlock->next)
					{
						if (siblingBlock->uiSizes[i].type == UI_SIZE_FILL)
						{
							fillBlockCount++;
						}
						else if (siblingBlock->uiPosition[i].type == UI_POSITION_AUTO)
						{
							if (inParentLayoutType)
							{
								sumOrMax += siblingBlock->rect.dim.elements[i];
							}
							else
							{
								sumOrMax = MaxF32(siblingBlock->rect.dim.elements[i], sumOrMax);
							}
						}
					}

					if (ASSERT(fillBlockCount > 0))
					{
						if (inParentLayoutType)
						{
							f32 sizeToFill = MaxF32(0, parentSize - sumOrMax);
							uiBlock->rect.dim.elements[i] = sizeToFill / fillBlockCount;
						}
						else
						{
							uiBlock->rect.dim.elements[i] = sumOrMax;
						}
					}
					childrenNeedUpwardRebuild = true;
				} break;
				case UI_SIZE_PIXELS:
				case UI_SIZE_TEXTURE:
				case UI_SIZE_PERCENT_OF_PARENT:
				case UI_SIZE_PERCENT_OF_OTHER_AXIS:
				case UI_SIZE_TEXT:
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

void CalculateUiPositionData(UiBuffer *uiBuffer, UiBlock *uiBlock, v2 *calculatedPositionDatas, iv2 windowDim)
{
	if (uiBlock)
	{
#if 0
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
#endif

		v2 *calculatedPosition = calculatedPositionDatas + (uiBlock - uiBuffer->uiBlocks);

		for (u32 i = 0; i < UI_AXIS_COUNT; i++)
		{
			switch(uiBlock->uiPosition[i].type)
			{
				case UI_POSITION_ABSOLUTE:
				{
					//NOTE: (Ahmayk) absolute position is passed in directly into final rect later
				} break;
				case UI_POSITION_RELATIVE:
				{
					calculatedPosition->elements[i] = uiBlock->uiPosition[i].value;
				} break;
				case UI_POSITION_PERCENT_OF_PARENT:
				{
					if (ASSERT(uiBlock->parent))
					{
						calculatedPosition->elements[i] = uiBlock->parent->rect.dim.elements[i] * uiBlock->uiPosition[i].value;
					}
				} break;
				case UI_POSITION_AUTO:
				{
					f32 parentSize = (f32) windowDim.elements[i];
					UI_CHILD_LAYOUT_TYPE parentLayoutType = UI_CHILD_LAYOUT_LEFT_TO_RIGHT;
					UI_CHILD_ALIGN_TYPE parentAlignTypeOfAxis = UI_CHILD_ALIGN_START;
					if (uiBlock->parent)
					{
						parentLayoutType = uiBlock->parent->uiChildLayoutType;
						parentSize = uiBlock->parent->rect.dim.elements[i];
						parentAlignTypeOfAxis = uiBlock->parent->uiChildAlignTypes[i];
					}

					UiBlock *prevAutoBlock = {};
					for (UiBlock *prev = uiBlock->prev; prev; prev = prev->prev)
					{
						if (prev->uiPosition[i].type == UI_POSITION_AUTO)
						{
							prevAutoBlock = prev;
							break;
						}
					}

					b32 inParentLayoutType = (i == UI_AXIS_X && parentLayoutType == UI_CHILD_LAYOUT_LEFT_TO_RIGHT) ||
										     (i == UI_AXIS_Y && parentLayoutType == UI_CHILD_LAYOUT_TOP_TO_BOTTOM);

					if (prevAutoBlock && inParentLayoutType)
					{
						v2 *calculatedPositionPrev = calculatedPositionDatas + (prevAutoBlock - uiBuffer->uiBlocks);
						calculatedPosition->elements[i] = calculatedPositionPrev->elements[i] + prevAutoBlock->rect.dim.elements[i];
					}
					else
					{
						switch(parentAlignTypeOfAxis)
						{
							case UI_CHILD_ALIGN_START:
							{
								calculatedPosition->elements[i] = 0;
							} break;
							case UI_CHILD_ALIGN_END:
							{
								f32 sumOrMaxOfSiblingsAndSelf = GetSumOrMaxOfAllAutoSiblingsAndSelf(uiBlock, (UI_AXIS) i);
								calculatedPosition->elements[i] = parentSize - MinF32(parentSize, sumOrMaxOfSiblingsAndSelf);
							} break;
							case UI_CHILD_ALIGN_CENTER:
							{
								f32 sumOrMaxOfSiblingsAndSelf = GetSumOrMaxOfAllAutoSiblingsAndSelf(uiBlock, (UI_AXIS) i);
								calculatedPosition->elements[i] = (parentSize * 0.5f) - (MinF32(parentSize, sumOrMaxOfSiblingsAndSelf) * 0.5f);
							} break;
							InvalidDefaultCase;
						}
					}
				} break;
				InvalidDefaultCase;
			}
		}

		CalculateUiPositionData(uiBuffer, uiBlock->firstChild, calculatedPositionDatas, windowDim);
		CalculateUiPositionData(uiBuffer, uiBlock->next, calculatedPositionDatas, windowDim);
	}
}

void CalculateUiPos(UiBuffer *uiBuffer, UiBlock *uiBlock, v2 *calculatedPositionDatas)
{
	if (uiBlock)
	{
		v2 *calculatedPosition = calculatedPositionDatas + (uiBlock - uiBuffer->uiBlocks);

		v2 parentPos = {};
		if (uiBlock->parent)
		{
			parentPos = uiBlock->parent->rect.pos;
		}

		for (u32 i = 0; i < UI_AXIS_COUNT; i++)
		{
			switch(uiBlock->uiPosition[i].type)
			{
				case UI_POSITION_ABSOLUTE:
				{
					uiBlock->rect.pos.elements[i] = uiBlock->uiPosition[i].value;
				} break;
				case UI_POSITION_RELATIVE:
				case UI_POSITION_PERCENT_OF_PARENT:
				case UI_POSITION_AUTO:
				{
					uiBlock->rect.pos.elements[i] = parentPos.elements[i] + calculatedPosition->elements[i];
				} break;
				InvalidDefaultCase
			}
		}

		CalculateUiPos(uiBuffer, uiBlock->firstChild, calculatedPositionDatas);
		CalculateUiPos(uiBuffer, uiBlock->next, calculatedPositionDatas);
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

void UiLayoutBlocks(UiBuffer *uiBuffer, iv2 windowDim, Arena *temporaryArena)
{
	for (u32 i = 1; i < uiBuffer->uiBlockCount; i++)
	{
		UiBlock *uiBlock = &uiBuffer->uiBlocks[i];
		for (u32 j = 0; j < ARRAY_COUNT(uiBlock->uiSizes); j++)
		{
			UiSize uiSize = uiBlock->uiSizes[j];
			switch (uiSize.type)
			{
				case UI_SIZE_TEXTURE:
				{
					//NOTE: (Ahmayk) :(
					uiBlock->rect.dim.elements[j] = (f32) uiBlock->uiTexture.dim.elements[j];
				} break;
				case UI_SIZE_PIXELS:
				{
					uiBlock->rect.dim.elements[j] = uiSize.value;
				} break;
				case UI_SIZE_TEXT:
				{
					if (ASSERT(uiBlock->string.length && !IsZeroV2(uiBlock->textDim)))
					{
						uiBlock->rect.dim.elements[j] = uiBlock->textDim.elements[j];
					}
				}
				case UI_SIZE_PERCENT_OF_PARENT:
				case UI_SIZE_SUM_OF_CHILDREN:
				case UI_SIZE_PERCENT_OF_OTHER_AXIS:
				case UI_SIZE_FILL:
				break;
				InvalidDefaultCase
			}
		}
	}

	ArenaMarker positionDataMarker;
	v2 *calculatedPositionDatas = ARENA_PUSH_ARRAY_MARKER(temporaryArena, uiBuffer->uiBlockCount, v2, &positionDataMarker);
	memset(calculatedPositionDatas, 0, uiBuffer->uiBlockCount * sizeof(v2));
	for (u32 i = 1; i < uiBuffer->uiBlockCount; i++)
	{
		UiBlock *uiBlock = &uiBuffer->uiBlocks[i];
		if (!uiBlock->parent)
		{
			CalculateUiUpwardsDependentSizes(uiBlock);
			CalculateUiDownwardsDependentSizes(uiBlock, windowDim);
			CalculateUiPositionData(uiBuffer, uiBlock, calculatedPositionDatas, windowDim);
			CalculateUiPos(uiBuffer, uiBlock, calculatedPositionDatas);
		}
	}
	ArenaPopMarker(positionDataMarker);
}

void UiEndFrame(UiState *uiState)
{
	UiBuffer *uiBufferLastFrame = &uiState->uiBuffers[1 - uiState->uiBufferIndex];
	ArenaReset(&uiBufferLastFrame->arena);
	uiState->uiBufferIndex = 1 - uiState->uiBufferIndex;
}

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

UiBlock *UiCreateRootBlock(UiState *uiState)
{
	UiBlock *result = {};
	UiBuffer *uiBuffer = &uiState->uiBuffers[uiState->uiBufferIndex];
	if (ASSERT(uiBuffer->uiBlockCount < ARRAY_COUNT(uiBuffer->uiBlocks)))
	{
		result = &uiBuffer->uiBlocks[uiBuffer->uiBlockCount++];
		*result = {};
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

struct UiSolveData
{
	b32 isSizeSolved[UI_AXIS_COUNT];
	v2 calculatedPosition;
};

struct UiSolveState
{
	UiBlock *uiBlocksStart;
	UiSolveData *uiSolveData;
	u32 solvedBlockSizeCount;
	iv2 windowDim;
};

UiSolveData *GetUiSolveData(UiSolveState *uiSolveState, UiBlock *uiBlock)
{
	UiSolveData *result = uiSolveState->uiSolveData + (uiBlock - uiSolveState->uiBlocksStart);
	return result;
}

b32 IsBlockSizeAxisSolved(UiSolveState *uiSolveState, UiBlock *uiBlock, UI_AXIS uiAxis)
{
	UiSolveData *uiSolveData = GetUiSolveData(uiSolveState, uiBlock);
	b32 result = uiSolveData->isSizeSolved[uiAxis];
	return result;
}

void MarkBlockSizeAxisAsSolved(UiSolveState *uiSolveState, UiBlock *uiBlock, UI_AXIS uiAxis)
{
	UiSolveData *uiSolveData = GetUiSolveData(uiSolveState, uiBlock);
	uiSolveData->isSizeSolved[uiAxis] = true;
	if (uiSolveData->isSizeSolved[0] && uiSolveData->isSizeSolved[1])
	{
		uiSolveState->solvedBlockSizeCount++;
	}
}

void CalculateFixedSizes(UiSolveState *uiSolveState, UiBlock *uiBlock)
{
	if (uiBlock)
	{
		for (u32 i = 0; i < ARRAY_COUNT(uiBlock->uiSizes); i++)
		{
			UiSize uiSize = uiBlock->uiSizes[i];
			switch (uiSize.type)
			{
				case UI_SIZE_TEXTURE:
				{
					//NOTE: (Ahmayk) :(
					uiBlock->rect.dim.elements[i] = (f32) uiBlock->uiTextureView.viewRect.dim.elements[i];
					MarkBlockSizeAxisAsSolved(uiSolveState, uiBlock, (UI_AXIS) i);
				} break;
				case UI_SIZE_PIXELS:
				{
					uiBlock->rect.dim.elements[i] = uiSize.value;
					MarkBlockSizeAxisAsSolved(uiSolveState, uiBlock, (UI_AXIS) i);
				} break;
				case UI_SIZE_TEXT:
				{
					if (ASSERT(uiBlock->string.length && !IsZeroV2(uiBlock->textDim)))
					{
						uiBlock->rect.dim.elements[i] = uiBlock->textDim.elements[i];
					}
					MarkBlockSizeAxisAsSolved(uiSolveState, uiBlock, (UI_AXIS) i);
				}
				case UI_SIZE_PERCENT_OF_PARENT:
				case UI_SIZE_SUM_OF_CHILDREN:
				case UI_SIZE_PERCENT_OF_OTHER_AXIS:
				case UI_SIZE_FILL:
					break;
					InvalidDefaultCase
			}
		}

		CalculateFixedSizes(uiSolveState, uiBlock->firstChild);
		CalculateFixedSizes(uiSolveState, uiBlock->next);
	}
}

void CalculateUiUpwardsDependentSizes(UiSolveState *uiSolveState, UiBlock *uiBlock)
{
	if (uiBlock)
	{
		for (int i = 0; i < ARRAY_COUNT(uiBlock->uiSizes); i++)
		{
			switch (uiBlock->uiSizes[i].type)
			{
			case UI_SIZE_PERCENT_OF_PARENT:
			{
				if (!IsBlockSizeAxisSolved(uiSolveState, uiBlock, (UI_AXIS) i) &&
					ASSERT(uiBlock->parent) &&
					IsBlockSizeAxisSolved(uiSolveState, uiBlock->parent, (UI_AXIS) i))
				{
					uiBlock->rect.dim.elements[i] = (uiBlock->parent->rect.dim.elements[i] * uiBlock->uiSizes[i].value);
					MarkBlockSizeAxisAsSolved(uiSolveState, uiBlock, (UI_AXIS) i);
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
		for (u32 i = 0; i < ARRAY_COUNT(uiBlock->uiSizes); i++)
		{
			if (uiBlock->uiSizes[i].type == UI_SIZE_PERCENT_OF_OTHER_AXIS &&
				!IsBlockSizeAxisSolved(uiSolveState, uiBlock, (UI_AXIS) i) &&
				ASSERT(uiBlock->uiSizes[1 - i].type != UI_SIZE_PERCENT_OF_OTHER_AXIS) &&
				IsBlockSizeAxisSolved(uiSolveState, uiBlock, (UI_AXIS) (1 - i)))
			{
				uiBlock->rect.dim.elements[i] = uiBlock->rect.dim.elements[1 - i] * uiBlock->uiSizes[i].value;
				MarkBlockSizeAxisAsSolved(uiSolveState, uiBlock, (UI_AXIS) i);
			}
		}

		CalculateUiUpwardsDependentSizes(uiSolveState, uiBlock->firstChild);
		CalculateUiUpwardsDependentSizes(uiSolveState, uiBlock->next);
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

b32 IsSiblingsOfAxisSolved(UiSolveState *uiSolveState, UiBlock *uiBlock, UI_AXIS uiAxis)
{
	b32 result = true;
	UiBlock *siblingBlock = uiBlock;
	while(siblingBlock->prev)
	{
		siblingBlock = siblingBlock->prev;
	}
	for (; siblingBlock; siblingBlock = siblingBlock->next)
	{
		if (!IsBlockSizeAxisSolved(uiSolveState, siblingBlock, uiAxis))
		{
			result = false;
			break;
		}
	}
	return result;
}

void CalculateUiDownwardsDependentSizes(UiSolveState *uiSolveState, UiBlock *uiBlock)
{
	if (uiBlock)
	{
		CalculateUiDownwardsDependentSizes(uiSolveState, uiBlock->firstChild);
		CalculateUiDownwardsDependentSizes(uiSolveState, uiBlock->next);

		b32 childrenNeedUpwardRebuild = false;

		for (int i = 0; i < ARRAY_COUNT(uiBlock->uiSizes); i++)
		{
			switch (uiBlock->uiSizes[i].type)
			{
				case UI_SIZE_SUM_OF_CHILDREN:
				{
					if (!IsBlockSizeAxisSolved(uiSolveState, uiBlock, (UI_AXIS) i) &&
						IsSiblingsOfAxisSolved(uiSolveState, uiBlock->firstChild, (UI_AXIS) i))
					{
						float sumOrMaxOfChildren = GetSumOrMaxOfAllAutoSiblingsAndSelf(uiBlock->firstChild, (UI_AXIS) i);
						uiBlock->rect.dim.elements[i] = sumOrMaxOfChildren;
						MarkBlockSizeAxisAsSolved(uiSolveState, uiBlock, (UI_AXIS) i);
					}
				} break;
				case UI_SIZE_FILL:
				{
					if (!IsBlockSizeAxisSolved(uiSolveState, uiBlock, (UI_AXIS) i))
					{
						b32 readyToSolve = true;
						f32 parentSize = (f32) uiSolveState->windowDim.elements[i];
						UI_CHILD_LAYOUT_TYPE parentLayoutType = UI_CHILD_LAYOUT_LEFT_TO_RIGHT;
						if (uiBlock->parent)
						{
							if (IsBlockSizeAxisSolved(uiSolveState, uiBlock->parent, (UI_AXIS) i))
							{
								parentLayoutType = uiBlock->parent->uiChildLayoutType;
								parentSize = uiBlock->parent->rect.dim.elements[i];
							}
							else
							{
								readyToSolve = false;
							}
						}

						f32 sumOrMax = 0;
						u32 fillBlockCount = 0;
						b32 inParentLayoutType = (i == UI_AXIS_X && parentLayoutType == UI_CHILD_LAYOUT_LEFT_TO_RIGHT) ||
							(i == UI_AXIS_Y && parentLayoutType == UI_CHILD_LAYOUT_TOP_TO_BOTTOM);

						if (readyToSolve)
						{
							UiBlock *siblingBlock = uiBlock;

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
									if (IsBlockSizeAxisSolved(uiSolveState, siblingBlock, (UI_AXIS) i))
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
									else
									{
										readyToSolve = false;
										break;
									}
								}
							}
						}

						if (readyToSolve)
						{
							if (ASSERT(fillBlockCount > 0))
							{
								if (inParentLayoutType)
								{
									f32 sizeToFill = MaxF32(0, parentSize - sumOrMax);
									uiBlock->rect.dim.elements[i] = sizeToFill / fillBlockCount;
								}
								else if (sumOrMax > 0)
								{
									uiBlock->rect.dim.elements[i] = sumOrMax;
								}
								else
								{
									uiBlock->rect.dim.elements[i] = parentSize;
								}
							}
							MarkBlockSizeAxisAsSolved(uiSolveState, uiBlock, (UI_AXIS) i);
						}
					}
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
	}
}

void CalculateUiPositionData(UiSolveState *uiSolveState, UiBlock *uiBlock)
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

		UiSolveData *uiSolveData = GetUiSolveData(uiSolveState, uiBlock);
		v2 *calculatedPosition = &uiSolveData->calculatedPosition;

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
					f32 parentSize = (f32) uiSolveState->windowDim.elements[i];
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
						UiSolveData *uiSolveDataPrev = GetUiSolveData(uiSolveState, uiBlock->prev);
						calculatedPosition->elements[i] = uiSolveDataPrev->calculatedPosition.elements[i] + prevAutoBlock->rect.dim.elements[i];
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

		CalculateUiPositionData(uiSolveState, uiBlock->firstChild);
		CalculateUiPositionData(uiSolveState, uiBlock->next);
	}
}

void CalculateUiPos(UiSolveState *uiSolveState, UiBlock *uiBlock)
{
	if (uiBlock)
	{
		UiSolveData *uiSolveData = GetUiSolveData(uiSolveState, uiBlock);
		v2 *calculatedPosition = &uiSolveData->calculatedPosition;

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

		CalculateUiPos(uiSolveState, uiBlock->firstChild);
		CalculateUiPos(uiSolveState, uiBlock->next);
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

u32 GetNumBlocksInTree(UiBlock *uiBlock, u32 count = 0)
{
	u32 result = 0;
	if (uiBlock)
	{
		result += 1;
		result += GetNumBlocksInTree(uiBlock->firstChild, result);
		result += GetNumBlocksInTree(uiBlock->next, result);
	}
	return result;
}

void UiLayoutBlocks(UiBuffer *uiBuffer, iv2 windowDim, Arena *temporaryArena)
{
	UiSolveState uiSolveState = {};
	uiSolveState.windowDim = windowDim;
	uiSolveState.uiBlocksStart = uiBuffer->uiBlocks;
	ArenaMarker positionDataMarker;
	uiSolveState.uiSolveData = ARENA_PUSH_ARRAY_MARKER(temporaryArena, uiBuffer->uiBlockCount, UiSolveData, &positionDataMarker);
	memset(uiSolveState.uiSolveData, 0, uiBuffer->uiBlockCount * sizeof(UiSolveData));

	for (u32 i = 1; i < uiBuffer->uiBlockCount; i++)
	{
		UiBlock *uiBlock = &uiBuffer->uiBlocks[i];
		if (!uiBlock->parent)
		{
			u32 blocksInTreeCount = GetNumBlocksInTree(uiBlock);
			CalculateFixedSizes(&uiSolveState, uiBlock);
			if (uiSolveState.solvedBlockSizeCount != blocksInTreeCount)
			{
				u32 solveIterationIndex = 0;
				u32 solveIterationLimit = 100;
				for (; solveIterationIndex < solveIterationLimit; solveIterationIndex++)
				{
					CalculateUiUpwardsDependentSizes(&uiSolveState, uiBlock);
					if (uiSolveState.solvedBlockSizeCount == blocksInTreeCount)
					{
						break;
					}
					CalculateUiDownwardsDependentSizes(&uiSolveState, uiBlock);
					if (uiSolveState.solvedBlockSizeCount == blocksInTreeCount)
					{
						break;
					}
				}
				ASSERT(solveIterationIndex < solveIterationLimit);
			}
			uiSolveState.solvedBlockSizeCount = 0;
			CalculateUiPositionData(&uiSolveState, uiBlock);
			CalculateUiPos(&uiSolveState, uiBlock);
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

#pragma once

#include "ui/ui.h"

//TODO: (Ahmayk) remove dependency
#include "main.h"
#include "vn_math_external.h"

static UiState G_UI_STATE = {};
static UiInputs G_UI_INPUTS = {};

//TODO: (Ahmayk) Temporary hack 
UiState *GetUiState()
{
	return &G_UI_STATE;
}

UiInputs *GetUiInputs()
{
	return &G_UI_INPUTS;
}

void UiInit(Arena *arena)
{
	G_UI_STATE.uiBuffers[0].arena = ArenaInitFromArena(arena, MegaByte * 5);
	G_UI_STATE.uiBuffers[1].arena = ArenaInitFromArena(arena, MegaByte * 5);
}

void CreateUiBlock(unsigned int flags, u32 hash, String string)
{
	UiBuffer *uiBuffer = &G_UI_STATE.uiBuffers[G_UI_STATE.uiBufferIndex];
	if (ASSERT(uiBuffer->uiBlockCount < ARRAY_COUNT(uiBuffer->uiBlocks)))
	{
		UiBlock *uiBlock = &uiBuffer->uiBlocks[uiBuffer->uiBlockCount++];
		*uiBlock = {};

		if (G_UI_STATE.parentStackCount)
		{
			uiBlock->parent = G_UI_STATE.parentStack[G_UI_STATE.parentStackCount - 1];
			ASSERT(uiBlock->parent);

			if (uiBlock->parent->firstChild)
			{
				ASSERT(uiBlock->parent->lastChild);
				uiBlock->prev = uiBlock->parent->lastChild;
				uiBlock->parent->lastChild->next = uiBlock;
			}
			else
			{
				uiBlock->parent->firstChild = uiBlock;
			}

			uiBlock->parent->lastChild = uiBlock;
		}

		uiBlock->flags = flags;
		uiBlock->uiSettings = G_UI_STATE.uiSettings;
		uiBlock->hash = hash;

		if (string.length)
		{
			uiBlock->string = ReallocString(string, &uiBuffer->arena);

			//TODO: (Ahmayk) how to not depend on raylib here? 
			ASSERT(uiBlock->uiSettings.font.baseSize);
			Vector2 textDim = MeasureTextEx(uiBlock->uiSettings.font, C_STRING_NULL_TERMINATED(uiBlock->string), (f32) uiBlock->uiSettings.font.baseSize, 1);
			uiBlock->textDim = RayVectorToV2(textDim);
		}

		uiBlock->uiInputs = G_UI_INPUTS;
	}
	G_UI_INPUTS = {};
}

void CreateUiText(String string)
{
	if (string.length)
	{
		CreateUiBlock(UI_FLAG_DRAW_TEXT, 0, string);
	}
}

void CreateUiTextWithBackground(String string, unsigned int flags = 0)
{
	if (string.length)
	{
		CreateUiBlock(UI_FLAG_DRAW_TEXT | UI_FLAG_DRAW_BACKGROUND | flags, 0, string);
	}
}

void SetUiAxis(UiSize uiSize1, UiSize uiSize2)
{
	G_UI_STATE.uiSettings.uiSizes[0] = uiSize1;
	G_UI_STATE.uiSettings.uiSizes[1] = uiSize2;
}

void SetUiAxisToPixelDim(v2 pixelDim)
{
	SetUiAxis({UI_SIZE_KIND_PIXELS, pixelDim.x}, {UI_SIZE_KIND_PIXELS, pixelDim.y});
}

void SetUiTimlineRowAxisPercentOfX(float percentOfParentX)
{
	G_UI_STATE.uiSettings.uiSizes[0] = UiSize{UI_SIZE_KIND_PERCENT_OF_PARENT, percentOfParentX};
	G_UI_STATE.uiSettings.uiSizes[1] = UiSize{UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
}

void PushPixelSize(v2 pixelSize)
{
	G_UI_STATE.uiSettings.uiSizes[0] = {UI_SIZE_KIND_PIXELS, pixelSize.x};
	G_UI_STATE.uiSettings.uiSizes[1] = {UI_SIZE_KIND_PIXELS, pixelSize.x};
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
		if (uiBlock->uiSettings.uiSizes[0].kind == UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT ||
				uiBlock->uiSettings.uiSizes[1].kind == UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT)
		{
			ASSERT(uiBlock->parent);
			ASSERT(uiBlock->uiInputs.texture.width && uiBlock->uiInputs.texture.height);
			v2 textureDim = GetTextureDim(uiBlock->uiInputs.texture);

			float scaleX = 1;
			float scaleY = 1;
			if (uiBlock->parent->rect.dim.x)
				scaleX = uiBlock->parent->rect.dim.x / textureDim.x;
			if (uiBlock->parent->rect.dim.y)
				scaleY = uiBlock->parent->rect.dim.y / textureDim.y;

			float scale = MinF32(scaleX, scaleY);

			uiBlock->rect.dim = textureDim * scale;
		}
		else
		{
			for (int j = 0;
					j < ARRAY_COUNT(uiBlock->uiSettings.uiSizes);
					j++)
			{
				UiSize uiSize = uiBlock->uiSettings.uiSizes[j];
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
				j < ARRAY_COUNT(uiBlock->uiSettings.uiSizes);
				j++)
		{
			UiSize uiSize = uiBlock->uiSettings.uiSizes[j];
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

						if (j == ARRAY_COUNT(uiBlock->uiSettings.uiSizes) - 1)
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
			uiBlock->computedRelativePixelPos = uiBlock->uiInputs.relativePixelPosition;
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

Color GetReactiveColor(CommandInput *commandInputs, UiBlock *uiBlockLastFrame, ReactiveUiColor reactiveUiColor, bool disabled)
{
	Color result = reactiveUiColor.neutral;

	if (disabled)
		result = reactiveUiColor.disabled;
	else if (uiBlockLastFrame)
	{
		bool down = uiBlockLastFrame->down;
		bool hovered = uiBlockLastFrame->hovered;

		COMMAND command = uiBlockLastFrame->uiInputs.command;
		if (down || (command && IsCommandDown(commandInputs, command)))
			result = reactiveUiColor.down;
		else if (hovered)
			result = reactiveUiColor.hovered;
	}

	return result;
}

Color GetReactiveColorWithState(CommandInput *commandInputs, UiBlock *uiBlockLastFrame, ReactiveUiColorState reactiveUiColorState, bool disabled, bool active)
{
	ReactiveUiColor reactiveUiColor = (active)
		? reactiveUiColorState.active
		: reactiveUiColorState.nonActive;
	Color result = GetReactiveColor(commandInputs, uiBlockLastFrame, reactiveUiColor, disabled);
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

void CreateUiButton(String string, u32 hash, ReactiveUiColorState reactiveUiColorState, bool active, bool disabled)
{
	ReactiveUiColor reactiveUiColor = (active)
		? reactiveUiColorState.active
		: reactiveUiColorState.nonActive;
	UiBlock *uiBlockLastFrame = GetUiBlockOfHashLastFrame(hash);
	G_UI_STATE.uiSettings.backColor = GetReactiveColor(G_UI_STATE.commandInputs, uiBlockLastFrame, reactiveUiColor, disabled);

	UiSettings *uiSettings = &G_UI_STATE.uiSettings;
	uiSettings->frontColor = BLACK;
	uiSettings->detailColor = BLACK;
	uiSettings->borderColor = (active) ? BLACK : GRAY;

	unsigned int flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER | UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_CENTERED | UI_FLAG_INTERACTABLE;
	CreateUiBlock(flags, hash, string);
}

Color AddConstantToColor(Color color, i8 constant)
{
	Color result = color;
	result.r = (u8) ClampI32(0, result.r + constant, 255);
	result.g = (u8) ClampI32(0, result.g + constant, 255);
	result.b = (u8) ClampI32(0, result.b + constant, 255);
	return result;
}

ReactiveUiColorState CreateButtonReactiveUiColorState(Color color)
{
	ReactiveUiColorState result = {};

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

		for (u32 j = 0; j < ARRAY_COUNT(uiBlock->uiSettings.uiSizes); j++)
		{
			UiSize uiSize = uiBlock->uiSettings.uiSizes[j];
			switch (uiSize.kind)
			{
				case UI_SIZE_KIND_TEXTURE:
					{
						v2 dim = GetTextureDim(uiBlock->uiInputs.texture);
						uiBlock->rect.dim.elements[j] = dim.elements[j];
						break;
					}
				case UI_SIZE_KIND_PIXELS:
					{
						uiBlock->rect.dim.elements[j] = uiSize.value;
						break;
					}
				case UI_SIZE_KIND_TEXT:
					{
						uiBlock->rect.dim.elements[j] = uiBlock->textDim.elements[j];
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

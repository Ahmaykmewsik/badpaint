#pragma once

void CreateUiBox(unsigned int flags = 0, String string = {})
{
	ASSERT(G_UI_INPUTS);
	ASSERT(G_UI_STATE);

	int uiBoxArrayIndexThisFrame = GetFrameModIndexThisFrame();
	ASSERT(G_UI_STATE->uiBoxCount < ARRAY_COUNT(G_UI_STATE->uiBoxes[uiBoxArrayIndexThisFrame]));

	UiBox *uiBox = &G_UI_STATE->uiBoxes[uiBoxArrayIndexThisFrame][G_UI_STATE->uiBoxCount];
	*uiBox = {};
	uiBox->index = G_UI_STATE->uiBoxCount;
	G_UI_STATE->uiBoxCount++;

	if (G_UI_STATE->parentStackCount)
	{
		uiBox->parent = G_UI_STATE->parentStack[G_UI_STATE->parentStackCount - 1];
		ASSERT(uiBox->parent);

		if (uiBox->parent->firstChild)
		{
			ASSERT(uiBox->parent->lastChild);
			uiBox->prev = uiBox->parent->lastChild;
			uiBox->parent->lastChild->next = uiBox;
		}
		else
		{
			uiBox->parent->firstChild = uiBox;
		}

		uiBox->parent->lastChild = uiBox;
	}

	uiBox->flags = flags;
	uiBox->uiSettings = G_UI_STATE->uiSettings;
	uiBox->frameRendered = G_CURRENT_FRAME;

	StringArray stringSplitByHashTag = {};
	if (string.length)
	{
		uiBox->string = string;

		stringSplitByHashTag = SplitStringOnceByTag(uiBox->string, STRING(G_UI_HASH_TAG), G_UI_STATE->twoFrameArenaThisFrame);

		if (stringSplitByHashTag.count == 2)
		{
			uiBox->string = stringSplitByHashTag.strings[0];
			uiBox->keyString = stringSplitByHashTag.strings[1];
		}
		else if (stringSplitByHashTag.count == 1)
		{
			uiBox->string = ReallocString(uiBox->string, G_UI_STATE->twoFrameArenaThisFrame);
		}
		else
		{
			InvalidCodePath;
		}

		ASSERT(uiBox->uiSettings.font.baseSize);
		Vector2 textDim = MeasureTextEx(uiBox->uiSettings.font, C_STRING_NULL_TERMINATED(uiBox->string), (f32) uiBox->uiSettings.font.baseSize, 1);
		uiBox->textDim = RayVectorToV2(textDim);
	}

	uiBox->uiInputs = *G_UI_INPUTS;
	*G_UI_INPUTS = {};

	if (stringSplitByHashTag.count > 1)
	{
		String keyString = stringSplitByHashTag.strings[1];
		if (keyString.length)
		{
			unsigned int hashValue = Murmur3String(C_STRING_NULL_TERMINATED(keyString), 0);

			for (u32 i = 0; i < keyString.length; i++)
			{
				unsigned int index = (hashValue + i) % ARRAY_COUNT(G_UI_STATE->uiHashEntries);
				UiHashEntry *uiHashEntry = &G_UI_STATE->uiHashEntries[index];
				if (!uiHashEntry->uiBox || uiHashEntry->uiBox->frameRendered < G_CURRENT_FRAME)
				{
					uiHashEntry->uiBox = uiBox;
					uiHashEntry->keyString = keyString;
					break;
				}
			}
		}
	}
}

void CreateUiText(String string)
{
	if (string.length)
	{
		CreateUiBox(UI_FLAG_DRAW_TEXT, string);
	}
}

void CreateUiTextWithBackground(String string, unsigned int flags = 0)
{
	if (string.length)
	{
		CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_DRAW_BACKGROUND | flags, string);
	}
}

void SetUiAxis(UiSize uiSize1, UiSize uiSize2)
{
	G_UI_STATE->uiSettings.uiSizes[0] = uiSize1;
	G_UI_STATE->uiSettings.uiSizes[1] = uiSize2;
}

void SetUiAxisToPixelDim(v2 pixelDim)
{
	SetUiAxis({UI_SIZE_KIND_PIXELS, pixelDim.x}, {UI_SIZE_KIND_PIXELS, pixelDim.y});
}

void SetUiTimlineRowAxisPercentOfX(float percentOfParentX)
{
	G_UI_STATE->uiSettings.uiSizes[0] = UiSize{UI_SIZE_KIND_PERCENT_OF_PARENT, percentOfParentX};
	G_UI_STATE->uiSettings.uiSizes[1] = UiSize{UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
}

void PushPixelSize(v2 pixelSize)
{
	G_UI_STATE->uiSettings.uiSizes[0] = {UI_SIZE_KIND_PIXELS, pixelSize.x};
	G_UI_STATE->uiSettings.uiSizes[1] = {UI_SIZE_KIND_PIXELS, pixelSize.x};
}

bool IsFlag(UiBox *uiBox, unsigned int flags = 0)
{
	bool result = uiBox->flags & flags;
	return result;
}

unsigned int GetThisUiBoxIndex()
{
	unsigned int result = 0;

	int uiBoxArrayIndexThisFrame = GetFrameModIndexThisFrame();
	if (G_UI_STATE->uiBoxCount)
		result = G_UI_STATE->uiBoxes[uiBoxArrayIndexThisFrame][G_UI_STATE->uiBoxCount - 1].index;

	return result;
}

void PushUiParent()
{
	ASSERT(G_UI_STATE->parentStackCount < ARRAY_COUNT(G_UI_STATE->parentStack));

	int uiBoxArrayIndexThisFrame = GetFrameModIndexThisFrame();
	G_UI_STATE->parentStack[G_UI_STATE->parentStackCount] = &G_UI_STATE->uiBoxes[uiBoxArrayIndexThisFrame][G_UI_STATE->uiBoxCount - 1];
	G_UI_STATE->parentStackCount++;
}

void PopUiParent()
{
	ASSERT(G_UI_STATE->parentStackCount > 0);
	G_UI_STATE->parentStackCount--;
}

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define UiDeferLoop(begin, end) for (int CONCAT(_i_, __LINE__) = ((begin), 0); !CONCAT(_i_, __LINE__); CONCAT(_i_, __LINE__) += 1, (end))

#define UiParent() UiDeferLoop(PushUiParent(), PopUiParent())

void CalculateUiUpwardsDependentSizes(UiBox *uiBox)
{
	if (uiBox)
	{
		if (uiBox->uiSettings.uiSizes[0].kind == UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT ||
				uiBox->uiSettings.uiSizes[1].kind == UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT)
		{
			ASSERT(uiBox->parent);
			ASSERT(uiBox->uiInputs.texture.width && uiBox->uiInputs.texture.height);
			v2 textureDim = GetTextureDim(uiBox->uiInputs.texture);

			float scaleX = 1;
			float scaleY = 1;
			if (uiBox->parent->rect.dim.x)
				scaleX = uiBox->parent->rect.dim.x / textureDim.x;
			if (uiBox->parent->rect.dim.y)
				scaleY = uiBox->parent->rect.dim.y / textureDim.y;

			float scale = MinF32(scaleX, scaleY);

			uiBox->rect.dim = textureDim * scale;
		}
		else
		{
			for (int j = 0;
					j < ARRAY_COUNT(uiBox->uiSettings.uiSizes);
					j++)
			{
				UiSize uiSize = uiBox->uiSettings.uiSizes[j];
				switch (uiSize.kind)
				{
					case UI_SIZE_KIND_PERCENT_OF_PARENT:
						{
							if (uiBox->parent && uiBox->parent->rect.dim.elements[j])
							{
								uiBox->rect.dim.elements[j] = (uiBox->parent->rect.dim.elements[j] * uiSize.value);
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

		CalculateUiUpwardsDependentSizes(uiBox->firstChild);
		CalculateUiUpwardsDependentSizes(uiBox->next);
	}
}

void CalculateUiDownwardsDependentSizes(UiBox *uiBox)
{
	if (uiBox)
	{
		CalculateUiDownwardsDependentSizes(uiBox->firstChild);
		CalculateUiDownwardsDependentSizes(uiBox->next);

		bool isHorizontal = IsFlag(uiBox, UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);

		for (int j = 0;
				j < ARRAY_COUNT(uiBox->uiSettings.uiSizes);
				j++)
		{
			UiSize uiSize = uiBox->uiSettings.uiSizes[j];
			switch (uiSize.kind)
			{
				case UI_SIZE_KIND_CHILDREN_OF_SUM:
					{
						float sumOrMaxOfChildren = 0;
						UiBox *child = uiBox->firstChild;
						while (child)
						{
							ASSERT(child != uiBox);

							((j == 0 && isHorizontal) || (j == 1 && !isHorizontal))
								? sumOrMaxOfChildren += child->rect.dim.elements[j]
								: sumOrMaxOfChildren = MaxF32(child->rect.dim.elements[j], sumOrMaxOfChildren);

							child = child->next;
						}
						uiBox->rect.dim.elements[j] = sumOrMaxOfChildren;

						if (j == ARRAY_COUNT(uiBox->uiSettings.uiSizes) - 1)
							CalculateUiUpwardsDependentSizes(uiBox->firstChild);
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

void CalculateUiRelativePositions(UiBox *uiBox)
{
	if (uiBox)
	{
		if (IsFlag(uiBox, UI_FLAG_MANUAL_POSITION) || (!uiBox->parent || IsFlag(uiBox->parent, UI_FLAG_CHILDREN_MANUAL_POSITION)))
		{
			uiBox->computedRelativePixelPos = uiBox->uiInputs.relativePixelPosition;
		}
		else if (uiBox->parent && IsFlag(uiBox, UI_FLAG_CENTER_IN_PARENT))
		{
			uiBox->computedRelativePixelPos = PositionInCenterV2(uiBox->parent->rect.dim, uiBox->rect.dim);
		}
		else if (uiBox->prev)
		{
			uiBox->computedRelativePixelPos = uiBox->prev->computedRelativePixelPos;

			if (uiBox->parent && IsFlag(uiBox->parent, UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT))
				uiBox->computedRelativePixelPos.x += uiBox->prev->rect.dim.x;
			else
				uiBox->computedRelativePixelPos.y += uiBox->prev->rect.dim.y;
		}

		CalculateUiRelativePositions(uiBox->firstChild);
		CalculateUiRelativePositions(uiBox->next);
	}
}

void CalculateUiPosGivenReletativePositions(UiBox *uiBox)
{
	if (uiBox)
	{
		if (uiBox->parent)
			uiBox->computedRelativePixelPos += uiBox->parent->computedRelativePixelPos;

		uiBox->rect.pos = uiBox->computedRelativePixelPos;

		CalculateUiPosGivenReletativePositions(uiBox->firstChild);
		CalculateUiPosGivenReletativePositions(uiBox->next);
	}
}

UiBox GetValidUiBoxOfIndexLastFrame(unsigned int index)
{
	UiBox result = {};

	if (index)
	{
		int uiBoxArrayIndexLastFrame = GetFrameModIndexLastFrame();
		if (index < ARRAY_COUNT(G_UI_STATE->uiBoxes[uiBoxArrayIndexLastFrame]))
		{
			UiBox uiBox = G_UI_STATE->uiBoxes[uiBoxArrayIndexLastFrame][index];
			if (uiBox.frameRendered == G_CURRENT_FRAME - 1)
				result = uiBox;
		}
	}

	return result;
}

Color GetReactiveColor(UiBox *uiBoxLastFrame, ReactiveUiColor reactiveUiColor, bool disabled)
{
	Color result = reactiveUiColor.neutral;

	if (disabled)
		result = reactiveUiColor.disabled;
	else if (uiBoxLastFrame)
	{
		bool down = uiBoxLastFrame->down;
		bool hovered = uiBoxLastFrame->hovered;

		COMMAND command = uiBoxLastFrame->uiInputs.command;
		if (command)
			down = COMMAND_STATES[command].down;

		if (down)
			result = reactiveUiColor.down;
		else if (hovered)
			result = reactiveUiColor.hovered;
	}

	return result;
}

Color GetReactiveColorWithState(UiBox *uiBoxLastFrame, ReactiveUiColorState reactiveUiColorState, bool disabled, bool active)
{
	ReactiveUiColor reactiveUiColor = (active)
		? reactiveUiColorState.active
		: reactiveUiColorState.nonActive;
	Color result = GetReactiveColor(uiBoxLastFrame, reactiveUiColor, disabled);
	return result;
}

String GetUiBoxKeyStringOfString(String string)
{
	String result = {};

	StringArray stringArray = SplitStringOnceByTag(string, STRING(G_UI_HASH_TAG), StringArena());

	if (stringArray.count > 1)
		result = stringArray.strings[1];

	return result;
}

UiBox *GetUiBoxOfStringKeyLastFrame(String stringKey)
{
	UiBox *result = {};

	if (stringKey.length)
	{
		unsigned int hashvalue = Murmur3String(C_STRING_NULL_TERMINATED(stringKey), 0);

		for (u32 i = 0; i < stringKey.length; i++)
		{
			unsigned int index = (hashvalue + i) % ARRAY_COUNT(G_UI_STATE->uiHashEntries);
			UiHashEntry *uiHashEntry = &G_UI_STATE->uiHashEntries[index];
			if (uiHashEntry->uiBox && uiHashEntry->keyString == stringKey && uiHashEntry->uiBox->frameRendered == G_CURRENT_FRAME - 1)
			{
				result = uiHashEntry->uiBox;
				break;
			}
		}
	}

	return result;
}

UiBox *GetUiBoxOfStringLastFrame(String string)
{
	String keyString = GetUiBoxKeyStringOfString(string);
	UiBox *result = GetUiBoxOfStringKeyLastFrame(keyString);
	return result;
}

String CreateScriptStringKey(unsigned int scriptLineNumber)
{
	String result = STRING("scriptEditor_line") + U32ToString(scriptLineNumber, StringArena());
	return result;
}

//TODO: this could be a hash
UiBox *GetUiBoxLastFrameOfStringKey(String stringKey)
{
	UiBox *result = {};

	int uiBoxArrayIndex = GetFrameModIndexLastFrame();
	for (u32 uiBoxIndex = 0; uiBoxIndex < ARRAY_COUNT(G_UI_STATE->uiBoxes[uiBoxArrayIndex]); uiBoxIndex++)
	{
		UiBox *uiBox = &G_UI_STATE->uiBoxes[uiBoxArrayIndex][uiBoxIndex];
		if (uiBoxIndex != uiBox->index)
			break;

		if (uiBox->keyString == stringKey)
		{
			result = uiBox;
			break;
		}
	}

	return result;
}

v2 GetCeneteredPosInRectV2(RectV2 rect, v2 dim)
{
	dim.x = ClampF32(0, dim.x, rect.dim.x);
	dim.y = ClampF32(0, dim.y, rect.dim.y);
	v2 result = (rect.dim * 0.5f) - (dim * 0.5f);
	return result;
}

void RenderUiEntries(UiBox *uiBox, v2 windowPixelDim, int uiDepth = 0)
{
	if (uiBox)
	{
		int drawOrder = 0;

		if (IsFlag(uiBox, UI_FLAG_DRAW_BACKGROUND) && uiBox->uiSettings.backColor.a)
		{
			uiBox->rect.dim.x = ClampF32(0, uiBox->rect.dim.x, windowPixelDim.x);
			uiBox->rect.dim.y = ClampF32(0, uiBox->rect.dim.y, windowPixelDim.y);
			Rectangle rect = RectToRayRectangle(uiBox->rect);
			DrawRectangleRec(rect, uiBox->uiSettings.backColor);
		}

		if (IsFlag(uiBox, UI_FLAG_DRAW_TEXT))
		{
			v2 pos = uiBox->rect.pos;

			if (IsFlag(uiBox, UI_FLAG_ALIGN_TEXT_CENTERED))
			{
				pos += GetCeneteredPosInRectV2(uiBox->rect, uiBox->textDim);
			}
			else if (IsFlag(uiBox, UI_FLAG_ALIGN_TEXT_RIGHT))
			{
				pos.x += uiBox->rect.dim.x - uiBox->textDim.x;
			}

			DrawTextPro(uiBox->uiSettings.font, C_STRING_NULL_TERMINATED(uiBox->string), V2ToRayVector(pos), {}, 0, (f32) uiBox->uiSettings.font.baseSize, 1, uiBox->uiSettings.frontColor);
		}

		if (IsFlag(uiBox, UI_FLAG_DRAW_TEXTURE))
		{
			ASSERT(uiBox->uiInputs.texture.id);

			float scale = uiBox->rect.dim.x / uiBox->uiInputs.texture.width;
			Texture texture = uiBox->uiInputs.texture;
			Rectangle source = {0.0f, 0.0f, (float)texture.width, (float)texture.height};
			Rectangle dest = {uiBox->rect.pos.x, uiBox->rect.pos.y, (float)texture.width * scale, (float)texture.height * scale};

			if (IsFlag(uiBox, UI_FLAG_ALIGN_TEXTURE_CENTERED))
			{
				v2 dim;
				dim.x = dest.width;
				dim.y = dest.height;
				v2 relativePos = GetCeneteredPosInRectV2(uiBox->rect, dim);
				dest.x += relativePos.x;
				dest.y += relativePos.y;
			}

			DrawTexturePro(uiBox->uiInputs.texture, source, dest, Vector2{0, 0}, 0, WHITE);
		}

		if (IsFlag(uiBox, UI_FLAG_DRAW_BORDER))
		{
			RectV2 rect = uiBox->rect;
			//TODO: (Ahmayk) Bleh. Need to have concept in UI of pixel-perfect positioning vs not
			DrawRectangleLines((u32)rect.pos.x, (u32)rect.pos.y, (u32)rect.dim.x, (u32)rect.dim.y, uiBox->uiSettings.borderColor);
		}

		RenderUiEntries(uiBox->firstChild, windowPixelDim, uiDepth + 2);
		RenderUiEntries(uiBox->next, windowPixelDim, uiDepth);
	}
}

void CreateUiButton(String string, ReactiveUiColorState reactiveUiColorState, bool active, bool disabled = false)
{
	ReactiveUiColor reactiveUiColor = (active)
		? reactiveUiColorState.active
		: reactiveUiColorState.nonActive;
	UiBox *uiBoxLastFrame = GetUiBoxOfStringLastFrame(string);
	G_UI_STATE->uiSettings.backColor = GetReactiveColor(uiBoxLastFrame, reactiveUiColor, disabled);

	UiSettings *uiSettings = &G_UI_STATE->uiSettings;
	uiSettings->frontColor = BLACK;
	uiSettings->detailColor = BLACK;
	uiSettings->borderColor = (active) ? BLACK : GRAY;

	unsigned int flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER | UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_CENTERED | UI_FLAG_INTERACTABLE;
	CreateUiBox(flags, string);
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

static float G_TOOLBOX_WIDTH_AND_HEIGHT = 35;

void CreateBrushEffectButton(BRUSH_EFFECT brushEffect, String string, Color baseColor, COMMAND command, Brush *currentBrush)
{
	SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
	String stringButton = string + G_UI_HASH_TAG + U32ToString(brushEffect, StringArena());
	ReactiveUiColorState uiColorState = CreateButtonReactiveUiColorState(baseColor);
	bool active = currentBrush->brushEffect == brushEffect;
	G_UI_INPUTS->command = command;
	CreateUiButton(stringButton, uiColorState, active, false);
}

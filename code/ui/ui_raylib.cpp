
#include "ui/ui_raylib.h"
#include "../includes/raylib/src/raylib.h"
#include "vn_math_external.h"

//TODO: (Ahmayk) REMOVE
#include "main.h"

v2 GetCeneteredPosInRectV2(RectV2 rect, v2 dim)
{
	dim.x = ClampF32(0, dim.x, rect.dim.x);
	dim.y = ClampF32(0, dim.y, rect.dim.y);
	v2 result = (rect.dim * 0.5f) - (dim * 0.5f);
	return result;
}

Font *FindRaylibFont(UiFont uiFont, RaylibRenderData *raylibRenderData)
{
	Font *result = {};

	for (u32 i = 0; i < raylibRenderData->fontCount; i++)
	{
		if (raylibRenderData->fonts[i].texture.id == uiFont.id)
		{
			result = &raylibRenderData->fonts[i];
			break;
		}
	}

	if (!ASSERT(result))
	{
		static Font stub;
		result = &stub;
		*result = {};
	}

	return result;
}

void UiRenderBlockRaylib(UiBlock *uiBlock, int uiDepth, RaylibRenderData *raylibRenderData)
{
	if (uiBlock)
	{
		int drawOrder = 0;

		if (IsFlag(uiBlock, UI_FLAG_DRAW_BACKGROUND) && uiBlock->uiSettings.backColor.a)
		{
			//uiBlock->rect.dim.x = ClampF32(0, uiBlock->rect.dim.x, windowPixelDim.x);
			//uiBlock->rect.dim.y = ClampF32(0, uiBlock->rect.dim.y, windowPixelDim.y);
			Rectangle rect = RectToRayRectangle(uiBlock->rect);
			DrawRectangleRec(rect, uiBlock->uiSettings.backColor);
		}

		if (IsFlag(uiBlock, UI_FLAG_DRAW_TEXT))
		{
			v2 pos = uiBlock->rect.pos;

			if (IsFlag(uiBlock, UI_FLAG_ALIGN_TEXT_CENTERED))
			{
				pos += GetCeneteredPosInRectV2(uiBlock->rect, uiBlock->textDim);
			}
			else if (IsFlag(uiBlock, UI_FLAG_ALIGN_TEXT_RIGHT))
			{
				pos.x += uiBlock->rect.dim.x - uiBlock->textDim.x;
			}

			Font *font = FindRaylibFont(uiBlock->uiFont, raylibRenderData);

			DrawTextPro(*font, C_STRING_NULL_TERMINATED(uiBlock->string), V2ToRayVector(pos), {}, 0, (f32) uiBlock->uiFont.baseSize, 1, uiBlock->uiSettings.frontColor);
		}

		if (IsFlag(uiBlock, UI_FLAG_DRAW_TEXTURE) && ASSERT(uiBlock->texture.id))
		{
			float scale = uiBlock->rect.dim.x / uiBlock->texture.width;
			Texture texture = uiBlock->texture;
			Rectangle source = {0.0f, 0.0f, (float)texture.width, (float)texture.height};
			Rectangle dest = {uiBlock->rect.pos.x, uiBlock->rect.pos.y, (float)texture.width * scale, (float)texture.height * scale};

			if (IsFlag(uiBlock, UI_FLAG_ALIGN_TEXTURE_CENTERED))
			{
				v2 dim;
				dim.x = dest.width;
				dim.y = dest.height;
				v2 relativePos = GetCeneteredPosInRectV2(uiBlock->rect, dim);
				dest.x += relativePos.x;
				dest.y += relativePos.y;
			}

			DrawTexturePro(uiBlock->texture, source, dest, Vector2{0, 0}, 0, WHITE);
		}

		if (IsFlag(uiBlock, UI_FLAG_DRAW_BORDER))
		{
			RectV2 rect = uiBlock->rect;
			//TODO: (Ahmayk) Bleh. Need to have concept in UI of pixel-perfect positioning vs not
			DrawRectangleLines((u32)rect.pos.x, (u32)rect.pos.y, (u32)rect.dim.x, (u32)rect.dim.y, uiBlock->uiSettings.borderColor);
		}

		UiRenderBlockRaylib(uiBlock->firstChild, uiDepth + 2, raylibRenderData);
		UiRenderBlockRaylib(uiBlock->next, uiDepth, raylibRenderData);
	}
}

void UiRenderBlocksRaylib(UiBuffer *uiBuffer, RaylibRenderData *raylibRenderData)
{
	for (u32 i = 1; i < uiBuffer->uiBlockCount; i++)
	{
		UiBlock *uiBlock = &uiBuffer->uiBlocks[i];
		if (!uiBlock->parent)
		{
			UiRenderBlockRaylib(uiBlock, 0, raylibRenderData);
		}
	}
}

void UiRaylibProcessStrings(UiBuffer *uiBuffer, RaylibRenderData *raylibRenderData)
{
	for (u32 i = 1; i < uiBuffer->uiBlockCount; i++)
	{
		UiBlock *uiBlock = &uiBuffer->uiBlocks[i];
		if (uiBlock->string.length)
		{
			uiBlock->string = ReallocString(uiBlock->string, &uiBuffer->arena);
			if (ASSERT(uiBlock->uiFont.baseSize) && ASSERT(uiBlock->uiFont.id))
			{
				Font *font = FindRaylibFont(uiBlock->uiFont, raylibRenderData);
				//TODO: (Ahmayk) move this here so we can pass in string and don't need to reallocate for null pointer
				Vector2 textDim = MeasureTextEx(*font, C_STRING_NULL_TERMINATED(uiBlock->string), (f32) uiBlock->uiFont.baseSize, 1);
				uiBlock->textDim = RayVectorToV2(textDim);
			}
		}
	}
}


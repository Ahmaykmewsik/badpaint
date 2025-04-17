
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

void UiRenderBlockRaylib(UiBlock *uiBlock, int uiDepth)
{
	if (uiBlock)
	{
		int drawOrder = 0;

		if ((uiBlock->flags & UI_FLAG_DRAW_BACKGROUND) && uiBlock->uiBlockColors.backColor.a > 0)
		{
			//uiBlock->rect.dim.x = ClampF32(0, uiBlock->rect.dim.x, windowPixelDim.x);
			//uiBlock->rect.dim.y = ClampF32(0, uiBlock->rect.dim.y, windowPixelDim.y);
			Rectangle rect = RectToRayRectangle(uiBlock->rect);
			Color color = ColorU32ToRayColor(uiBlock->uiBlockColors.backColor);
			DrawRectangleRec(rect, color);
		}

		if (uiBlock->flags & UI_FLAG_DRAW_TEXT)
		{
			v2 pos = uiBlock->rect.pos;

			if (uiBlock->flags & UI_FLAG_ALIGN_TEXT_CENTERED)
			{
				pos += GetCeneteredPosInRectV2(uiBlock->rect, uiBlock->textDim);
			}
			else if (uiBlock->flags & UI_FLAG_ALIGN_TEXT_RIGHT)
			{
				pos.x += uiBlock->rect.dim.x - uiBlock->textDim.x;
			}

			Font *font = (Font*) uiBlock->uiFont.data;
			if (ASSERT(font) && ASSERT(uiBlock->uiFont.id == (u32) font->texture.id))
			{
				Color color = ColorU32ToRayColor(uiBlock->uiBlockColors.frontColor);
				DrawTextPro(*font, C_STRING_NULL_TERMINATED(uiBlock->string), V2ToRayVector(pos), {}, 0, (f32) font->baseSize, 1, color);
			}
		}

		if (uiBlock->flags & UI_FLAG_DRAW_TEXTURE)
		{
			Texture *texture = (Texture *) uiBlock->uiTexture.data;
			if (ASSERT(texture) && ASSERT(uiBlock->uiTexture.id == texture->id))
			{
				float scale = uiBlock->rect.dim.x / texture->width;
				Rectangle source = {0.0f, 0.0f, (f32)texture->width, (f32)texture->height};
				Rectangle dest = {uiBlock->rect.pos.x, uiBlock->rect.pos.y, (f32)texture->width * scale, (f32)texture->height * scale};

				if (uiBlock->flags & UI_FLAG_ALIGN_TEXTURE_CENTERED)
				{
					v2 dim;
					dim.x = dest.width;
					dim.y = dest.height;
					v2 relativePos = GetCeneteredPosInRectV2(uiBlock->rect, dim);
					dest.x += relativePos.x;
					dest.y += relativePos.y;
				}

				DrawTexturePro(*texture, source, dest, Vector2{0, 0}, 0, WHITE);
			}
		}

		if (uiBlock->flags & UI_FLAG_DRAW_BORDER)
		{
			RectV2 rect = uiBlock->rect;
			Color color = ColorU32ToRayColor(uiBlock->uiBlockColors.borderColor);
			//TODO: (Ahmayk) Bleh. Need to have concept in UI of pixel-perfect positioning vs not
			DrawRectangleLines((u32)rect.pos.x, (u32)rect.pos.y, (u32)rect.dim.x, (u32)rect.dim.y, color);
		}

		UiRenderBlockRaylib(uiBlock->firstChild, uiDepth + 2);
		UiRenderBlockRaylib(uiBlock->next, uiDepth);
	}
}

void UiRaylibRenderBlocks(UiBuffer *uiBuffer)
{
	for (u32 i = 1; i < uiBuffer->uiBlockCount; i++)
	{
		UiBlock *uiBlock = &uiBuffer->uiBlocks[i];
		if (!uiBlock->parent)
		{
			UiRenderBlockRaylib(uiBlock, 0);
		}
	}
}

void UiRaylibProcessStrings(UiBuffer *uiBuffer)
{
	for (u32 i = 1; i < uiBuffer->uiBlockCount; i++)
	{
		UiBlock *uiBlock = &uiBuffer->uiBlocks[i];
		if (uiBlock->string.length)
		{
			uiBlock->string = ReallocString(uiBlock->string, &uiBuffer->arena);
			Font *font = (Font*) uiBlock->uiFont.data;
			if (ASSERT(font) && ASSERT(uiBlock->uiFont.id == (u32) font->texture.id))
			{
				//TODO: (Ahmayk) move this here so we can pass in string and don't need to reallocate for null pointer
				Vector2 textDim = MeasureTextEx(*font, C_STRING_NULL_TERMINATED(uiBlock->string), (f32) font->baseSize, 1);
				uiBlock->textDim = RayVectorToV2(textDim);
			}
		}
	}
}

UiTexture UiRaylibTextureToUiTexture(Texture *texture)
{
	UiTexture result;
	result.id = texture->id;
	result.dim.x = (i32) texture->width;
	result.dim.y = (i32) texture->height;
	result.data = texture;
	return result;
}


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
			v2 pos = {};
			for (u32 i = 0; i < ARRAY_COUNT(uiBlock->uiTextAlignTypes); i++)
			{
				switch(uiBlock->uiTextAlignTypes[i])
				{
					case UI_TEXT_ALIGN_LEFT:
					{
						pos.elements[i] = uiBlock->rect.pos.elements[i];
					} break;
					case UI_TEXT_ALIGN_CENTER:
					{
						f32 textLengthInBlock = MinF32(uiBlock->textDim.elements[i], uiBlock->rect.dim.elements[i]);
						f32 offset = (uiBlock->rect.dim.elements[i] * 0.5f) - (textLengthInBlock * 0.5f);
						pos.elements[i] = uiBlock->rect.pos.elements[i] + offset;
					} break;
					case UI_TEXT_ALIGN_RIGHT:
					{
						pos.elements[i] = uiBlock->rect.pos.elements[i] + uiBlock->rect.dim.elements[i] - uiBlock->textDim.elements[i];
					} break;
					InvalidDefaultCase;
				}
			}

			//NOTE: (Ahmayk) snap to grid, raylib produces artifacts if we don't do this
			pos = RoundV2(pos);

			Font *font = (Font*) uiBlock->uiFont.data;
			if (ASSERT(font) && ASSERT(uiBlock->uiFont.id == (u32) font->texture.id))
			{
				Color color = ColorU32ToRayColor(uiBlock->uiBlockColors.frontColor);
				DrawTextPro(*font, C_STRING_NULL_TERMINATED(uiBlock->string), V2ToRayVector(pos), {}, 0, (f32) font->baseSize, 1, color);
			}
		}

		if (uiBlock->flags & UI_FLAG_DRAW_TEXTURE)
		{
			Texture *texture = (Texture *) uiBlock->uiTextureView.data;
			if (ASSERT(texture) &&
				ASSERT(uiBlock->uiTextureView.id == texture->id) &&
				ASSERT(uiBlock->uiTextureView.dim.x == texture->width) &&
				ASSERT(uiBlock->uiTextureView.dim.y == texture->height))
			{
				RectIV2 *viewRect = &uiBlock->uiTextureView.viewRect;
				Rectangle source = {(f32)viewRect->pos.x, (f32)viewRect->pos.x, (f32)viewRect->dim.x, (f32)viewRect->dim.y};
				Rectangle dest = RectToRayRectangle(uiBlock->rect);
				DrawTexturePro(*texture, source, dest, Vector2{0, 0}, 0, WHITE);
			}
		}

		if (uiBlock->flags & UI_FLAG_DRAW_BORDER)
		{
			RectV2 rect = uiBlock->rect;
			//NOTE: (Ahmayk) draw border around block, not on top of it
			rect.dim.x += 1;
			rect.dim.y += 1;
			Color color = ColorU32ToRayColor(uiBlock->uiBlockColors.borderColor);
			//TODO: (Ahmayk) Bleh. Need to have concept in UI of pixel-perfect positioning vs not
			DrawRectangleLines((u32)rect.pos.x, (u32)rect.pos.y, (u32)rect.dim.x, (u32)rect.dim.y, color);
		}

		if (uiBlock->flags & UI_FLAG_DRAW_LINE_TOPLEFT_BOTTOMRIGHT)
		{
			iv2 start;
			iv2 end;
			start.x = (u32) RoundF32(uiBlock->rect.pos.x);
			start.y = (u32) RoundF32(uiBlock->rect.pos.y);
			end.x = (u32) RoundF32(uiBlock->rect.pos.x + uiBlock->rect.dim.x);
			end.y = (u32) RoundF32(uiBlock->rect.pos.y + uiBlock->rect.dim.y);
			Color color = ColorU32ToRayColor(uiBlock->uiBlockColors.frontColor);
			DrawLine(start.x, start.y, end.x, end.y, color);
		}

		if (uiBlock->flags & UI_FLAG_DRAW_LINE_BOTTOMLEFT_TOPRIGHT)
		{
			iv2 start;
			iv2 end;
			start.x = (u32) RoundF32(uiBlock->rect.pos.x);
			start.y = (u32) RoundF32(uiBlock->rect.pos.y + uiBlock->rect.dim.y);
			end.x = (u32) RoundF32(uiBlock->rect.pos.x + uiBlock->rect.dim.x);
			end.y = (u32) RoundF32(uiBlock->rect.pos.y);
			Color color = ColorU32ToRayColor(uiBlock->uiBlockColors.frontColor);
			DrawLine(start.x, start.y, end.x, end.y, color);
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

UiTextureView UiRaylibTextureToUiTextureView(Texture *texture)
{
	UiTextureView result = {};
	result.id = texture->id;
	result.dim.x = (i32) texture->width;
	result.dim.y = (i32) texture->height;
	result.viewRect.dim = result.dim;
	result.data = texture;
	return result;
}

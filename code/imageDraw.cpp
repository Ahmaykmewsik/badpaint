#include "image.h"
#include "imageDraw.h"

#include <cstring>  //memcpy

BadpaintPixel CreateBadpaintPixel(BADPAINT_PIXEL_TYPE badpaintPixelType, ImageBadpaintPixels *imageBadpaintPixels, iv2 endPosIV2, u32 toolSize, u8 processBatchIndex)
{
	BadpaintPixel result = {};
	result.badpaintPixelType = badpaintPixelType;
	switch(result.badpaintPixelType)
	{
		case BADPAINT_PIXEL_TYPE_RANDOM:
		{
			result.r1RandomValue = (u8) RandomInRangeI32(0, 255);
		} break;
		case BADPAINT_PIXEL_TYPE_COPY_OTHER_PIXEL:
		{
			i32 minX = endPosIV2.x - toolSize;
			i32 minY = endPosIV2.y - toolSize;
			i32 maxX = endPosIV2.x + toolSize;
			i32 maxY = endPosIV2.y + toolSize;
			i32 posX = RandomInRangeI32(minX, maxX);
			i32 posY = RandomInRangeI32(minY, maxY);
			result.r2PosX = (u16) ClampI32(0, posX, imageBadpaintPixels->dim.x);
			result.r3PosY = (u16) ClampI32(0, posY, imageBadpaintPixels->dim.y);
		} break;
	}
	result.processBatchIndex = processBatchIndex;
	return result;
}

void CanvasSetDirtyRect(ImageBadpaintPixels *imageBadpaintPixels, RectIV2 updateArea)
{
	u32 numX = (u32) CeilF32(imageBadpaintPixels->dim.x / (f32) imageBadpaintPixels->drawingRectDim.x);
	u32 numY = (u32) CeilF32(imageBadpaintPixels->dim.y / (f32) imageBadpaintPixels->drawingRectDim.y);
	u32 indexX = MaxI32(0, updateArea.pos.x) / imageBadpaintPixels->drawingRectDim.x;
	u32 indexY = MaxI32(0, updateArea.pos.y) / imageBadpaintPixels->drawingRectDim.y;

	RectIV2 drawRect;
	drawRect.dim = imageBadpaintPixels->drawingRectDim;
	for (u32 y = indexY; y < numY; y++)
	{
		drawRect.pos.y = y * imageBadpaintPixels->drawingRectDim.x;
		b32 interceptsThisRow = false;
		for (u32 x = indexX; x < numX; x++)
		{
			drawRect.pos.x = x * imageBadpaintPixels->drawingRectDim.x;
			if (!IsInterceptRectIV2(updateArea, drawRect))
			{
				break;
			}
			interceptsThisRow = true;
			u32 dirtyIndexX = drawRect.pos.x / imageBadpaintPixels->drawingRectDim.x;
			u32 dirtyIndexY = drawRect.pos.y / imageBadpaintPixels->drawingRectDim.y;
			u32 index = dirtyIndexX + (dirtyIndexY * numX);
			if (ASSERT(index < imageBadpaintPixels->drawingRectCount))
			{
				imageBadpaintPixels->drawingRectDirtyListFrame[index] = true;
				imageBadpaintPixels->drawingRectDirtyListProcess[index] = true;
			}
		}
		if (!interceptsThisRow)
		{
			break;
		}
	}
}

void ProcessEdge(iv2 a, iv2 b, i32 y, i32 *xmin, i32 *xmax)
{
	if (a.y == b.y)
	{
		if (y == a.y)
		{
			i32 left = a.x < b.x ? a.x : b.x;
			i32 right = a.x > b.x ? a.x : b.x;
			if (left < *xmin) *xmin = left;
			if (right > *xmax) *xmax = right;
		}
	}
	else if ((a.y <= y && y <= b.y) || (b.y <= y && y <= a.y))
	{
		f32 t = (f32) (y - a.y) / (b.y - a.y);
		//TODO: (Ahmayk) Worth it?
		i32 x = a.x + (i32) RoundF32((t * (b.x - a.x)));
		if (x < *xmin) *xmin = x;
		if (x > *xmax) *xmax = x;
	}
}

b32 CanvasImageBadpaintDrawPixel(ImageBadpaintPixels *dst, i32 index, BadpaintPixel *badpaintPixel)
{
	b32 result = false;
	BadpaintPixel *badpaintPixelDest = &dst->dataBadpaintPixel[index];

	switch (badpaintPixel->badpaintPixelType)
	{
	case BADPAINT_PIXEL_TYPE_NONE:
	case BADPAINT_PIXEL_TYPE_REMOVE:
	case BADPAINT_PIXEL_TYPE_MAX:
	{
		result = badpaintPixelDest->badpaintPixelType != badpaintPixel->badpaintPixelType;
	} break;
	case BADPAINT_PIXEL_TYPE_SHIFT:
	{
		result = badpaintPixelDest->badpaintPixelType != badpaintPixel->badpaintPixelType;
	} break;
	case BADPAINT_PIXEL_TYPE_RANDOM:
	{
		//NOTE: (Ahmayk) assume that random value is probably different! No real need to do check here for performance
		result = true;
	} break;
	case BADPAINT_PIXEL_TYPE_COPY_OTHER_PIXEL:
	{
		b32 typedMatch = badpaintPixelDest->badpaintPixelType == badpaintPixel->badpaintPixelType &&
			(badpaintPixelDest->r2PosX != badpaintPixel->r2PosX || 
			badpaintPixelDest->r3PosY != badpaintPixel->r3PosY);
		result = badpaintPixelDest->badpaintPixelType != badpaintPixel->badpaintPixelType || typedMatch;
	} break;
	InvalidDefaultCase
	}

	if (result) 
	{
		*badpaintPixelDest = *badpaintPixel;
	}
	return result;
}

b32 CanvasFillConvexQuad(ImageBadpaintPixels *imageBadpaintPixels, iv2 p0, iv2 p1, iv2 p2, iv2 p3, BadpaintPixel *badpaintPixel)
{
	b32 result = false;
	iv2 minIV2 = p0;
	iv2 maxIV2 = p0;
    iv2 points[] = {p0, p1, p2, p3};
    for (u32 i = 1; i < 4; i++)
	{
        if (points[i].x < minIV2.x)
		{
			minIV2.x = points[i].x;
		}
        if (points[i].x > maxIV2.x)
		{
			maxIV2.x = points[i].x;
		}
        if (points[i].y < minIV2.y)
		{
			minIV2.y = points[i].y;
		}
        if (points[i].y > maxIV2.y)
		{
			maxIV2.y = points[i].y;
		}
    }

	minIV2.y = ClampI32(0, minIV2.y, imageBadpaintPixels->dim.y - 1);
	maxIV2.y = ClampI32(0, maxIV2.y, imageBadpaintPixels->dim.y - 1);

    for (i32 y = minIV2.y; y <= maxIV2.y; y++)
	{
        i32 xmin = imageBadpaintPixels->dim.x - 1;
        i32 xmax = 0;
        ProcessEdge(p0, p1, y, &xmin, &xmax);
        ProcessEdge(p1, p2, y, &xmin, &xmax);
        ProcessEdge(p2, p3, y, &xmin, &xmax);
        ProcessEdge(p3, p0, y, &xmin, &xmax);
		xmin = ClampI32(0, xmin, imageBadpaintPixels->dim.x - 1);
		xmax = ClampI32(0, xmax, imageBadpaintPixels->dim.x - 1);

        if (xmin <= xmax)
		{
			u32 startIndex = (y * imageBadpaintPixels->dim.x) + xmin;
			u32 endIndex = (y * imageBadpaintPixels->dim.x) + xmax;
            for (u32 i = startIndex; i <= endIndex; i++)
			{
				result |= CanvasImageBadpaintDrawPixel(imageBadpaintPixels, i, badpaintPixel);
            }
        }
    }

	if (result)
	{
		RectIV2 updateArea;
		updateArea.pos = minIV2;
		updateArea.dim = maxIV2 - minIV2 + 1;
		CanvasSetDirtyRect(imageBadpaintPixels, updateArea);
	}
	return result;
}

b32 CanvasImageDrawRectangle(ImageBadpaintPixels *imageBadpaintPixels, int posX, int posY, int width, int height, BadpaintPixel *badpaintPixel)
{
	b32 result = false;
    // Security check to avoid drawing out of bounds in case of bad user data
    if (posX < 0) { width += posX; posX = 0; }
    if (posY < 0) { height += posY; posY = 0; }
    if (width < 0) width = 0;
    if (height < 0) height = 0;

    // Clamp the size the the image bounds
    if ((posX + width) >= imageBadpaintPixels->dim.x) width = imageBadpaintPixels->dim.x - posX;
    if ((posY + height) >= imageBadpaintPixels->dim.y) height = imageBadpaintPixels->dim.y - posY;

    // Check if the  is even inside the image
    if ((posX >= imageBadpaintPixels->dim.x) || (posY >= imageBadpaintPixels->dim.y)) return result;
    if (((posX + width) <= 0) || (posY + height <= 0)) return result;

	u32 startY = posY;
	u32 endY = startY + height;
	for (u32 y = startY; y < endY; y++)
	{
		u32 startIndex = (imageBadpaintPixels->dim.x * y) + posX;
		u32 endIndex = startIndex + width;
		for (u32 i = startIndex; i < endIndex; i++)
		{
			result |= CanvasImageBadpaintDrawPixel(imageBadpaintPixels, i, badpaintPixel);
		}
	}

   return result;
}

b32 CanvasDrawCircle(ImageBadpaintPixels *imageBadpaintPixels, iv2 pos, u32 radius, BadpaintPixel *badpaintPixel)
{
	b32 result = false;

	u32 centerX = pos.x;
	u32 centerY = pos.y;
    int x = 0;
    int y = radius;
    int decesionParameter = 3 - 2*radius;

    while (y >= x)
    {
        result |= CanvasImageDrawRectangle(imageBadpaintPixels, centerX - x, centerY + y, x*2, 1, badpaintPixel);
        result |= CanvasImageDrawRectangle(imageBadpaintPixels, centerX - x, centerY - y, x*2, 1, badpaintPixel);
        result |= CanvasImageDrawRectangle(imageBadpaintPixels, centerX - y, centerY + x, y*2, 1, badpaintPixel);
        result |= CanvasImageDrawRectangle(imageBadpaintPixels, centerX - y, centerY - x, y*2, 1, badpaintPixel);
        x++;

        if (decesionParameter > 0)
        {
            y--;
            decesionParameter = decesionParameter + 4*(x - y) + 10;
        } 
        else decesionParameter = decesionParameter + 4*x + 6;
    }

	if (result)
	{
		RectIV2 updateArea;
		updateArea.dim.x = radius * 2 + 1;
		updateArea.dim.y = radius * 2 + 1;
		updateArea.pos.x = pos.x - radius;
		updateArea.pos.y = pos.y - radius;
		CanvasSetDirtyRect(imageBadpaintPixels, updateArea);
	}
	return result;
}

b32 CanvasDrawCircleStroke(ImageBadpaintPixels *imageBadpaintPixels, iv2 startPos, iv2 endPos, u32 radius, BadpaintPixel *badpaintPixel)
{
	b32 result = false;

	result |= CanvasDrawCircle(imageBadpaintPixels, startPos, radius, badpaintPixel);

	if (startPos != endPos)
	{
		result |= CanvasDrawCircle(imageBadpaintPixels, endPos, radius, badpaintPixel);

		i32 dx = endPos.x - startPos.x;
		i32 dy = endPos.y - startPos.y;
		f32 length = SqrtI32(dx*dx + dy*dy);
		if (length > 2)
		{
			f32 px = (-dy * (i32) radius) / length;
			f32 py = (dx * (i32) radius) / length;
			iv2 points[4] =
			{
				{ (i32)RoundF32(startPos.x + px), (i32)RoundF32(startPos.y + py) },
				{ (i32)RoundF32(startPos.x - px), (i32)RoundF32(startPos.y - py) },
				{ (i32)RoundF32(endPos.x   - px), (i32)RoundF32(endPos.y   - py) },
				{ (i32)RoundF32(endPos.x   + px), (i32)RoundF32(endPos.y   + py) }
			};
			result |= CanvasFillConvexQuad(imageBadpaintPixels, points[0], points[1], points[2], points[3], badpaintPixel);
		}
	}
	return result;
}

b32 CanvasDrawSpray(ImageBadpaintPixels *imageBadpaintPixels, iv2 posIV2, u32 toolSize, BadpaintPixel *badpaintPixel)
{
	b32 result = false;
	for (u32 i = 0; i < 55; i++)
	{
		f32 randomRadius = RandomF32ZeroToOne() * (toolSize * 0.5f);
		f32 randomAngleDegree = RandomF32ZeroToOne() * 360;
		iv2 sprayPixel = posIV2;
		sprayPixel.x += (i32) RoundF32(randomRadius * CosF32(randomAngleDegree)); 
		sprayPixel.y += (i32) RoundF32(randomRadius * SinF32(randomAngleDegree)); 
		if (sprayPixel.x >= 0 && sprayPixel.x < imageBadpaintPixels->dim.x &&
			sprayPixel.y >= 0 && sprayPixel.y < imageBadpaintPixels->dim.y)
		{
			u32 index = (sprayPixel.y * imageBadpaintPixels->dim.x) + sprayPixel.x;
			result |= CanvasImageBadpaintDrawPixel(imageBadpaintPixels, index, badpaintPixel);
		}
	}
	if (result)
	{
		RectIV2 updateArea;
		updateArea.dim.x = toolSize + 1;
		updateArea.dim.y = toolSize + 1;
		updateArea.pos.x = posIV2.x - (u32) CeilF32(toolSize * 0.5f);
		updateArea.pos.y = posIV2.y - (u32) CeilF32(toolSize * 0.5f);
		CanvasSetDirtyRect(imageBadpaintPixels, updateArea);
	}
	return result;
}

u8 *GetPNGFilteredPixel(ImagePNGFiltered *imagePNGFiltered, iv2 pos)
{
	u8 *result = {};
	if (ASSERT(pos.x <= imagePNGFiltered->dim.x && pos.x >= 0 && pos.y <= imagePNGFiltered->dim.y && pos.y >= 0))
	{
		u32 filterByteOffset = pos.y + 1;
		result = imagePNGFiltered->dataU8 + (4 * ((imagePNGFiltered->dim.x * pos.y) + pos.x)) + filterByteOffset;
	}
	else
	{
		result = imagePNGFiltered->dataU8;
	}

	return result;
}

u8 *GetImageRawRGBA32Pixel(ImageRawRGBA32 *imageRaw, iv2 pos)
{
	u8 *result = {};
	if (ASSERT(pos.x <= imageRaw->dim.x && pos.x >= 0 && pos.y <= imageRaw->dim.y && pos.y >= 0))
	{
		result = imageRaw->dataU8 + (4 * ((imageRaw->dim.x * pos.y) + pos.x));
	}
	else
	{
		result = imageRaw->dataU8;
	}

	return result;
}

b32 ImageSwapPoints(ImageRawRGBA32 *imageRaw, iv2 pos1, iv2 pos2)
{
	b32 result = false;

	pos1.x = ClampI32(0, pos1.x, imageRaw->dim.x - 1);
	pos1.y = ClampI32(0, pos1.y, imageRaw->dim.y - 1);
	pos2.x = ClampI32(0, pos2.x, imageRaw->dim.x - 1);
	pos2.y = ClampI32(0, pos2.y, imageRaw->dim.y - 1);
	
	u8 *pixel1 = GetImageRawRGBA32Pixel(imageRaw, pos1);
	u8 *pixel2 = GetImageRawRGBA32Pixel(imageRaw, pos2);

	u32 tempPixel;
	memcpy(&tempPixel, pixel1, sizeof(u32));
	memcpy(pixel1, pixel2, sizeof(u32));
	memcpy(pixel2, &tempPixel, sizeof(u32));

	//Image dst = ImageRawToRayImage(&canvas->drawnImageData);
	//CanvasImageDrawPixel(&dst, pos1.x, pos1.y, Color{1, 0, 0, 0});
	//CanvasImageDrawPixel(&dst, pos2.x, pos2.y, Color{1, 0, 0, 0});

	result = true;

#if 0
	if (result)
	{
		RectIV2 updateArea;
		updateArea.dim.x = 1; 
		updateArea.dim.y = 1;
		updateArea.pos = pos1;
		CanvasSetDirtyRect(canvas, updateArea);
		updateArea.pos = pos2;
		CanvasSetDirtyRect(canvas, updateArea);
	}
#endif

	return result;
}


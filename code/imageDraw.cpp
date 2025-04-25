#include "image.h"
#include "imageDraw.h"

#include <cstring>  //memcpy

void CanvasSetDirtyRect(Canvas *canvas, RectIV2 updateArea)
{
	u32 numX = (u32) CeilF32(canvas->drawnImageData.dim.x / (f32) canvas->drawingRectDim.x);
	u32 numY = (u32) CeilF32(canvas->drawnImageData.dim.y / (f32) canvas->drawingRectDim.y);
	u32 indexX = MaxI32(0, updateArea.pos.x) / canvas->drawingRectDim.x;
	u32 indexY = MaxI32(0, updateArea.pos.y) / canvas->drawingRectDim.y;

	RectIV2 drawRect;
	drawRect.dim = canvas->drawingRectDim;
	for (u32 y = indexY; y < numY; y++)
	{
		drawRect.pos.y = y * canvas->drawingRectDim.x;
		b32 interceptsThisRow = false;
		for (u32 x = indexX; x < numX; x++)
		{
			drawRect.pos.x = x * canvas->drawingRectDim.x;
			if (!IsInterceptRectIV2(updateArea, drawRect))
			{
				break;
			}
			interceptsThisRow = true;
			u32 dirtyIndexX = drawRect.pos.x / canvas->drawingRectDim.x;
			u32 dirtyIndexY = drawRect.pos.y / canvas->drawingRectDim.y;
			u32 index = dirtyIndexX + (dirtyIndexY * numX);
			if (ASSERT(index < canvas->drawingRectCount))
			{
				canvas->drawingRectDirtyListFrame[index] = true;
				canvas->drawingRectDirtyListProcess[index] = true;
			}
		}
		if (!interceptsThisRow)
		{
			break;
		}
	}
}

Image ImageRawToRayImage(ImageRawRGBA32 *imageRaw)
{
	Image result;
	result.data = imageRaw->dataU8;
	result.width = imageRaw->dim.x;
	result.height = imageRaw->dim.y;
	result.mipmaps = 1;
	result.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	return result;
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

b32 CanvasImageDrawPixel(Image *dst, int x, int y, Color color)
{
	b32 result = false;
	u8 r = ((unsigned char *)dst->data)[(y*dst->width + x)*4];
	if (r != color.r) 
	{
		((unsigned char *)dst->data)[(y*dst->width + x)*4] = color.r;
		((unsigned char *)dst->data)[(y*dst->width + x)*4 + 1] = color.g;
		((unsigned char *)dst->data)[(y*dst->width + x)*4 + 2] = color.b;
		((unsigned char *)dst->data)[(y*dst->width + x)*4 + 3] = color.a;
		result = true;
	}
	return result;
}

b32 CanvasFillConvexQuad(Canvas *canvas, iv2 p0, iv2 p1, iv2 p2, iv2 p3, Color color)
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

	minIV2.y = ClampI32(0, minIV2.y, canvas->drawnImageDataRoot.dim.y - 1);
	maxIV2.y = ClampI32(0, maxIV2.y, canvas->drawnImageDataRoot.dim.y - 1);

	Image tempImage = ImageRawToRayImage(&canvas->drawnImageDataRoot);
    for (i32 y = minIV2.y; y <= maxIV2.y; y++)
	{
        i32 xmin = canvas->drawnImageData.dim.x - 1;
        i32 xmax = 0;
        ProcessEdge(p0, p1, y, &xmin, &xmax);
        ProcessEdge(p1, p2, y, &xmin, &xmax);
        ProcessEdge(p2, p3, y, &xmin, &xmax);
        ProcessEdge(p3, p0, y, &xmin, &xmax);
		//xmin = ClampI32(0, xmin, canvas->drawnImageData.dim.x - 1);
		//xmax = ClampI32(0, xmax, canvas->drawnImageData.dim.x - 1);

        if (xmin <= xmax)
		{
            for (i32 x = xmin; x <= xmax; x++)
			{
				result |= CanvasImageDrawPixel(&tempImage, x, y, color);
            }
        }
    }

	if (result)
	{
		RectIV2 updateArea;
		updateArea.pos = minIV2;
		updateArea.dim = maxIV2 - minIV2 + 1;
		CanvasSetDirtyRect(canvas, updateArea);
	}
	return result;
}

b32 CanvasImageDrawRectangle(Image *dst, int posX, int posY, int width, int height, Color color)
{
	b32 result = false;
    // Security check to avoid drawing out of bounds in case of bad user data
    if (posX < 0) { width += posX; posX = 0; }
    if (posY < 0) { height += posY; posY = 0; }
    if (width < 0) width = 0;
    if (height < 0) height = 0;

    // Clamp the size the the image bounds
    if ((posX + width) >= dst->width) width = dst->width - posX;
    if ((posY + height) >= dst->height) height = dst->height - posY;

    // Check if the  is even inside the image
    if ((posX >= dst->width) || (posY >= dst->height)) return result;
    if (((posX + width) <= 0) || (posY + height <= 0)) return result;

	u32 startY = posY;
	u32 endY = startY + height;
	for (u32 y = startY; y < endY; y++)
	{
		u32 startIndex = (dst->width * y) + posX;
		u32 endIndex = startIndex + width;
		for (u32 i = startIndex; i < endIndex; i++)
		{
			Color *pixelDest = &((Color*)dst->data)[i];
			if (pixelDest->r != color.r)
			{
				*pixelDest = color;
				result |= true;
			}
		}
	}

   return result;
}

b32 CanvasDrawCircle(Canvas *canvas, iv2 pos, u32 radius, Color color)
{
	b32 result = false;

	Image dst = ImageRawToRayImage(&canvas->drawnImageDataRoot);
	u32 centerX = pos.x;
	u32 centerY = pos.y;
    int x = 0;
    int y = radius;
    int decesionParameter = 3 - 2*radius;

    while (y >= x)
    {
        result |= CanvasImageDrawRectangle(&dst, centerX - x, centerY + y, x*2, 1, color);
        result |= CanvasImageDrawRectangle(&dst, centerX - x, centerY - y, x*2, 1, color);
        result |= CanvasImageDrawRectangle(&dst, centerX - y, centerY + x, y*2, 1, color);
        result |= CanvasImageDrawRectangle(&dst, centerX - y, centerY - x, y*2, 1, color);
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
		CanvasSetDirtyRect(canvas, updateArea);
	}
	return result;
}

b32 CanvasDrawCircleStroke(Canvas *canvas, iv2 startPos, iv2 endPos, u32 radius, Color color)
{
	b32 result = false;

	result |= CanvasDrawCircle(canvas, startPos, radius, color);

	if (startPos != endPos)
	{
		result |= CanvasDrawCircle(canvas, endPos, radius, color);

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
			result |= CanvasFillConvexQuad(canvas, points[0], points[1], points[2], points[3], color);
		}
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


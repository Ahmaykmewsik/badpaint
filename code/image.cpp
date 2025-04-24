#pragma once

#include "image.h"
#include "imageProcess.h"
#include "main.h"
#include "platform_win32.h"

#include "../includes/raylib/src/rlgl.h"

#include "../includes/raylib/src/external/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../includes/raylib/src/external/stb_image_write.h"

#include "../includes/raylib/src/external/glad.h" // GLAD extensions loading library, includes OpenGL headers

void UploadAndReplaceTexture(ImageRawRGBA32 *imageRaw, Texture *texture)
{
	if (ASSERT(imageRaw->dataU8))
	{
		if (texture->id)
		{
			rlUnloadTexture(texture->id);
		}
		*texture = {};

		texture->id = rlLoadTexture(imageRaw->dataU8, imageRaw->dim.x, imageRaw->dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
		texture->width = imageRaw->dim.x;
		texture->height = imageRaw->dim.y;
		texture->mipmaps = 1;
		texture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	}
	// GenTextureMipmaps(texture);
}

void UpdateRectInTexture(Texture *texture, void *data, RectIV2 rect)
{
	if (data)
	{
		rlUpdateTexture(texture->id, rect.pos.x, rect.pos.y, rect.dim.x, rect.dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, data);
	}
	else
	{
		//TODO: Log error
		// Print("Empty image passed into texture uploader!");
	}
}

void SetPNGFilterType(Canvas *canvas, ImageRawRGBA32 *rootImageRaw, GameMemory *gameMemory)
{
	LockOnBool(&canvas->filterLock);
	ArenaReset(&canvas->arenaFilteredPNG);
	canvas->imagePNGFiltered = PiratedSTB_EncodePngFilters(rootImageRaw, &canvas->arenaFilteredPNG, canvas->currentPNGFilterType);

#if 0
#if DEBUG_MODE
	MemoryProtectWrite(canvas->arenaFilteredPNG.memory, canvas->arenaFilteredPNG.used);
#endif
#endif

	UnlockOnBool(&canvas->filterLock);

	iv2 canvasDim = rootImageRaw->dim; 
	u32 visualizedCanvasDataSize = canvasDim.x * canvasDim.y * sizeof(Color);

	ArenaMarker marker = {};
	ImageRawRGBA32 visualizedFilteredRootImage = {};
	visualizedFilteredRootImage.dim = canvasDim;
	visualizedFilteredRootImage.dataSize = visualizedCanvasDataSize;
	visualizedFilteredRootImage.dataU8 = (u8*) ArenaPushSize(&gameMemory->temporaryArena, visualizedCanvasDataSize, &marker);
	for (u32 y = 0; y < (u32) canvasDim.y; y++)
	{
		u32 filterByteOffset = y + 1;
		u32 startIndex = canvasDim.x * y;
		u32 endIndex = startIndex + canvasDim.x;

		u8 *scanlineFiltered = canvas->imagePNGFiltered.dataU8 + (startIndex * 4) + filterByteOffset;
		u8 *scanlineVisualized = visualizedFilteredRootImage.dataU8 + (startIndex * 4);
		memcpy(scanlineVisualized, scanlineFiltered, canvasDim.x * 4);

		for (u32 i = startIndex; i < endIndex; i++)
		{
			u8 *filteredPixel = canvas->imagePNGFiltered.dataU8 + (i * 4) + filterByteOffset;
			u8 *visualizedPixel = visualizedFilteredRootImage.dataU8 + (i * 4);
			visualizedPixel[3] = 255;
		}
	}

	if (!canvas->textureVisualizedFilteredRootImage.id)
	{
		canvas->textureVisualizedFilteredRootImage.id = rlLoadTexture(visualizedFilteredRootImage.dataU8, visualizedFilteredRootImage.dim.x, visualizedFilteredRootImage.dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
		canvas->textureVisualizedFilteredRootImage.width = canvasDim.x;
		canvas->textureVisualizedFilteredRootImage.height = canvasDim.y; 
		canvas->textureVisualizedFilteredRootImage.mipmaps = 1;
		canvas->textureVisualizedFilteredRootImage.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	}
	RectIV2 rect = {};
	rect.dim = canvasDim;
	UpdateRectInTexture(&canvas->textureVisualizedFilteredRootImage, visualizedFilteredRootImage.dataU8, rect);
	ArenaPopMarker(marker);
}

RectIV2 GetDrawingRectFromIndex(iv2 imageDim, iv2 rectDim, u32 i)
{
	RectIV2 result;
	result.dim = rectDim; 

	u32 numX = (u32) CeilF32(imageDim.x / (f32) rectDim.x);
	u32 indexX = i % numX;
	u32 indexY = ((u32) FloorF32(i / (f32) numX));
	result.pos.x = result.dim.x * indexX;
	result.pos.y = result.dim.y * indexY;

	if (result.pos.x + result.dim.x > imageDim.x)
	{
		result.dim.x = imageDim.x - result.pos.x;
	}
	if (result.pos.y + result.dim.y > imageDim.y)
	{
		result.dim.y = imageDim.y - result.pos.y;
	}
	ASSERT(result.dim.y > 0);

	return result;
}

u32 GetDrawingRectCount(iv2 imageDim, iv2 rectDim)
{
	u32 numX = (u32) CeilF32(imageDim.x / (f32) rectDim.x);
	u32 numY = (u32) CeilF32(imageDim.y / (f32) rectDim.y);
	u32 result = numX * numY;
	return result;
}

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

	minIV2.y = ClampI32(0, minIV2.y, canvas->drawnImageData.dim.y - 1);
	maxIV2.y = ClampI32(0, maxIV2.y, canvas->drawnImageData.dim.y - 1);

	Image tempImage = ImageRawToRayImage(&canvas->drawnImageData);
    for (i32 y = minIV2.y; y <= maxIV2.y; y++)
	{
        i32 xmin = INT_MAX;
        i32 xmax = INT_MIN;
        ProcessEdge(p0, p1, y, &xmin, &xmax);
        ProcessEdge(p1, p2, y, &xmin, &xmax);
        ProcessEdge(p2, p3, y, &xmin, &xmax);
        ProcessEdge(p3, p0, y, &xmin, &xmax);
		xmin = ClampI32(0, xmin, canvas->drawnImageData.dim.x - 1);
		xmax = ClampI32(0, xmax, canvas->drawnImageData.dim.x - 1);

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

	Image dst = ImageRawToRayImage(&canvas->drawnImageData);
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

b32 CanvasSwapPoints(Canvas *canvas, iv2 pos1, iv2 pos2)
{
	b32 result = false;

	pos1.x = ClampI32(0, pos1.x, canvas->imagePNGFiltered.dim.x);
	pos1.y = ClampI32(0, pos1.y, canvas->imagePNGFiltered.dim.y);
	pos2.x = ClampI32(0, pos2.x, canvas->imagePNGFiltered.dim.x);
	pos2.y = ClampI32(0, pos2.y, canvas->imagePNGFiltered.dim.y);

#if 1
	LockOnBool(&canvas->filterLock);
	
	u8 *pixel1 = GetPNGFilteredPixel(&canvas->imagePNGFiltered, pos1);
	u8 *pixel2 = GetPNGFilteredPixel(&canvas->imagePNGFiltered, pos2);

	u32 tempPixel;
	memcpy(&tempPixel, pixel1, sizeof(u32));
	memcpy(pixel1, pixel2, sizeof(u32));
	memcpy(pixel2, &tempPixel, sizeof(u32));

	UnlockOnBool(&canvas->filterLock);
#endif

	//Image dst = ImageRawToRayImage(&canvas->drawnImageData);
	//CanvasImageDrawPixel(&dst, pos1.x, pos1.y, Color{1, 0, 0, 0});
	//CanvasImageDrawPixel(&dst, pos2.x, pos2.y, Color{1, 0, 0, 0});

	result = true;

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

	return result;
}

//TODO: (Ahmayk) This takes a good deal of time now, the exspensive stuff should be offloaded to another thread
//And we have a fun little loading animation or something
void InitializeCanvas(Canvas *canvas, ImageRawRGBA32 *rootImageRaw, GameMemory *gameMemory)
{
	//NOTE: (Ahmayk) free and reallocate temporary arena to reduce memory size
	//to uncommit the memory there, will reduce our memory footprint
	if (ASSERT(gameMemory->temporaryArena.used == 0))
	{
		u64 size = gameMemory->temporaryArena.size;
		ArenaFree(&gameMemory->temporaryArena);
		gameMemory->temporaryArena = ArenaInit(size);
		gameMemory->temporaryArena.memory;
	}

	ArenaFree(&gameMemory->canvasArena);

	iv2 canvasDim = rootImageRaw->dim;
	u32 visualizedCanvasDataSize = canvasDim.x * canvasDim.y * sizeof(Color);
	u32 conversionArenaSize = visualizedCanvasDataSize + (rootImageRaw->dim.x * 4);
	conversionArenaSize = (u32) (conversionArenaSize * 1.2f);
	AlignPow2U32(&conversionArenaSize, 256);

	//NOTE: (Ahmayk) drawn image, visualized image, drawingRects, dirtyRectsForEachProcesssImage, finalImageRectHashes
	u32 canvasArneaSize = (u32) MaxU32(MegaByte * 1, (u32) (conversionArenaSize * 2.1f));
	gameMemory->canvasArena = ArenaInit(canvasArneaSize);
	//TODO: (Ahmayk) have a better undo plan!
	ArenaFree(&gameMemory->canvasRollbackArena);
	gameMemory->canvasRollbackArena = ArenaInit(MegaByte * 800);

	ArenaGroupFree(&gameMemory->conversionArenaGroup);
	u32 conversionArenaGroupSize = MaxU32(MegaByte * 400, conversionArenaSize * 2 + (KiloByte * 16));
	AlignPow2U32(&conversionArenaSize, 4096);
	gameMemory->conversionArenaGroup = ArenaGroupInit(conversionArenaGroupSize);

	if (gameMemory->temporaryArena.memory && 
		gameMemory->canvasArena.memory &&
		gameMemory->canvasRollbackArena.memory &&
		gameMemory->conversionArenaGroup.masterArena.memory)
	{
		canvas->drawnImageData.dataU8 = ArenaPushSize(&gameMemory->canvasArena, visualizedCanvasDataSize, {});
		canvas->drawnImageData.dim = canvasDim;
		canvas->drawnImageData.dataSize = visualizedCanvasDataSize;
		memset(canvas->drawnImageData.dataU8, 0, visualizedCanvasDataSize);

		canvas->arenaFilteredPNG = ArenaInitFromArena(&gameMemory->canvasArena, conversionArenaSize);

		ArenaGroupResetAndFill(&gameMemory->conversionArenaGroup, conversionArenaSize);

		UploadAndReplaceTexture(&canvas->drawnImageData, &canvas->textureDrawing);

		if (canvas->textureVisualizedFilteredRootImage.id)
		{
			rlUnloadTexture(canvas->textureVisualizedFilteredRootImage.id);
			canvas->textureVisualizedFilteredRootImage = {};
		}
		SetPNGFilterType(canvas, rootImageRaw, gameMemory);

		if (canvas->initialized)
		{
			glDeleteBuffers(ARRAY_COUNT(canvas->drawingPboIDs), canvas->drawingPboIDs);
		}
		glGenBuffers(ARRAY_COUNT(canvas->drawingPboIDs), canvas->drawingPboIDs);
		for(u32 i = 0; i < ARRAY_COUNT(canvas->drawingPboIDs); i++) 
		{
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, canvas->drawingPboIDs[i]);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, visualizedCanvasDataSize, NULL, GL_STREAM_DRAW);
		}
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		if (canvas->initialized)
		{
			glDeleteBuffers(ARRAY_COUNT(canvas->finalImagePboIDs), canvas->finalImagePboIDs);
		}
		glGenBuffers(ARRAY_COUNT(canvas->finalImagePboIDs), canvas->finalImagePboIDs);
		for(u32 i = 0; i < ARRAY_COUNT(canvas->finalImagePboIDs); i++) 
		{
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, canvas->finalImagePboIDs[i]);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, rootImageRaw->dim.x * rootImageRaw->dim.y * 4, NULL, GL_STREAM_DRAW);
		}
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		canvas->finalImageRectDim = iv2{512, 512};
		canvas->finalImageRectCount = GetDrawingRectCount(rootImageRaw->dim, canvas->finalImageRectDim);
		canvas->cachedFinalImageRectHashes = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->finalImageRectCount, u32);
		HashImageRects(rootImageRaw, canvas->finalImageRectDim, &canvas->cachedFinalImageRectHashes);

		canvas->rollbackSizeCount = (u32) FloorF32((f32)gameMemory->canvasRollbackArena.size / visualizedCanvasDataSize) - 1;
		canvas->rollbackImageData = ARENA_PUSH_ARRAY(&gameMemory->canvasRollbackArena, canvas->rollbackSizeCount * visualizedCanvasDataSize, u8);
		canvas->rollbackIndexStart = 0;
		canvas->rollbackIndexNext = 0;
		canvas->rollbackStartHasProgressed = {};
		canvas->rollbackHasRolledBackOnce = {};
		canvas->saveRollbackOnNextPress = {};
		canvas->dataOnCanvas = {};

		canvas->drawingRectDim = iv2{32, 32};
		canvas->drawingRectCount = GetDrawingRectCount(canvasDim, canvas->drawingRectDim);
		canvas->drawingRectDirtyListFrame = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->drawingRectCount, b32);
		canvas->drawingRectDirtyListProcess = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->drawingRectCount, b32);
		memset(canvas->drawingRectDirtyListFrame, 0, canvas->drawingRectCount * sizeof(b32));
		memset(canvas->drawingRectDirtyListProcess, 0, canvas->drawingRectCount * sizeof(b32));

		canvas->initialized = true;
	}
	else
	{
		ArenaFree(&gameMemory->canvasArena);
		ArenaFree(&gameMemory->canvasRollbackArena);
		ArenaGroupFree(&gameMemory->conversionArenaGroup);
	}
}

b32 InitializeNewImage(GameMemory *gameMemory, ImageRawRGBA32 *rootImageRaw, Canvas *canvas, Texture *loadedTexture, ProcessedImage *processedImages, u32 threadCount)
{
	b32 result = false;
	if (rootImageRaw->dataU8)
	{
		InitializeCanvas(canvas, rootImageRaw, gameMemory);
		if (canvas->initialized)
		{
			UploadAndReplaceTexture(rootImageRaw, loadedTexture);
			for (u32 i = 0; i < threadCount; i++)
			{
				ProcessedImage *processedImage = processedImages + i;
				processedImage->dirtyRectsInProcess = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->drawingRectCount, b32);
				processedImage->finalImageRectHashes = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->finalImageRectCount, u32);
			}
		}
		else
		{
			//NOTE: (Ahmayk) The only way canvas init can fail is for this reason.
			//Not sure how error handling should work yet...will need to change this once there's more than 1 way for this to fail
			if (rootImageRaw->dataSize > MegaByte * 500)
			{
				String notification = STRING("Yikes, this thing is huge! Sorry, but I failed to allocate the memory I needed to handle that. I'll need to be more memory efficent to handle that monster!");
				//TODO: (Ahmayk) Handle error reporting outside this function!
				//InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
			}
			else
			{
				String notification = STRING("Failed to allocate the memory I needed to handle the new image! Man, not your day, huh?");
				//TODO: (Ahmayk) Handle error reporting outside this function!
				//InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
			}
			*rootImageRaw = {};
		}
	}
	return result;
}



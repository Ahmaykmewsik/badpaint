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

void InitTextureGPU(TextureGPU *textureGPU, ImageRawRGBA32 *imageRaw)
{
	if (textureGPU->texture.id)
	{
		rlUnloadTexture(textureGPU->texture.id);
		glDeleteBuffers(ARRAY_COUNT(textureGPU->pboIDs), textureGPU->pboIDs);
	}
	*textureGPU = {};

	textureGPU->dim = imageRaw->dim;
	textureGPU->texture.id = rlLoadTexture(imageRaw->dataU8, textureGPU->dim.x, textureGPU->dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
	textureGPU->texture.width = textureGPU->dim.x;
	textureGPU->texture.height = textureGPU->dim.y;
	textureGPU->texture.mipmaps = 1; 
	textureGPU->texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8; 
	glGenBuffers(ARRAY_COUNT(textureGPU->pboIDs), textureGPU->pboIDs);
	for(u32 i = 0; i < ARRAY_COUNT(textureGPU->pboIDs); i++) 
	{
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, textureGPU->pboIDs[i]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, imageRaw->dataSize, NULL, GL_STREAM_DRAW);
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
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

	//NOTE: (Ahmayk) drawn image, drawingRects, dirtyRectsForEachProcesssImage, finalImageRectHashes
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
		canvas->rootBadpaintPixels.dataBadpaintPixel = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, rootImageRaw->dim.x * rootImageRaw->dim.y, BadpaintPixel);
		canvas->rootBadpaintPixels.dim = canvasDim;
		canvas->rootBadpaintPixels.dataSize = visualizedCanvasDataSize;
		memset(canvas->rootBadpaintPixels.dataBadpaintPixel, 0, visualizedCanvasDataSize);

		ImageRawRGBA32 tempImageRaw = {};
		tempImageRaw.dataU8 = (u8*) canvas->rootBadpaintPixels.dataBadpaintPixel;
		tempImageRaw.dim = rootImageRaw->dim;
		tempImageRaw.dataSize = rootImageRaw->dataSize; 
		InitTextureGPU(&canvas->textureGPUDrawing, &tempImageRaw);

		ArenaGroupResetAndFill(&gameMemory->conversionArenaGroup, conversionArenaSize);

		if (canvas->textureVisualizedFilteredRootImage.id)
		{
			rlUnloadTexture(canvas->textureVisualizedFilteredRootImage.id);
			canvas->textureVisualizedFilteredRootImage = {};
		}


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

		canvas->rootBadpaintPixels.drawingRectDim = iv2{32, 32};
		canvas->rootBadpaintPixels.drawingRectCount = GetDrawingRectCount(canvasDim, canvas->rootBadpaintPixels.drawingRectDim);
		canvas->rootBadpaintPixels.drawingRectDirtyListFrame = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->rootBadpaintPixels.drawingRectCount, b32);
		canvas->rootBadpaintPixels.drawingRectDirtyListProcess = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->rootBadpaintPixels.drawingRectCount, b32);
		memset(canvas->rootBadpaintPixels.drawingRectDirtyListFrame, 0, canvas->rootBadpaintPixels.drawingRectCount * sizeof(b32));
		memset(canvas->rootBadpaintPixels.drawingRectDirtyListProcess, 0, canvas->rootBadpaintPixels.drawingRectCount * sizeof(b32));

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
				processedImage->dirtyRectsInProcess = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->rootBadpaintPixels.drawingRectCount, b32);
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

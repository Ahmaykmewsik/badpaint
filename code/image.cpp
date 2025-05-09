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

ImageBadpaintPixels InitBadpaintPixelImage(Arena *arena, ImageRawRGBA32 *imageRaw)
{
	ImageBadpaintPixels result = {};
	iv2 dim = imageRaw->dim;
	result.dataBadpaintPixel = ARENA_PUSH_ARRAY(arena, dim.x * dim.y, BadpaintPixel);
	result.dim = dim;
	result.dataSize = dim.x * dim.y * 4;
	memset(result.dataBadpaintPixel, 0, result.dataSize);
	result.drawingRectDim = iv2{32, 32};
	result.drawingRectCount = GetDrawingRectCount(dim, result.drawingRectDim);
	result.drawingRectDirtyListFrame = ARENA_PUSH_ARRAY(arena, result.drawingRectCount, b32);
	result.drawingRectDirtyListProcess = ARENA_PUSH_ARRAY(arena, result.drawingRectCount, b32);
	memset(result.drawingRectDirtyListFrame, 0, result.drawingRectCount * sizeof(b32));
	memset(result.drawingRectDirtyListProcess, 0, result.drawingRectCount * sizeof(b32));
	InitTextureGPU(&result.textureGPU, imageRaw);
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

	//TODO: (Ahmayk) figure out what the upper bound of this actually should be 
	u32 canvasArneaSize = (u32) MaxU32(MegaByte * 1, (u32) (conversionArenaSize * 10));
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
		ArenaMarker blankMarker;
		ImageRawRGBA32 tempImageRawBlank = {};
		tempImageRawBlank.dataSize = rootImageRaw->dim.x * rootImageRaw->dim.y * 4;
		tempImageRawBlank.dataU8 = ArenaPushSize(&gameMemory->temporaryArena, tempImageRawBlank.dataSize, &blankMarker);
		tempImageRawBlank.dim = rootImageRaw->dim; 
		memset(tempImageRawBlank.dataU8, 0, tempImageRawBlank.dataSize);

		canvas->badpaintPixelsRootImage = InitBadpaintPixelImage(&gameMemory->canvasArena, &tempImageRawBlank);
		canvas->badpaintPixelsPNGFiltered = InitBadpaintPixelImage(&gameMemory->canvasArena, &tempImageRawBlank);
		canvas->badpaintPixelsFinalImage = InitBadpaintPixelImage(&gameMemory->canvasArena, &tempImageRawBlank);

		InitTextureGPU(&canvas->textureGPURoot, &tempImageRawBlank);
		InitTextureGPU(&canvas->textureGPUPNGFiltered, &tempImageRawBlank);
		InitTextureGPU(&canvas->textureGPUFinal, &tempImageRawBlank);

		ArenaPopMarker(blankMarker);

		ArenaGroupResetAndFill(&gameMemory->conversionArenaGroup, conversionArenaSize);

		canvas->finalImageRectDim = iv2{512, 512};
		canvas->finalImageRectCount = GetDrawingRectCount(rootImageRaw->dim, canvas->finalImageRectDim);
		canvas->cachedFinalImageRectHashes = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->finalImageRectCount, u32);
		//NOTE: (Ahmayk) leave garbage data in cache, will upload first image. then will be replaced

		canvas->rollbackSizeCount = (u32) FloorF32((f32)gameMemory->canvasRollbackArena.size / visualizedCanvasDataSize) - 1;
		canvas->rollbackImageData = ARENA_PUSH_ARRAY(&gameMemory->canvasRollbackArena, canvas->rollbackSizeCount * visualizedCanvasDataSize, u8);
		canvas->rollbackIndexStart = 0;
		canvas->rollbackIndexNext = 0;
		canvas->rollbackStartHasProgressed = {};
		canvas->rollbackHasRolledBackOnce = {};
		canvas->saveRollbackOnNextPress = {};
		canvas->dataOnCanvas = {};


		canvas->initialized = true;
		canvas->proccessAsap = true;
	}
	else
	{
		ArenaFree(&gameMemory->canvasArena);
		ArenaFree(&gameMemory->canvasRollbackArena);
		ArenaGroupFree(&gameMemory->conversionArenaGroup);
	}
}

b32 InitializeNewImage(GameMemory *gameMemory, AppState *appState)
{
	b32 result = false;
	if (appState->rootImageRaw.dataU8)
	{
		InitializeCanvas(&appState->canvas, &appState->rootImageRaw, gameMemory);
		if (appState->canvas.initialized)
		{
			for (u32 i = 0; i < appState->processedImageCount; i++)
			{
				ProcessedImage *processedImage = appState->processedImages + i;
				processedImage->dirtyRectsInProcess = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, appState->canvas.badpaintPixelsRootImage.drawingRectCount, b32);
				processedImage->finalImageRectHashes = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, appState->canvas.finalImageRectCount, u32);
			}
		}
		else
		{
			//NOTE: (Ahmayk) The only way canvas init can fail is for this reason.
			//Not sure how error handling should work yet...will need to change this once there's more than 1 way for this to fail
			if (appState->rootImageRaw.dataSize > MegaByte * 500)
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
			appState->rootImageRaw = {};
		}
	}
	return result;
}

void RenderAndUploadBadpaintPixelImage(ImageBadpaintPixels *imageBadpaintPixels)
{
	b32 somethingToDraw = false;
	for (u32 rectIndex = 0; rectIndex < imageBadpaintPixels->drawingRectCount; rectIndex++)
	{
		if (imageBadpaintPixels->drawingRectDirtyListFrame[rectIndex])
		{
			somethingToDraw = true;
			break;
		}
	}

	if (somethingToDraw)
	{
		TextureGPU *textureGPU = &imageBadpaintPixels->textureGPU;

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, textureGPU->pboIDs[textureGPU->currentPboID]);
		ColorU32 *pixels = (ColorU32 *) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		if (ASSERT(pixels))
		{
			for (u32 rectIndex = 0; rectIndex < imageBadpaintPixels->drawingRectCount; rectIndex++)
			{
				if (imageBadpaintPixels->drawingRectDirtyListFrame[rectIndex])
				{
					RectIV2 drawingRect = GetDrawingRectFromIndex(imageBadpaintPixels->dim, imageBadpaintPixels->drawingRectDim, rectIndex);
					u32 startY = drawingRect.pos.y;
					u32 endY = startY + drawingRect.dim.y;
					for (u32 y = startY; y < endY; y++)
					{
						u32 startIndex = (imageBadpaintPixels->dim.x * y) + drawingRect.pos.x;
						u32 endIndex = startIndex + drawingRect.dim.x;
						for (u32 i = startIndex; i < endIndex; i++)
						{
							BadpaintPixel *badpaintPixel = &imageBadpaintPixels->dataBadpaintPixel[i];
							ColorU32 *outPixel = (pixels + i);
							//processBatchIndex != 0 -> is being processed currently
							if (badpaintPixel->processBatchIndex != 0)
							{
								*outPixel = BADPAINT_PIXEL_TYPE_COLORS_PROCESSING[badpaintPixel->badpaintPixelType];
							}
							else
							{
								*outPixel = BADPAINT_PIXEL_TYPE_COLORS_PRIMARY[badpaintPixel->badpaintPixelType];
							}
						}
					}
				}
			}
			glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		}

		glBindTexture(GL_TEXTURE_2D, textureGPU->texture.id);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, textureGPU->dim.x);

		for (u32 rectIndex = 0; rectIndex < imageBadpaintPixels->drawingRectCount; rectIndex++)
		{
			if (imageBadpaintPixels->drawingRectDirtyListFrame[rectIndex])
			{
				RectIV2 drawingRect = GetDrawingRectFromIndex(imageBadpaintPixels->dim, imageBadpaintPixels->drawingRectDim, rectIndex);
				GLintptr offset = (drawingRect.pos.y * textureGPU->dim.x + drawingRect.pos.x) * sizeof(ColorU32);
				glTexSubImage2D(GL_TEXTURE_2D, 0, drawingRect.pos.x, drawingRect.pos.y, drawingRect.dim.x, drawingRect.dim.y, GL_RGBA, GL_UNSIGNED_BYTE, (void*)offset);
				imageBadpaintPixels->drawingRectDirtyListFrame[rectIndex] = false;
			}
		}

		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		textureGPU->currentPboID = ModNextU32(textureGPU->currentPboID, ARRAY_COUNT(textureGPU->pboIDs));

		GenTextureMipmaps(&textureGPU->texture);
	}

}


#include "base/base.h"

//NOTE: (Ahmayk) hack to allow raw openGL calls in our code 
#include "../includes/raylib/src/external/glad.h" // GLAD extensions loading library, includes OpenGL headers

#include "../includes/raylib/src/raylib.h"
#include "../includes//raylib//src/external/glfw/include/GLFW/glfw3.h"

#include "ui/ui.h"
#include "ui/ui_raylib.h"
#include "widgets.h"
#include "vn_math_external.h"
#include "input.h"
#include "image.h"
#include "platform_win32.h"
#include "main.h"

#include "assets/font.h"
#include "assets/defaultImage.h"

#include <cstring>  //memcpy

void RunApp(PlatformWorkQueue *threadWorkQueue, GameMemory gameMemory, unsigned int threadCount)
{
	ImageRawRGBA32 *rootImageRaw = ARENA_PUSH_STRUCT(&gameMemory.permanentArena, ImageRawRGBA32);
	Canvas *canvas = ARENA_PUSH_STRUCT(&gameMemory.permanentArena, Canvas);

	canvas->currentPNGFilterType = PNG_FILTER_TYPE_OPTIMAL;

	bool imageIsBroken = {};

	ProcessedImage *processedImages = ARENA_PUSH_ARRAY(&gameMemory.permanentArena, threadCount, ProcessedImage);
	for (u32 i = 0; i < threadCount; i++)
	{
		ProcessedImage *processedImage = processedImages + i;
		processedImage->rootImageRaw = rootImageRaw;
		processedImage->canvas = canvas;
		processedImage->index = i;
	}

	UiInit(&gameMemory.permanentArena);

	UiState *G_UI_STATE = GetUiState();

	SetTraceLogLevel(LOG_NONE);

	InitWindow(200, 200, "badpaint");

	u32 refreshRate = 0;
	for (u32 i = 0; i < (u32) GetMonitorCount(); i++)
	{
		refreshRate = MaxU32(refreshRate, GetMonitorRefreshRate(i));
	}
	SetTargetFPS(refreshRate);

	SetWindowState(FLAG_WINDOW_RESIZABLE);

	iv2 screenDim = iv2{GetMonitorWidth(0), GetMonitorHeight(0)};

	iv2 windowDim;
	windowDim.x = (u32) RoundF32(screenDim.x * 0.8f);
	windowDim.y = (u32) RoundF32(screenDim.y * 0.8f);

	v2 windowPosMiddle = PositionInCenterV2(screenDim, windowDim);
	SetWindowPosition((u32) RoundF32(windowPosMiddle.x), (u32) RoundF32(windowPosMiddle.y));
	SetWindowSize(windowDim.x, windowDim.y);

	Font defaultFont = LoadFontFromMemory(".otf", PAINT_FONT_DATA, ARRAY_COUNT(PAINT_FONT_DATA), 18, 0, 0);

	Color deleteColor = RED;

	Brush currentBrush = {};
	currentBrush.brushEffect = BRUSH_EFFECT_REMOVE;
	currentBrush.size = 10;

	Texture loadedTexture = {};
	v2 pressedMousePos = {};
	//TODO: (Ahmayk) Remove
	u32 draggedHash = {};

	Slider brushSizeSlider = {};
	brushSizeSlider.sliderAction = SLIDER_ACTION_BRUSH_SIZE;
	brushSizeSlider.unsignedIntToChange = &currentBrush.size;
	brushSizeSlider.min = 1;
	brushSizeSlider.max = 50;

	*rootImageRaw = LoadDataIntoRawImage(&DEFAULT_IMAGE_DATA[0], ARRAY_COUNT(DEFAULT_IMAGE_DATA), &gameMemory);
	if (rootImageRaw->dataSize)
	{
		InitializeNewImage(&gameMemory, rootImageRaw, canvas, &loadedTexture, &currentBrush, processedImages, threadCount);
	}

	while (!WindowShouldClose())
	{
		f64 timeStart = GetTime();
		//NOTE: For crashing the games (useful for testing crash handler)
		if (IsKeyDown(KEY_C) &&
				IsKeyDown(KEY_R) &&
				IsKeyDown(KEY_A) &&
				IsKeyDown(KEY_S) &&
				IsKeyDown(KEY_H) &&
				IsKeyDown(KEY_RIGHT_CONTROL))
		{
			__debugbreak();
		}

		windowDim.x = GetScreenWidth();
		windowDim.y = GetScreenHeight();

		v2 mousePixelPos = v2{(float)GetMouseX(), (float)GetMouseY()};
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
		{
			ArenaReset(&gameMemory.mouseClickArena);
			draggedHash = {};

			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				pressedMousePos = mousePixelPos;
		}

		if (IsFileDropped())
		{
			FilePathList droppedFiles = LoadDroppedFiles();
			char *fileName = droppedFiles.paths[0];
			ArenaMarker loadMarker = ArenaPushMarker(&gameMemory.temporaryArena);
			unsigned int fileSize = {};
			u8 *fileData = LoadDataFromDisk(fileName, &fileSize, &gameMemory.temporaryArena);
			const char *getFileExtension = GetFileExtension(fileName);
			if (fileData)
			{
				//NOTE: (Ahmayk) prints error internally (may want to change?)
				*rootImageRaw = LoadDataIntoRawImage(fileData, fileSize, &gameMemory);
			}
			else
			{
				String notification = STRING("Oops! I failed to read the file at all! Sorry! Guess you're out of luck pal. No badpaint for that file today.");
				InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
			}
			UnloadDroppedFiles(droppedFiles);
			ArenaPopMarker(loadMarker);

			if (rootImageRaw->dataSize)
			{
				InitializeNewImage(&gameMemory, rootImageRaw, canvas, &loadedTexture, &currentBrush, processedImages, threadCount);
				if (rootImageRaw->dataSize > MegaByte * 500)
				{
					String notification = STRING("You like to play dangerously, don't you?");
					InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
				}
				else if (rootImageRaw->dataSize > MegaByte * 100)
				{
					String notification = STRING("This image is CHUNKY! Some things might be a little slow.");
					InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
				}
			}
		}

		UiBuffer *uiBufferLastFrame = &G_UI_STATE->uiBuffers[1 - G_UI_STATE->uiBufferIndex];

		for (u32 i = 0; i < uiBufferLastFrame->uiBlockCount; i++)
		{
			UiBlock *uiBlock = &uiBufferLastFrame->uiBlocks[i];
			if (IsFlag(uiBlock, UI_FLAG_INTERACTABLE))
			{
				uiBlock->cursorRelativePixelPos = mousePixelPos - uiBlock->rect.pos;

				if (IsInRectV2(mousePixelPos, uiBlock->rect))
				{
					uiBlock->hovered = true;
					uiBlock->pressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
					uiBlock->down = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
					if (uiBlock->pressed)
					{
						draggedHash = uiBlock->hash;
					}
				}

				if (uiBlock->sliderAction && draggedHash == uiBlock->hash)
				{
					float normPressedPosInRect = (pressedMousePos.x - uiBlock->rect.pos.x) / uiBlock->rect.dim.x;
					float normDifference = (pressedMousePos.x - mousePixelPos.x) / uiBlock->rect.dim.x;
					float normValue = ClampF32(0, normPressedPosInRect - normDifference, 1);

					//TODO: lookup slider
					*brushSizeSlider.unsignedIntToChange = (u32) RoundF32(LerpF32(brushSizeSlider.min, normValue, brushSizeSlider.max));
				}
			}
		}

		CommandInput commandInputs[COMMAND_COUNT] = {};

		for (u32 i = 0; i < COMMAND_COUNT; i++)
		{
			CommandInput *commandInput = &commandInputs[i];
			KeyboardKey key = {};
			if (ASSERT(i < ARRAY_COUNT(COMMAND_KEY_BINDINGS)))
			{
				key = COMMAND_KEY_BINDINGS[i];
			}
			if (key)
			{
				commandInput->down = IsKeyDown(key);
				commandInput->pressed = IsKeyPressed(key);
			}
		}

		for (u32 i = 0; i < uiBufferLastFrame->uiBlockCount; i++)
		{
			UiBlock *uiBlock = &uiBufferLastFrame->uiBlocks[i];
			if (IsFlag(uiBlock, UI_FLAG_INTERACTABLE))
			{
				if (uiBlock->command)
				{
					CommandInput *commandInput = commandInputs + uiBlock->command;
					commandInput->down |= uiBlock->down;
					commandInput->pressed |= uiBlock->pressed;
					uiBlock->down |= commandInput->down;
					uiBlock->pressed |= commandInput->pressed;
				}
			}
		}

		if (IsCommandPressed(commandInputs, COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE))
			currentBrush.brushEffect = BRUSH_EFFECT_ERASE;
		if (IsCommandPressed(commandInputs, COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE))
			currentBrush.brushEffect = BRUSH_EFFECT_REMOVE;
		if (IsCommandPressed(commandInputs, COMMAND_SWITCH_BRUSH_EFFECT_TO_MAX))
			currentBrush.brushEffect = BRUSH_EFFECT_MAX;
		if (IsCommandPressed(commandInputs, COMMAND_SWITCH_BRUSH_EFFECT_TO_SHIFT))
			currentBrush.brushEffect = BRUSH_EFFECT_SHIFT;
		if (IsCommandPressed(commandInputs, COMMAND_SWITCH_BRUSH_EFFECT_TO_RANDOM))
			currentBrush.brushEffect = BRUSH_EFFECT_RANDOM;

		if (IsCommandPressed(commandInputs, COMMAND_EXPORT_IMAGE))
		{
			if (loadedTexture.height && loadedTexture.width)
			{
				ArenaMarker arenaMarker = ArenaPushMarker(&gameMemory.temporaryArena);
				String filePath = AllocateString(256, &gameMemory.temporaryArena);
				u32 filepathLength;
				b32 success = GetPngImageFilePathFromUser(filePath.chars, filePath.length, &filepathLength);
				filePath.length = filepathLength;
				if (success && filePath.length)
				{
					Image exportImgae = LoadImageFromTexture(loadedTexture);
					ExportImage(exportImgae, filePath);

					String notification = STRING("You have given new life to: ") + filePath;
					InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
				}
				else
				{
					String notification = STRING("Sorry the save failed. You fail too. You suck. Sorry.");
					InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
				}
				ArenaPopMarker(arenaMarker);
			}
			else
			{
				String notification = STRING("Bruh.");
				InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
			}
		}

		u32 HASH_CANVAS = Murmur3String("canvas");
		u32 HASH_FINAL_TEXTURE = Murmur3String("finalTexture");

		UiBlock *canvasUiBlock = GetUiBlockOfHashLastFrame(HASH_CANVAS);
		UiBlock *finalTextureUiBlock = GetUiBlockOfHashLastFrame(HASH_FINAL_TEXTURE);

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && canvas->saveRollbackOnNextPress)
		{
			unsigned char *newRollbackImage = &canvas->rollbackImageData[(canvas->drawnImageData.dataSize * canvas->rollbackIndexNext)];
			memcpy(newRollbackImage, canvas->drawnImageData.dataU8, canvas->drawnImageData.dataSize);
			canvas->rollbackIndexNext =	ModNextU32(canvas->rollbackIndexNext, canvas->rollbackSizeCount - 1);
			if (canvas->rollbackIndexNext == canvas->rollbackIndexStart)
			{
				canvas->rollbackIndexStart = ModNextU32(canvas->rollbackIndexStart, canvas->rollbackSizeCount - 1);
				canvas->rollbackStartHasProgressed = true;
			}
			//printf("New rollback! start: %d next: %d\n", canvas->rollbackIndexStart, canvas->rollbackIndexNext);
			canvas->saveRollbackOnNextPress = false;
		}

		v2 cursorPosInDrawnImagePrevious = {};
		v2 cursorPosInDrawnImage = {};
		b32 isDownOnPaintable = false;
		b32 isHoveredOnPaintable = false;
		if (canvasUiBlock && canvasUiBlock->hovered)
		{
			f32 scale = canvas->drawnImageData.dim.x / canvasUiBlock->rect.dim.x;
			cursorPosInDrawnImagePrevious = (mousePixelPos - RayVectorToV2(GetMouseDelta()) - canvasUiBlock->rect.pos) * scale;
			cursorPosInDrawnImage = (mousePixelPos - canvasUiBlock->rect.pos) * scale;
			isDownOnPaintable = canvasUiBlock->down;
			isHoveredOnPaintable = true;
		}
		if (finalTextureUiBlock && finalTextureUiBlock->hovered)
		{
			f32 scale = rootImageRaw->dim.x / finalTextureUiBlock->rect.dim.x;
			cursorPosInDrawnImagePrevious = (mousePixelPos - RayVectorToV2(GetMouseDelta()) - finalTextureUiBlock->rect.pos) * scale;
			cursorPosInDrawnImage = (mousePixelPos - finalTextureUiBlock->rect.pos) * scale;
			isDownOnPaintable = finalTextureUiBlock->down;
			isHoveredOnPaintable = true;
		}

		if (!imageIsBroken && isDownOnPaintable)
		{
			Color colorToPaint = {};

			colorToPaint.r = (u8) currentBrush.brushEffect;
			colorToPaint.g = (u8) RandomInRangeI32(0, 255);
			colorToPaint.a = (u8) canvas->processBatchIndex;

			iv2 startPosIV2;
			startPosIV2.x = (u32) RoundF32(cursorPosInDrawnImagePrevious.x);
			startPosIV2.y = (u32) RoundF32(cursorPosInDrawnImagePrevious.y);
			iv2 endPosIV2;
			endPosIV2.x = (u32) RoundF32(cursorPosInDrawnImage.x);
			endPosIV2.y = (u32) RoundF32(cursorPosInDrawnImage.y);

			b32 drewSomething = CanvasDrawCircleStroke(canvas, startPosIV2, endPosIV2, currentBrush.size, colorToPaint);
			if (drewSomething)
			{
				canvas->proccessAsap = true;
				canvas->saveRollbackOnNextPress = true;
				canvas->dataOnCanvas = true;
			}
		}

		if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown((KEY_RIGHT_CONTROL))) && IsKeyPressed(KEY_Z))
		{
			if (canvas->rollbackIndexNext != canvas->rollbackIndexStart)
			{
				canvas->rollbackIndexNext =	ModBackU32(canvas->rollbackIndexNext, canvas->rollbackSizeCount - 1);
				u8 *rollbackImage = &canvas->rollbackImageData[(canvas->drawnImageData.dataSize * canvas->rollbackIndexNext)];
				memcpy(canvas->drawnImageData.dataU8, rollbackImage, canvas->drawnImageData.dataSize);
				memset(canvas->drawingRectDirtyListFrame, 1, canvas->drawingRectCount * sizeof(b32));
				canvas->proccessAsap = true;
				imageIsBroken = false;
				canvas->rollbackHasRolledBackOnce = true;
				canvas->saveRollbackOnNextPress = false;
				//printf("UNDO! start: %d next: %d\n", canvas->rollbackIndexStart, canvas->rollbackIndexNext);
			}
			else if (canvas->rollbackIndexNext == 0 && 
				canvas->rollbackIndexStart == 0 &&
				!canvas->rollbackStartHasProgressed &&
				canvas->dataOnCanvas)
			{
				memset(canvas->drawnImageData.dataU8, 0, canvas->drawnImageData.dataSize);
				memset(canvas->drawingRectDirtyListFrame, 1, canvas->drawingRectCount * sizeof(b32));
				canvas->proccessAsap = true;
				imageIsBroken = false;
				canvas->rollbackHasRolledBackOnce = true;
				canvas->saveRollbackOnNextPress = false;
				canvas->dataOnCanvas = false;
				//printf("UNDO! start: %d next: %d\n", canvas->rollbackIndexStart, canvas->rollbackIndexNext);
			}
			else if (canvas->rollbackStartHasProgressed)
			{
				String notification = STRING("Tragedy has struck! For you must now live with your mistakes. You've run out of undos!");
				InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
			}
			else if (canvas->rollbackHasRolledBackOnce)
			{
				String notification = STRING("You cannot go before the begining of time, my friend. (Nothing else to undo)");
				InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
			}
			else
			{
				String notification = STRING("You must go forward before you can go backwards. (No undo history yet! Draw something!!!!)");
				InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
			}
		}

		if (IsKeyPressed(KEY_ZERO))
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_NONE;
		if (IsKeyPressed(KEY_ONE))
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_SUB;
		if (IsKeyPressed(KEY_TWO))
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_UP;
		if (IsKeyPressed(KEY_THREE))
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_AVERAGE;
		if (IsKeyPressed(KEY_FOUR))
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_PAETH;
		if (IsKeyPressed(KEY_FIVE))
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_OPTIMAL;

		if (canvas->initialized && canvas->imagePNGFiltered.pngFilterType != canvas->currentPNGFilterType)
		{
			SetPNGFilterType(canvas, rootImageRaw, &gameMemory);
			canvas->proccessAsap = true;
		}

		if (canvas->initialized && canvas->proccessAsap)
		{
			ProcessedImage *processedImage = GetFreeProcessedImage(processedImages, threadCount);
			if (processedImage)
			{
				ArenaPair arenaPair = ArenaPairAssign(&gameMemory.conversionArenaGroup);
				if (arenaPair.arena1 && arenaPair.arena2)
				{
					processedImage->arenaPair = arenaPair;
					memcpy(processedImage->dirtyRectsInProcess, canvas->drawingRectDirtyListProcess, canvas->drawingRectCount * sizeof(b32));
					canvas->proccessAsap = false;
					processedImage->active = true;
					processedImage->processBatchIndex = canvas->processBatchIndex;
					canvas->processBatchIndex++;
					//NOTE: (Ahmayk) skip 0 as this represents already processed 
					if (canvas->processBatchIndex == 0)
					{
						canvas->processBatchIndex++;
					}
					memset(canvas->drawingRectDirtyListProcess, 0, canvas->drawingRectCount * sizeof(b32));
					PlatformAddThreadWorkEntry(threadWorkQueue, ProcessImageOnThread, (void *)processedImage);
					// Print("Queueing Work on thread " + IntToString(processedImage->index));
				}
			}
		}

		ProcessedImage *latestCompletedProcessedImage = {};
		for (u32 threadIndex = 0; threadIndex < threadCount; threadIndex++)
		{
			ProcessedImage *processedImageOfIndex = processedImages + threadIndex;
			if (processedImageOfIndex->active && processedImageOfIndex->processingComplete)
			{
				for (u32 rectIndex = 0; rectIndex < canvas->drawingRectCount; rectIndex++)
				{
					if (processedImageOfIndex->dirtyRectsInProcess[rectIndex])
					{
						canvas->drawingRectDirtyListFrame[rectIndex] = true;
						RectIV2 drawingRect = GetDrawingRectFromIndex(canvas->drawnImageData.dim, canvas->drawingRectDim, rectIndex);
						u32 pixelIndex = 0;
						u32 startY = drawingRect.pos.y;
						u32 endY = startY + drawingRect.dim.y;
						for (u32 y = startY; y < endY; y++)
						{
							u32 startIndex = (canvas->drawnImageData.dim.x * y) + drawingRect.pos.x;
							u32 endIndex = startIndex + drawingRect.dim.x;
							for (u32 i = startIndex; i < endIndex; i++)
							{
								Color *canvasPixel = &((Color *)canvas->drawnImageData.dataU8)[i];
								if (canvasPixel->a == processedImageOfIndex->processBatchIndex)
								{
									canvasPixel->a = 0;
								}
								pixelIndex++;
							}
						}
					}
				}

				if (latestCompletedProcessedImage && (latestCompletedProcessedImage->frameStarted > processedImageOfIndex->frameStarted))
				{
					// Print("Throwing away image from thread " + IntToString(latestCompletedProcessedImage->index));
					ResetProcessedImage(processedImageOfIndex, canvas);
				}
				else
				{
					latestCompletedProcessedImage = processedImageOfIndex;
				}
			}
		}

		b32 uploaded = false;
		if (latestCompletedProcessedImage)
		{
			if (latestCompletedProcessedImage->finalProcessedImageRaw.dataU8)
			{
				//UploadAndReplaceTexture(&latestCompletedProcessedImage->finalProcessedImageRaw, &loadedTexture);
				// Print("Uploading New Image from thread " + IntToString(latestCompletedProcessedImage->index));
				//RectIV2 drawingRect = {};
				//drawingRect.dim = latestCompletedProcessedImage->finalProcessedImageRaw.dim;
				//UpdateRectInTexture(&loadedTexture, latestCompletedProcessedImage->finalProcessedImageRaw.dataU8, drawingRect);

#if 1

				iv2 finalImageDim = latestCompletedProcessedImage->finalProcessedImageRaw.dim;
				ArenaMarker marker = {};
				b32 *finalImageDirtyRects = ARENA_PUSH_ARRAY_MARKER(&gameMemory.temporaryArena, canvas->finalImageRectCount, b32, &marker);
				memset(finalImageDirtyRects, 1, canvas->finalImageRectCount * sizeof(b32));

				b32 atLeastOneDirtyRect = false;
				for (u32 rectIndex = 0; rectIndex < canvas->finalImageRectCount; rectIndex++)
				{
					if (latestCompletedProcessedImage->finalImageRectHashes[rectIndex] != canvas->cachedFinalImageRectHashes[rectIndex])
					{
						finalImageDirtyRects[rectIndex] = true;
						atLeastOneDirtyRect = true;
						break;
					}
				}

				if (atLeastOneDirtyRect)
				{
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, canvas->finalImagePboIDs[canvas->currentFinalImagePboID]);
					u8 *pixels = (u8 *) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
					if (ASSERT(pixels))
					{
						for (u32 rectIndex = 0; rectIndex < canvas->finalImageRectCount; rectIndex++)
						{
							if (finalImageDirtyRects[rectIndex])
							{
								RectIV2 drawingRect = GetDrawingRectFromIndex(finalImageDim, canvas->finalImageRectDim, rectIndex);
								u32 startY = drawingRect.pos.y;
								u32 endY = startY + drawingRect.dim.y;
								for (u32 y = startY; y < endY; y++)
								{
									u32 index = ((finalImageDim.x * y) + drawingRect.pos.x) * 4;
									u8 *dst = pixels + index;
									u8 *src = latestCompletedProcessedImage->finalProcessedImageRaw.dataU8 + index;
									memcpy(dst, src, drawingRect.dim.x * 4);
								}
							}
						}
						glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
					}

					glBindTexture(GL_TEXTURE_2D, loadedTexture.id);
					glPixelStorei(GL_UNPACK_ROW_LENGTH, loadedTexture.width);
					for (u32 rectIndex = 0; rectIndex < canvas->finalImageRectCount; rectIndex++)
					{
						if (finalImageDirtyRects[rectIndex])
						{
							RectIV2 drawingRect = GetDrawingRectFromIndex(finalImageDim, canvas->finalImageRectDim, rectIndex);
							GLintptr offset = (drawingRect.pos.y * finalImageDim.x + drawingRect.pos.x) * sizeof(Color);
							glTexSubImage2D(GL_TEXTURE_2D, 0, drawingRect.pos.x, drawingRect.pos.y, drawingRect.dim.x, drawingRect.dim.y, GL_RGBA, GL_UNSIGNED_BYTE, (void*)offset);
						}
					}

					ArenaPopMarker(marker);

					glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
					canvas->currentFinalImagePboID = ModNextU32(canvas->currentFinalImagePboID, ARRAY_COUNT(canvas->finalImagePboIDs));

					uploaded = true;

					memcpy(canvas->cachedFinalImageRectHashes, latestCompletedProcessedImage->finalImageRectHashes, canvas->finalImageRectCount * sizeof(u32));
				}
#endif
			}
			else
			{
				imageIsBroken = true;
			}
			ResetProcessedImage(latestCompletedProcessedImage, canvas);
		}

		b32 somethingToDraw = false;
		for (u32 rectIndex = 0; rectIndex < canvas->drawingRectCount; rectIndex++)
		{
			if (canvas->drawingRectDirtyListFrame[rectIndex])
			{
				somethingToDraw = true;
			}
		}

		if (somethingToDraw)
		{
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, canvas->drawingPboIDs[canvas->currentDrawingPboID]);
			Color *pixels = (Color *) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
			if (ASSERT(pixels))
			{
				for (u32 rectIndex = 0; rectIndex < canvas->drawingRectCount; rectIndex++)
				{
					if (canvas->drawingRectDirtyListFrame[rectIndex])
					{
						RectIV2 drawingRect = GetDrawingRectFromIndex(canvas->drawnImageData.dim, canvas->drawingRectDim, rectIndex);
						u32 startY = drawingRect.pos.y;
						u32 endY = startY + drawingRect.dim.y;
						for (u32 y = startY; y < endY; y++)
						{
							u32 startIndex = (canvas->drawnImageData.dim.x * y) + drawingRect.pos.x;
							u32 endIndex = startIndex + drawingRect.dim.x;
							for (u32 i = startIndex; i < endIndex; i++)
							{
								Color canvasPixel = ((Color *)canvas->drawnImageData.dataU8)[i];
								Color *outPixel = (pixels + i);
								//NOTE: (Ahmayk) alpha = 0 -> no processing
								//alpha != 0 -> is being processed currently
								if (canvasPixel.a != 0)
								{
									*outPixel = BRUSH_EFFECT_COLORS_PROCESSING[canvasPixel.r];
								}
								else
								{
									*outPixel = BRUSH_EFFECT_COLORS_PRIMARY[canvasPixel.r];
								}
							}
						}
					}
				}
				glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
			}

			glBindTexture(GL_TEXTURE_2D, canvas->textureDrawing.id);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, canvas->textureDrawing.width);

			for (u32 rectIndex = 0; rectIndex < canvas->drawingRectCount; rectIndex++)
			{
				if (canvas->drawingRectDirtyListFrame[rectIndex])
				{
					RectIV2 drawingRect = GetDrawingRectFromIndex(canvas->drawnImageData.dim, canvas->drawingRectDim, rectIndex);
					GLintptr offset = (drawingRect.pos.y * canvas->textureDrawing.width + drawingRect.pos.x) * sizeof(Color);
					glTexSubImage2D(GL_TEXTURE_2D, 0, drawingRect.pos.x, drawingRect.pos.y, drawingRect.dim.x, drawingRect.dim.y, GL_RGBA, GL_UNSIGNED_BYTE, (void*)offset);
					canvas->drawingRectDirtyListFrame[rectIndex] = false;
				}
			}

			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			canvas->currentDrawingPboID = ModNextU32(canvas->currentDrawingPboID, ARRAY_COUNT(canvas->drawingPboIDs));

			GenTextureMipmaps(&canvas->textureDrawing);
		}

		//----------------------------------------------------
		//---------------------RENDER-------------------------
		//----------------------------------------------------

		UiBuffer *uiBufferCurrent = &G_UI_STATE->uiBuffers[G_UI_STATE->uiBufferIndex];
		// NOTE: We start at 1 so that we always have a null uiBlock
		uiBufferCurrent->uiBlockCount = 1;
		G_UI_STATE->uiSettings = {};
		G_UI_STATE->parentStackCount = {};
		UiSettings *uiSettings = &G_UI_STATE->uiSettings;
		G_UI_STATE->commandInputs = commandInputs;

		UiFont defaultUiFont = {};
		defaultUiFont.id = defaultFont.texture.id;
		defaultUiFont.data = &defaultFont;

		float titleBarHeight = 20;

		UiBlock *root = CreateUiBlock(G_UI_STATE, uiSettings);
		root->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PIXELS, (f32) windowDim.x};
		root->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, (f32) windowDim.y};
		UiParent()
		{
			uiSettings->frontColor = BLACK;
			uiSettings->borderColor = DARKGRAY;
			uiSettings->backColor = Color{191, 191, 191, 255};

			UiBlock *titleBar = CreateUiBlock(G_UI_STATE, uiSettings);
			titleBar->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT;
			titleBar->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PERCENT_OF_PARENT, (f32) windowDim.x};
			titleBar->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, titleBarHeight};

			UiBlock *body = CreateUiBlock(G_UI_STATE, uiSettings);
			body->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PIXELS, (f32) windowDim.x};
			body->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, windowDim.y - titleBarHeight};
			UiParent()
			{
				UiBlock *topPart = CreateUiBlock(G_UI_STATE, uiSettings);
				topPart->flags = UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER;
				topPart->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
				topPart->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 0.5f};
				UiParent()
				{
					if (loadedTexture.id)
					{
						if (!imageIsBroken)
						{
							UiBlock *finalTexture = CreateUiBlock(G_UI_STATE, uiSettings);
							finalTexture->flags = UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT | UI_FLAG_INTERACTABLE;
							finalTexture->hash = HASH_FINAL_TEXTURE;
							finalTexture->uiTexture = UiRaylibTextureToUiTexture(&loadedTexture);
							finalTexture->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT};
							finalTexture->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT};
						}
						else
						{
							UiBlock *stringBlock = CreateUiBlock(G_UI_STATE, uiSettings);
							stringBlock->flags = UI_FLAG_DRAW_TEXT | UI_FLAG_CENTER_IN_PARENT;
							stringBlock->string = STRING("Congulations! You broke the image. (undo with Ctrl-Z)");
							stringBlock->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_TEXT};
							stringBlock->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_TEXT};
							stringBlock->uiFont = defaultUiFont;
						}
					}
					else
					{
						UiBlock *stringBlock = CreateUiBlock(G_UI_STATE, uiSettings);
						stringBlock->flags = UI_FLAG_DRAW_TEXT | UI_FLAG_CENTER_IN_PARENT;
						stringBlock->string = STRING("Drop any image into the window for editing.");
						stringBlock->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_TEXT};
						stringBlock->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_TEXT};
						stringBlock->uiFont = defaultUiFont;
					}
				}

				UiBlock *bottomPart = CreateUiBlock(G_UI_STATE, uiSettings);
				bottomPart->flags = UI_FLAG_DRAW_BORDER;
				bottomPart->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
				bottomPart->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 0.5f};
				UiParent()
				{
					if (canvas->textureVisualizedFilteredRootImage.id)
					{
						UiBlock *b = CreateUiBlock(G_UI_STATE, uiSettings);
						b->flags = UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT;
						b->uiTexture = UiRaylibTextureToUiTexture(&canvas->textureVisualizedFilteredRootImage);
						b->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT};
						b->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT};
					}
					if (canvas->textureDrawing.id)
					{
						UiBlock *b = CreateUiBlock(G_UI_STATE, uiSettings);
						b->flags = UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT | UI_FLAG_INTERACTABLE;
						b->hash = HASH_CANVAS;
						b->uiTexture = UiRaylibTextureToUiTexture(&canvas->textureDrawing);
						b->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT};
						b->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT};
					}
				}
			}
		}

		float toolbarWidth = G_TOOLBOX_WIDTH_AND_HEIGHT * 2;
		uiSettings->backColor = Color{191, 191, 191, 255};

		UiBlock *sideToolbar = CreateUiBlock(G_UI_STATE, uiSettings);
		sideToolbar->flags = UI_FLAG_DRAW_BACKGROUND;
		sideToolbar->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PIXELS, toolbarWidth};
		sideToolbar->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS}; //??
		sideToolbar->relativePixelPosition = v2{0, (float)titleBarHeight};

		UiParent()
		{
			ReactiveUiColorState uiColorState = {};

			uiSettings->frontColor = BLACK;
			uiSettings->borderColor = GRAY;
			{
				UiBlock *b = CreateUiBlock(G_UI_STATE, uiSettings);
				b->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
				b->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, 10};
			}

			UiBlock *h;
			h = CreateUiBlock(G_UI_STATE, uiSettings);
			h->flags = UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT;
			h->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
			h->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
			UiParent()
			{
				CreateBrushEffectButton(BRUSH_EFFECT_ERASE, STRING("Ers"), defaultUiFont, Color{245, 245, 245, 255}, COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE, &currentBrush);
				CreateBrushEffectButton(BRUSH_EFFECT_REMOVE, STRING("Rmv"), defaultUiFont, BRUSH_EFFECT_COLORS_PRIMARY[BRUSH_EFFECT_REMOVE], COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE, &currentBrush);
			}

			h = CreateUiBlock(G_UI_STATE, uiSettings);
			h->flags = UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT;
			h->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
			h->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
			UiParent()
			{
				CreateBrushEffectButton(BRUSH_EFFECT_MAX, STRING("Max"), defaultUiFont, BRUSH_EFFECT_COLORS_PRIMARY[BRUSH_EFFECT_MAX], COMMAND_SWITCH_BRUSH_EFFECT_TO_MAX, &currentBrush);
				CreateBrushEffectButton(BRUSH_EFFECT_SHIFT, STRING("Sft"), defaultUiFont, BRUSH_EFFECT_COLORS_PRIMARY[BRUSH_EFFECT_SHIFT], COMMAND_SWITCH_BRUSH_EFFECT_TO_SHIFT, &currentBrush);
			}

			h = CreateUiBlock(G_UI_STATE, uiSettings);
			h->flags = UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT;
			h->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
			h->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT};
			UiParent()
			{
				CreateBrushEffectButton(BRUSH_EFFECT_RANDOM, STRING("Rnd"), defaultUiFont, BRUSH_EFFECT_COLORS_PRIMARY[BRUSH_EFFECT_RANDOM], COMMAND_SWITCH_BRUSH_EFFECT_TO_RANDOM, &currentBrush);
			}

			{
				UiBlock *b = CreateUiBlock(G_UI_STATE, uiSettings);
				b->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
				b->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, 10};
			}

			uiSettings->backColor = Color{191, 191, 191, 255};
			UiBlock *sliderNumLabel = CreateUiBlock(G_UI_STATE, uiSettings);
			sliderNumLabel->flags = UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_CENTERED;
			sliderNumLabel->string = U32ToString(currentBrush.size, StringArena());
			sliderNumLabel->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
			sliderNumLabel->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_TEXT};
			sliderNumLabel->uiFont = defaultUiFont;

			ReactiveUiColor uiColorParent = {};
			uiColorParent.down = Color{220, 220, 220, 255};
			uiColorParent.hovered = Color{200, 200, 200, 255};
			uiColorParent.neutral = Color{180, 180, 180, 255};
			u32 hashBrushSizeSlider = Murmur3String("brushSizeSlider");
			UiBlock *sliderUiBlock = GetUiBlockOfHashLastFrame(hashBrushSizeSlider);
			uiSettings->backColor = GetReactiveColor(commandInputs, sliderUiBlock, uiColorParent, false);

			Slider slider = brushSizeSlider;
			float value = MapNormalizeF32(brushSizeSlider.min, (f32) *brushSizeSlider.unsignedIntToChange, brushSizeSlider.max);

			UiBlock *sliderBase = CreateUiBlock(G_UI_STATE, uiSettings);
			sliderBase->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_INTERACTABLE | UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER;
			sliderBase->hash = hashBrushSizeSlider;
			sliderBase->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
			sliderBase->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT * 0.5f};
			sliderBase->value = value;
			sliderBase->sliderAction = SLIDER_ACTION_BRUSH_SIZE;
			UiParent()
			{
				ReactiveUiColor uiColorChild = {};
				uiColorChild.down = Color{20, 131, 255, 255};
				uiColorChild.hovered = Color{10, 131, 251, 255};
				uiColorChild.neutral = Color{0, 121, 241, 255};
				uiSettings->backColor = GetReactiveColor(commandInputs, sliderUiBlock, uiColorChild, false);

				UiBlock *sliderTop = CreateUiBlock(G_UI_STATE, uiSettings);
				sliderTop->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER;
				sliderTop->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PERCENT_OF_PARENT, value};
				sliderTop->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
			}

			uiSettings->backColor = BRUSH_EFFECT_COLORS_PRIMARY[currentBrush.brushEffect];
			if (currentBrush.brushEffect == BRUSH_EFFECT_ERASE)
			{
				uiSettings->backColor = Color{245, 245, 245, 255};
			}
			uiSettings->borderColor = BLACK;
			UiBlock *brushView = CreateUiBlock(G_UI_STATE, uiSettings);
			brushView->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER;
			brushView->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PIXELS, toolbarWidth};
			brushView->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_PIXELS, toolbarWidth};
		}

		NotificationMessage *notificationMessage = GetNotificationMessage();
		if (notificationMessage->string.length)
		{
			uiSettings->frontColor = Color{255, 255, 255, (unsigned char)(notificationMessage->alpha * 255)};
			uiSettings->backColor = Color{100, 100, 100, (unsigned char)(notificationMessage->alpha * 255)};
			UiBlock *notifBlock = CreateUiBlock(G_UI_STATE, uiSettings);
			notifBlock->flags = UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT;
			notifBlock->string = notificationMessage->string;
			notifBlock->relativePixelPosition = v2{0, titleBarHeight};
			notifBlock->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PIXELS, (f32) windowDim.x};
			notifBlock->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_TEXT};
			notifBlock->uiFont = defaultUiFont;

			notificationMessage->alpha -= 0.001f;
			if (notificationMessage->alpha <= 0)
			{
				notificationMessage->alpha = {};
			}
		}

		uiSettings->frontColor = BLACK;
		u32 fps = GetFPS();
		String fpsString = STRING("FPS: ") + U32ToString(fps, StringArena());
		if (fps < 60)
		{
			uiSettings->frontColor = RED;
		}
		UiBlock *blockFps = CreateUiBlock(G_UI_STATE, uiSettings);
		blockFps->flags = UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT;
		blockFps->string = fpsString;
		blockFps->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_TEXT};
		blockFps->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_TEXT};
		blockFps->uiFont = defaultUiFont;

		{
			uiSettings->frontColor = DARKGRAY;
			UiBlock *b = CreateUiBlock(G_UI_STATE, uiSettings);
			b->flags = UI_FLAG_DRAW_TEXT;
			b->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_TEXT};
			b->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_TEXT};
			b->relativePixelPosition = v2{windowDim.x * 0.07f, 2};
			b->string = STRING("Use 0-5 to toggle differnet PNG filter algorythms");
			b->uiFont = defaultUiFont;
		}

		{
			uiSettings->frontColor = BLACK;
			UiBlock *b = CreateUiBlock(G_UI_STATE, uiSettings);
			b->flags = UI_FLAG_DRAW_TEXT;
			b->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_TEXT};
			b->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_TEXT};
			b->relativePixelPosition = v2{windowDim.x * 0.45f, 2};
			b->string = STRING("PNG Filter: ") + PNG_FILTER_NAMES[canvas->currentPNGFilterType];
			b->uiFont = defaultUiFont;
		}

		{
			ReactiveUiColorState uiColorState = {};
			uiColorState.nonActive.down = DARKGREEN;
			uiColorState.nonActive.hovered = Color{10, 238, 58, 255};
			uiColorState.nonActive.neutral = GREEN;
			UiBlock *b = CreateUiButton(STRING("EXPORT IMAGE"), Murmur3String("exportImage"), defaultUiFont, uiColorState, false, false);
			b->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PIXELS, 200};
			b->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_TEXT};
			b->relativePixelPosition = v2{windowDim.x * 0.8f, 2};
			b->command = COMMAND_EXPORT_IMAGE;
		}

		{
			uiSettings->frontColor = DARKGRAY;
			UiBlock *b = CreateUiBlock(G_UI_STATE, uiSettings);
			b->flags = UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT;
			b->uiSizes[UI_AXIS_X] = {UI_SIZE_KIND_PIXELS, (f32) windowDim.x};
			b->uiSizes[UI_AXIS_Y] = {UI_SIZE_KIND_TEXT};
			b->relativePixelPosition = v2{-5, (f32) windowDim.y - 20};
			b->string = STRING(VERSION_NUMBER);
			b->uiFont = defaultUiFont;
		}

		UiRaylibProcessStrings(uiBufferCurrent);
		UiLayoutBlocks(uiBufferCurrent);

		BeginDrawing();

		ClearBackground(Color{127, 127, 127, 255});

		UiRenderBlocksRaylib(uiBufferCurrent);

		if (isHoveredOnPaintable)
		{
			v2 normalizedRelativePos = cursorPosInDrawnImage / canvas->drawnImageData.dim;
			if (canvasUiBlock)
			{
				v2 hoverPos = canvasUiBlock->rect.pos + (canvasUiBlock->rect.dim * normalizedRelativePos);
				Color hoverCursorColor = BRUSH_EFFECT_COLORS_PROCESSING[currentBrush.brushEffect];
				f32 size = currentBrush.size * SafeDivideF32(canvasUiBlock->rect.dim.x, (f32) canvas->drawnImageData.dim.x);
				DrawCircle((i32)hoverPos.x, (i32)hoverPos.y, size, hoverCursorColor);
			}
			if (finalTextureUiBlock)
			{
				v2 hoverPos = finalTextureUiBlock->rect.pos + (finalTextureUiBlock->rect.dim * normalizedRelativePos);
				Color outlineColor = Color{0, 0, 0, 100};
				f32 size = currentBrush.size * SafeDivideF32(finalTextureUiBlock->rect.dim.x, (f32) canvas->drawnImageData.dim.x);
				DrawCircleLines((i32)hoverPos.x, (i32)hoverPos.y, size, outlineColor);
			}
		}

		EndDrawing();

		ArenaReset(&gameMemory.temporaryArena);

		UiEndFrame();

#if 0
		f64 timeEnd = GetTime();
		f64 totalMs = (timeEnd - timeStart) * 1000;
		if (uploaded)
		{
			printf("uploaded: %d totalFrameTime: %f\n", uploaded, totalMs);
		}
#endif
	}

	RayCloseWindow();
}

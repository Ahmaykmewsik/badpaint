
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

	UiState *G_UI_STATE = GetUiState();
	UiInputs *G_UI_INPUTS = GetUiInputs();

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
	//Font largeFont = LoadFontFromMemory(".otf", PAINT_FONT_DATA, ARRAY_COUNT(PAINT_FONT_DATA), 32, 0, 0);

#if 0
	u32 dataSize;
    unsigned char *fileData = LoadFileData("./assets/defaultImage.png", &dataSize);

    // NOTE: Text data buffer size is estimated considering image data size in bytes
    // and requiring 6 char bytes for every byte: "0x00, "
    char *txtData = (char *)RL_CALLOC(dataSize*6 + 2000, sizeof(char));

    int byteCount = 0;
    // Get file name from path and convert variable name to uppercase
    char *varFileName = "foo";
    byteCount += sprintf(txtData + byteCount, "static unsigned char %s_DATA[%i] = { ", varFileName, dataSize);
    for (u32 i = 0; i < dataSize - 1; i++) byteCount += sprintf(txtData + byteCount, ((i%20 == 0)? "0x%x,\n" : "0x%x, "), ((unsigned char *)fileData)[i]);
    byteCount += sprintf(txtData + byteCount, "0x%x };\n", ((unsigned char *)fileData)[dataSize - 1]);
    SaveFileText("defaultImage.h", txtData);
    RL_FREE(txtData);
#endif

	Color deleteColor = RED;

	Brush currentBrush = {};
	currentBrush.brushEffect = BRUSH_EFFECT_REMOVE;
	currentBrush.size = 10;

	Texture loadedTexture = {};
	v2 pressedMousePos = {};
	String draggedUiStringKey = {};

	Slider brushSizeSlider = {};
	brushSizeSlider.sliderAction = SLIDER_ACTION_BRUSH_SIZE;
	brushSizeSlider.unsignedIntToChange = &currentBrush.size;
	brushSizeSlider.min = 1;
	brushSizeSlider.max = 50;

#if 0
	iv2 imageDefaultDim = {680, 384}; 
	gameMemory.rootImageArena = ArenaInit(imageDefaultDim.x * imageDefaultDim.y * 4);
	if (gameMemory.rootImageArena.memory)
	{
		rootImageRaw->dim = imageDefaultDim;
		rootImageRaw->dataSize = imageDefaultDim.x * imageDefaultDim.y * 4;
		rootImageRaw->dataU8 = (u8*) ArenaPushSize(&gameMemory.rootImageArena, rootImageRaw->dataSize, {});

		Font font = defaultFont; 
		Color topColor = Color{0, 115, 198, 255};
		Color bottomColor = Color{30, 139, 217, 255};
		//Image imageDefault = GenImageChecked(imageDefaultDim.x, imageDefaultDim.y, 8, 8, WHITE, LIGHTGRAY);
		Image imageDefault = GenImageGradientV(imageDefaultDim.x, imageDefaultDim.y, topColor, bottomColor);
		//Image imageDefault = GenImagePerlinNoise(imageDefaultDim.x, imageDefaultDim.y, 69, 69, 8);
		//Image imageDefault = GenImageColor(imageDefaultDim.x, imageDefaultDim.y, );
		//ImageColorTint(&imageDefault, Color{1, 27, 208, 255});
		//ImageColorBrightness(&imageDefault, 40);
		const char *defaultText = "Drop any image into this window to paint it!";
		Vector2 textRayVector2 = MeasureTextEx(font, defaultText, (f32) font.baseSize, 1);
		iv2 textPos;
		textPos.x = (i32) RoundF32((imageDefaultDim.x * 0.5f) - (textRayVector2.x * 0.5f));
		textPos.y = (i32) RoundF32((imageDefaultDim.y * 0.5f) - (textRayVector2.y * 0.5f));
		Vector2 textPosRayVector = {(f32)textPos.x, (f32)textPos.y};
		iv2 margins = {80, 8};
		RectIV2 bgRect;
		bgRect.pos = textPos - margins; 
		bgRect.dim.x = (i32) textRayVector2.x + (margins.x * 2);
		bgRect.dim.y = (i32) textRayVector2.y + (margins.y * 2);
		ImageDrawRectangle(&imageDefault, bgRect.pos.x, bgRect.pos.y, bgRect.dim.x, bgRect.dim.y, WHITE);
		ImageDrawTextEx(&imageDefault, font, defaultText, textPosRayVector, (f32) font.baseSize, 1.0f, BLACK);
		memcpy(rootImageRaw->dataU8, imageDefault.data, rootImageRaw->dataSize);
		UnloadImage(imageDefault);
		InitializeNewImage(&gameMemory, rootImageRaw, canvas, &loadedTexture, &currentBrush, processedImages, threadCount);
	}
#endif

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

		G_UI_STATE->twoFrameArenaLastFrame = GetTwoFrameArenaLastFrame(&gameMemory);
		G_UI_STATE->twoFrameArenaThisFrame = GetTwoFrameArenaThisFrame(&gameMemory);
		int uiBlockArrayIndex = GetFrameModIndexLastFrame();
		windowDim.x = GetScreenWidth();
		windowDim.y = GetScreenHeight();

		v2 mousePixelPos = v2{(float)GetMouseX(), (float)GetMouseY()};
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
		{
			ArenaReset(&gameMemory.mouseClickArena);
			draggedUiStringKey = {};

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

		for (u32 i = 0; i < G_UI_STATE->uiBlockCount; i++)
		{
			UiBlock *uiBlock = &G_UI_STATE->uiBlockes[uiBlockArrayIndex][i];
			if (IsFlag(uiBlock, UI_FLAG_INTERACTABLE))
			{
				uiBlock->cursorRelativePixelPos = mousePixelPos - uiBlock->rect.pos;

				if (IsInRectV2(mousePixelPos, uiBlock->rect))
				{
					uiBlock->hovered = true;
					uiBlock->pressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
					uiBlock->down = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
					if (uiBlock->pressed && uiBlock->keyString.length)
					{
						draggedUiStringKey = ReallocString(uiBlock->keyString, &gameMemory.mouseClickArena);
					}
				}

				if (uiBlock->uiInputs.sliderAction && draggedUiStringKey == uiBlock->keyString)
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
		for (u32 i = 0; i < G_UI_STATE->uiBlockCount; i++)
		{
			UiBlock *uiBlock = &G_UI_STATE->uiBlockes[uiBlockArrayIndex][i];
			if (IsFlag(uiBlock, UI_FLAG_INTERACTABLE))
			{
				if (uiBlock->uiInputs.command)
				{
					CommandInput *commandInput = commandInputs + uiBlock->uiInputs.command;
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

		UiBlock *canvasUiBlock = GetUiBlockLastFrameOfStringKey(G_CANVAS_STRING_TAG_CHARS);
		UiBlock *finalTextureUiBlock = GetUiBlockLastFrameOfStringKey(STRING("finalTexture"));

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
					processedImage->frameStarted = G_CURRENT_FRAME;
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

		// NOTE: We start at 1 so that we always have a null uiBlock
		G_UI_STATE->uiBlockCount = 1;
		G_UI_STATE->uiSettings = {};
		G_UI_STATE->parentStackCount = {};
		UiSettings *uiSettings = &G_UI_STATE->uiSettings;
		G_UI_STATE->commandInputs = commandInputs;

		uiSettings->font = defaultFont;

		float titleBarHeight = 20;

		SetUiAxis({UI_SIZE_KIND_PIXELS, (f32) windowDim.x}, {UI_SIZE_KIND_PIXELS, (f32) windowDim.y});
		CreateUiBlock();
		UiParent()
		{
			uiSettings->frontColor = BLACK;
			uiSettings->borderColor = DARKGRAY;
			uiSettings->backColor = Color{191, 191, 191, 255};
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, titleBarHeight});
			CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				//TODO: Menu
			}

			SetUiAxis({UI_SIZE_KIND_PIXELS, (f32) windowDim.x}, {UI_SIZE_KIND_PIXELS, windowDim.y - titleBarHeight});
			CreateUiBlock();
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 0.5});
				CreateUiBlock(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER);
				UiParent()
				{
					if (loadedTexture.id)
					{
						if (!imageIsBroken)
						{
							G_UI_INPUTS->texture = loadedTexture;
							SetUiAxis({UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT}, {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT});
							CreateUiBlock(UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT | UI_FLAG_INTERACTABLE, STRING(G_UI_HASH_TAG) + STRING("finalTexture"));
						}
						else
						{
							String string = STRING("Congulations! You broke the image. (undo with Ctrl-Z)");
							SetUiAxis({UI_SIZE_KIND_TEXT, 1}, {UI_SIZE_KIND_TEXT, 1});
							CreateUiBlock(UI_FLAG_DRAW_TEXT | UI_FLAG_CENTER_IN_PARENT, string);
						}
					}
					else
					{
						String string = STRING("Drop any image into the window for editing.");
						SetUiAxis({UI_SIZE_KIND_TEXT, 1}, {UI_SIZE_KIND_TEXT, 1});
						CreateUiBlock(UI_FLAG_DRAW_TEXT | UI_FLAG_CENTER_IN_PARENT, string);
					}
				}
				SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 0.5});
				CreateUiBlock(UI_FLAG_DRAW_BORDER);
				UiParent()
				{
					if (canvas->textureVisualizedFilteredRootImage.id)
					{
						G_UI_INPUTS->texture = canvas->textureVisualizedFilteredRootImage;
						SetUiAxis({UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT}, {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT});
						CreateUiBlock(UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT);
					}
					if (canvas->textureDrawing.id)
					{
						G_UI_INPUTS->texture = canvas->textureDrawing;
						SetUiAxis({UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT}, {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT});
						CreateUiBlock(UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT | UI_FLAG_INTERACTABLE, STRING(G_UI_HASH_TAG) + G_CANVAS_STRING_TAG_CHARS);
					}
				}
			}
		}

		float toolbarWidth = G_TOOLBOX_WIDTH_AND_HEIGHT * 2;

		SetUiAxis({UI_SIZE_KIND_PIXELS, toolbarWidth}, {UI_SIZE_KIND_PIXELS});
		G_UI_INPUTS->relativePixelPosition = v2{0, (float)titleBarHeight};
		uiSettings->backColor = Color{191, 191, 191, 255};
		CreateUiBlock(UI_FLAG_DRAW_BACKGROUND);
		UiParent()
		{
			ReactiveUiColorState uiColorState = {};

			uiSettings->frontColor = BLACK;
			uiSettings->borderColor = GRAY;

			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBlock(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
				CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
				CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBlock(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
				CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
				CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBlock(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
				CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
				CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBlock(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
				CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
				CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBlock(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
				CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
				CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
			}

			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, 10});
			CreateUiBlock();

			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBlock(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				CreateBrushEffectButton(BRUSH_EFFECT_ERASE, STRING("Ers"), Color{245, 245, 245, 255}, COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE, &currentBrush);
				CreateBrushEffectButton(BRUSH_EFFECT_REMOVE, STRING("Rmv"), BRUSH_EFFECT_COLORS_PRIMARY[BRUSH_EFFECT_REMOVE], COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE, &currentBrush);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBlock(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				CreateBrushEffectButton(BRUSH_EFFECT_MAX, STRING("Max"), BRUSH_EFFECT_COLORS_PRIMARY[BRUSH_EFFECT_MAX], COMMAND_SWITCH_BRUSH_EFFECT_TO_MAX, &currentBrush);
				CreateBrushEffectButton(BRUSH_EFFECT_SHIFT, STRING("Sft"), BRUSH_EFFECT_COLORS_PRIMARY[BRUSH_EFFECT_SHIFT], COMMAND_SWITCH_BRUSH_EFFECT_TO_SHIFT, &currentBrush);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBlock(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				CreateBrushEffectButton(BRUSH_EFFECT_RANDOM, STRING("Rnd"), BRUSH_EFFECT_COLORS_PRIMARY[BRUSH_EFFECT_RANDOM], COMMAND_SWITCH_BRUSH_EFFECT_TO_RANDOM, &currentBrush);
			}

			// SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			// CreateUiBlock(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			// UiParent()
			// {
			// }

			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, 10});
			CreateUiBlock();

			uiSettings->backColor = Color{191, 191, 191, 255};
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_TEXT});
			CreateUiBlock(UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_CENTERED, U32ToString(currentBrush.size, StringArena()));

			ReactiveUiColor uiColorParent = {};
			uiColorParent.down = Color{220, 220, 220, 255};
			uiColorParent.hovered = Color{200, 200, 200, 255};
			uiColorParent.neutral = Color{180, 180, 180, 255};
			String stringKey = STRING("brushSizeSlider");
			UiBlock *sliderUiBlock = GetUiBlockLastFrameOfStringKey(stringKey);
			uiSettings->backColor = GetReactiveColor(commandInputs, sliderUiBlock, uiColorParent, false);
			G_UI_INPUTS->sliderAction = SLIDER_ACTION_BRUSH_SIZE;

			Slider slider = brushSizeSlider;
			float value = MapNormalizeF32(brushSizeSlider.min, (f32) *brushSizeSlider.unsignedIntToChange, brushSizeSlider.max);
			G_UI_INPUTS->value = value;

			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT * 0.5f});
			CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_INTERACTABLE | UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER, G_UI_HASH_TAG + stringKey);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, value}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 1});
				ReactiveUiColor uiColorChild = {};
				uiColorChild.down = Color{20, 131, 255, 255};
				uiColorChild.hovered = Color{10, 131, 251, 255};
				uiColorChild.neutral = Color{0, 121, 241, 255};
				uiSettings->backColor = GetReactiveColor(commandInputs, sliderUiBlock, uiColorChild, false);
				CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER);
			}

			SetUiAxis({UI_SIZE_KIND_PIXELS, toolbarWidth}, {UI_SIZE_KIND_PIXELS, toolbarWidth});

			uiSettings->backColor = BRUSH_EFFECT_COLORS_PRIMARY[currentBrush.brushEffect];
			if (currentBrush.brushEffect == BRUSH_EFFECT_ERASE)
				uiSettings->backColor = Color{245, 245, 245, 255};
			uiSettings->borderColor = BLACK;
			CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
		}

		NotificationMessage *notificationMessage = GetNotificationMessage();
		if (notificationMessage->string.length)
		{
			SetUiAxis({UI_SIZE_KIND_PIXELS, (f32) windowDim.x}, {UI_SIZE_KIND_TEXT});
			uiSettings->frontColor = Color{255, 255, 255, (unsigned char)(notificationMessage->alpha * 255)};
			uiSettings->backColor = Color{100, 100, 100, (unsigned char)(notificationMessage->alpha * 255)};
			G_UI_INPUTS->relativePixelPosition = v2{0, titleBarHeight};
			CreateUiBlock(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT, notificationMessage->string);

			notificationMessage->alpha -= 0.001f;
			if (notificationMessage->alpha <= 0)
			{
				notificationMessage->alpha = {};
			}
		}

		SetUiAxis({UI_SIZE_KIND_TEXT}, {UI_SIZE_KIND_TEXT});
		uiSettings->frontColor = BLACK;
		u32 fps = GetFPS();
		String fpsString = STRING("FPS: ") + U32ToString(fps, StringArena());
		if (fps < 60)
			uiSettings->frontColor = RED;
		CreateUiBlock(UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT, fpsString);

		uiSettings->frontColor = DARKGRAY;
		G_UI_INPUTS->relativePixelPosition = v2{windowDim.x * 0.07f, 2};
		String label = STRING("Use 0-5 to toggle differnet PNG filter algorythms");
		CreateUiBlock(UI_FLAG_DRAW_TEXT, label);

		uiSettings->frontColor = BLACK;
		G_UI_INPUTS->relativePixelPosition = v2{windowDim.x * 0.45f, 2};
		label = STRING("PNG Filter: ") + PNG_FILTER_NAMES[canvas->currentPNGFilterType];
		CreateUiBlock(UI_FLAG_DRAW_TEXT, label);

		SetUiAxis({UI_SIZE_KIND_PIXELS, 200}, {UI_SIZE_KIND_TEXT});
		G_UI_INPUTS->relativePixelPosition = v2{windowDim.x * 0.8f, 2};
		label = STRING("EXPORT IMAGE") + G_UI_HASH_TAG + "export";
		G_UI_INPUTS->command = COMMAND_EXPORT_IMAGE;
		ReactiveUiColorState uiColorState = {};
		uiColorState.nonActive.down = DARKGREEN;
		uiColorState.nonActive.hovered = Color{10, 238, 58, 255};
		uiColorState.nonActive.neutral = GREEN;
		CreateUiButton(label, uiColorState, false, false);

		SetUiAxis({UI_SIZE_KIND_PIXELS, (f32) windowDim.x}, {UI_SIZE_KIND_TEXT});
		G_UI_INPUTS->relativePixelPosition = v2{-5, (f32) windowDim.y - 20};
		uiSettings->frontColor = DARKGRAY;
		label = STRING(VERSION_NUMBER);
		CreateUiBlock(UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT, label);

		UiLayoutBlocks();

		BeginDrawing();

		ClearBackground(Color{127, 127, 127, 255});

		UiRenderBlocksRaylib(G_UI_STATE);

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
		Arena *memoryArenaLastFrame = GetTwoFrameArenaLastFrame(&gameMemory);
		ArenaReset(memoryArenaLastFrame);

		G_CURRENT_FRAME++;

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

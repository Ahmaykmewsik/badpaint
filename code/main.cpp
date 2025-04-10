
#include "headers.h"

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

	G_UI_INPUTS = ARENA_PUSH_STRUCT(&gameMemory.permanentArena, UiInputs);
	G_UI_STATE = ARENA_PUSH_STRUCT(&gameMemory.permanentArena, UiState);

	SetTraceLogLevel(RL_LOG_NONE);

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

	Font defaultFont = LoadFontEx("./assets/W95FA.otf", 18, 0, 0);
	Font bigFont = LoadFontEx("./assets/W95FA.otf", 48, 0, 0);

	Color deleteColor = RED;

	Brush currentBrush = {};
	currentBrush.brushEffect = BRUSH_EFFECT_REMOVE;
	currentBrush.size = 3;

	Texture loadedTexture = {};

	stbi_write_force_png_filter = 5;
	int pngFilterLastFrame = stbi_write_force_png_filter;

	G_COMMAND_STATES[COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE].key = KEY_E;
	G_COMMAND_STATES[COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE].key = KEY_R;
	G_COMMAND_STATES[COMMAND_SWITCH_BRUSH_EFFECT_TO_MAX].key = KEY_A;
	G_COMMAND_STATES[COMMAND_SWITCH_BRUSH_EFFECT_TO_SHIFT].key = KEY_S;
	G_COMMAND_STATES[COMMAND_SWITCH_BRUSH_EFFECT_TO_RANDOM].key = KEY_N;

	v2 pressedMousePos = {};
	String draggedUiStringKey = {};

	Slider brushSizeSlider = {};
	brushSizeSlider.sliderAction = SLIDER_ACTION_BRUSH_SIZE;
	brushSizeSlider.unsignedIntToChange = &currentBrush.size;
	brushSizeSlider.min = 1;
	brushSizeSlider.max = 50;

	//NOTE: DEVELOPER HACK
	{
		InitializeNewImage("./assets/handmadelogo.png", &gameMemory, rootImageRaw, canvas, &loadedTexture, &currentBrush);
	}

	while (!WindowShouldClose())
	{
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
		int uiBoxArrayIndex = GetFrameModIndexLastFrame();
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
			InitializeNewImage(fileName, &gameMemory, rootImageRaw, canvas, &loadedTexture, &currentBrush);

			if (rootImageRaw->dataSize > 15000000)
			{
				String notification = STRING("...Are you serious?!? Ok be patient with me, this image is freaking huge. I'm not going to run well at all.");
				InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
			}
			else if (rootImageRaw->dataSize > 8000000)
			{
				String notification = STRING("Uh...I'm not really ready to edit images this big yet, but I can try. Don't blame me if I'm slow though. You asked for it.");
				InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
			}
			else if (rootImageRaw->dataSize > 5000000)
			{
				String notification = STRING("Woah, this image is kind of large!. I'll try my best...");
				InitNotificationMessage(notification, &gameMemory.circularNotificationBuffer);
			}
			UnloadDroppedFiles(droppedFiles);
		}

		if (IsKeyDown(KEY_E))
		{
			int foo = 4;
		}

		for (int i = 0;
				i < COMMAND_COUNT;
				i++)
		{
			CommandState *state = G_COMMAND_STATES + i;
			KeyboardKey key = state->key;
			if (key)
			{
				state->down = IsKeyDown(key);
				state->pressed = IsKeyPressed(key);
			}
			else
			{
				state->down = false;
				state->pressed = false;
			}
		}

		for (int i = 0;
				i < G_UI_STATE->uiBoxCount;
				i++)
		{
			UiBox *uiBox = &G_UI_STATE->uiBoxes[uiBoxArrayIndex][i];
			if (IsFlag(uiBox, UI_FLAG_INTERACTABLE))
			{
				uiBox->cursorRelativePixelPos = mousePixelPos - uiBox->rect.pos;

				if (IsInRectV2(mousePixelPos, uiBox->rect))
				{
					uiBox->hovered = true;
					uiBox->pressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
					uiBox->down = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

					if (uiBox->uiInputs.command)
					{
						CommandState *state = G_COMMAND_STATES + uiBox->uiInputs.command;
						state->down |= uiBox->down;
						state->pressed |= uiBox->pressed;
					}

					if (uiBox->pressed && uiBox->keyString.length)
					{
						draggedUiStringKey = ReallocString(uiBox->keyString, &gameMemory.mouseClickArena);
					}
				}

				if (uiBox->uiInputs.sliderAction && draggedUiStringKey == uiBox->keyString)
				{
					float normPressedPosInRect = (pressedMousePos.x - uiBox->rect.pos.x) / uiBox->rect.dim.x;
					float normDifference = (pressedMousePos.x - mousePixelPos.x) / uiBox->rect.dim.x;
					float normValue = ClampF32(0, normPressedPosInRect - normDifference, 1);

					//TODO: lookup slider
					*brushSizeSlider.unsignedIntToChange = (u32) RoundF32(LerpF32(brushSizeSlider.min, normValue, brushSizeSlider.max));
				}
			}
		}

		if (IsCommandPressed(COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE))
			currentBrush.brushEffect = BRUSH_EFFECT_ERASE_EFFECT;
		if (IsCommandPressed(COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE))
			currentBrush.brushEffect = BRUSH_EFFECT_REMOVE;
		if (IsCommandPressed(COMMAND_SWITCH_BRUSH_EFFECT_TO_MAX))
			currentBrush.brushEffect = BRUSH_EFFECT_MAX;
		if (IsCommandPressed(COMMAND_SWITCH_BRUSH_EFFECT_TO_SHIFT))
			currentBrush.brushEffect = BRUSH_EFFECT_SHIFT;
		if (IsCommandPressed(COMMAND_SWITCH_BRUSH_EFFECT_TO_RANDOM))
			currentBrush.brushEffect = BRUSH_EFFECT_RANDOM;

		if (IsCommandPressed(COMMAND_EXPORT_IMAGE))
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

		UiBox *canvasUiBox = GetUiBoxLastFrameOfStringKey(G_CANVAS_STRING_TAG_CHARS);
		if (canvasUiBox && canvasUiBox->down && !imageIsBroken)
		{
			if (canvasUiBox->pressed)
			{
				unsigned char *newRollbackImage = &canvas->rollbackImageData[(canvas->drawnImageData.dataSize * canvas->rollbackIndexNext)];
				memcpy(newRollbackImage, canvas->drawnImageData.dataU8, canvas->drawnImageData.dataSize);
				canvas->rollbackIndexNext =	ModNextU32(canvas->rollbackIndexNext, canvas->rollbackSizeCount - 1);
				if (canvas->rollbackIndexNext == canvas->rollbackIndexStart)
				{
					canvas->rollbackIndexStart = ModNextU32(canvas->rollbackIndexStart, canvas->rollbackSizeCount - 1);
				}
			}

			ProcessedImage *processedImage = {};
			for (u32 i = 0; i < threadCount; i++)
			{
				ProcessedImage *processedImageOfIndex = processedImages + i;
				if (!processedImageOfIndex->active)
				{
					processedImage = processedImageOfIndex;
					break;
				}
			}

			Color colorToPaint = {};

			if (currentBrush.brushEffect != BRUSH_EFFECT_ERASE_EFFECT)
			{
				unsigned int processedImageIndex = (processedImage)
					? processedImage->index
					: threadCount + 1;
				//NOTE: (Ahmayk) lol
				ASSERT(processedImageIndex < 255);
				colorToPaint.r = (u8) currentBrush.brushEffect;
				colorToPaint.g = (u8) RandomInRangeI32(0, 255);
				colorToPaint.a = (u8) processedImageIndex;
			}

			f32 scale = MaxF32(1, canvas->drawnImageData.dim.x / canvasUiBox->rect.dim.x);
			v2 startPos = (mousePixelPos - RayVectorToV2(GetMouseDelta()) - canvasUiBox->rect.pos) * scale;
			v2 endPos = (mousePixelPos - canvasUiBox->rect.pos) * scale;

			iv2 startPosIV2;
			startPosIV2.x = (u32) RoundF32(startPos.x);
			startPosIV2.y = (u32) RoundF32(startPos.y);
			iv2 endPosIV2;
			endPosIV2.x = (u32) RoundF32(endPos.x);
			endPosIV2.y = (u32) RoundF32(endPos.y);

			CanvasDrawCircleStroke(canvas, startPosIV2, endPosIV2, currentBrush.size, colorToPaint);

			ArenaPair arenaPair = {};
			if (processedImage)
			{
				arenaPair = ArenaPairAssign(&gameMemory.conversionArenaGroup);
			}

			if (processedImage && arenaPair.arena1 && arenaPair.arena2)
			{
				processedImage->arenaPair = arenaPair;
				StartProcessedImageWork(canvas, threadCount, processedImage, threadWorkQueue);
			}
			else
			{
				// Print("No thread avaliabe");
				canvas->proccessAsap = true;
			}

			canvas->needsTextureUpload = true;
		}

		if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown((KEY_RIGHT_CONTROL))) && IsKeyPressed(KEY_Z))
		{
			if (canvas->rollbackIndexNext != canvas->rollbackIndexStart)
			{
				ModBackU32(canvas->rollbackIndexNext, canvas->rollbackSizeCount - 1);
				unsigned char *rollbackImage = &canvas->rollbackImageData[(canvas->drawnImageData.dataSize * canvas->rollbackIndexNext)];
				memcpy(canvas->drawnImageData.dataU8, rollbackImage, canvas->drawnImageData.dataSize);
				canvas->proccessAsap = true;
				canvas->needsTextureUpload = true;
				canvas->oldDataPresent = true;
				imageIsBroken = false;
			}
			else
			{
				String notification = STRING("The past is no more. You must now live with your mistakes. You've run out of undos!");
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
					StartProcessedImageWork(canvas, threadCount, processedImage, threadWorkQueue);
				}
			}
		}

		ProcessedImage *latestCompletedProcessedImage = {};
		for (u32 i = 0; i < threadCount; i++)
		{
			ProcessedImage *processedImageOfIndex = processedImages + i;
			if (processedImageOfIndex->active && processedImageOfIndex->frameFinished > 0)
			{
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

		if (latestCompletedProcessedImage)
		{
			if (latestCompletedProcessedImage->finalProcessedImageRaw.dataU8)
			{
				UploadAndReplaceTexture(&latestCompletedProcessedImage->finalProcessedImageRaw, &loadedTexture);
				// Print("Uploading New Image from thread " + IntToString(latestCompletedProcessedImage->index));
				//TODO: put the latest uploaded image somewhere for safekeeping?
			}
			else
			{
				imageIsBroken = true;
			}

			ResetProcessedImage(latestCompletedProcessedImage, canvas);
		}

		if (canvas->needsTextureUpload)
		{
			canvas->needsTextureUpload = false;

			u32 drawingRectCount = GetDrawingRectCount(canvas);
			for (u32 rectIndex = 0; rectIndex < drawingRectCount; rectIndex++)
			{
				if (canvas->drawingRectDirtyList[rectIndex])
				{
					RectIV2 drawingRect = GetDrawingRectFromIndex(canvas, rectIndex);
					u32 pixelCount = drawingRect.dim.x * drawingRect.dim.y;
					ArenaMarker marker {};
					Color *pixels = ARENA_PUSH_ARRAY_MARKER(&gameMemory.temporaryArena, pixelCount, Color, &marker);
					memset(pixels, 0, pixelCount * sizeof(Color));

					u32 pixelIndex = 0;
					u32 startY = drawingRect.pos.y;
					u32 endY = startY + drawingRect.dim.y;
					for (u32 y = startY; y < endY; y++)
					{
						u32 startIndex = (canvas->drawnImageData.dim.x * y) + drawingRect.pos.x;
						u32 endIndex = startIndex + drawingRect.dim.x;
						for (u32 i = startIndex; i < endIndex; i++)
						{
							Color canvasPixel = ((Color *)canvas->drawnImageData.dataU8)[i];
							if (canvasPixel.r)
							{
								Color drawnPixel = G_BRUSH_EFFECT_COLORS[canvasPixel.r];
								if (canvasPixel.a < 255)
								{
									drawnPixel.a = 127;
								}
								Color *outPixel = (pixels + pixelIndex);
								*outPixel = drawnPixel;
							}
#if 0
							else
							{
								Color *outPixel = (pixels + pixelIndex);
								*outPixel = PINK;
								outPixel->a = 100;
							}
#endif

							pixelIndex++;
						}
					}

					UpdateRectInTexture(&canvas->textureDrawing, pixels, drawingRect);
					ArenaPopMarker(marker);

					canvas->drawingRectDirtyList[rectIndex] = false;
				}
			}
			GenTextureMipmaps(&canvas->textureDrawing);
		}

		//----------------------------------------------------
		//---------------------RENDER-------------------------
		//----------------------------------------------------

		// NOTE: We start at 1 so that we always have a null uiBox
		G_UI_STATE->uiBoxCount = 1;
		G_UI_STATE->uiSettings = {};
		G_UI_STATE->parentStackCount = {};
		UiSettings *uiSettings = &G_UI_STATE->uiSettings;

		BeginDrawing();

		ClearBackground(Color{127, 127, 127, 255});

		uiSettings->font = defaultFont;

		float titleBarHeight = 20;

		SetUiAxis({UI_SIZE_KIND_PIXELS, (f32) windowDim.x}, {UI_SIZE_KIND_PIXELS, (f32) windowDim.y});
		CreateUiBox();
		UiParent()
		{
			uiSettings->frontColor = BLACK;
			uiSettings->borderColor = DARKGRAY;
			uiSettings->backColor = Color{191, 191, 191, 255};
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, titleBarHeight});
			CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				//TODO: Menu
			}

			SetUiAxis({UI_SIZE_KIND_PIXELS, (f32) windowDim.x}, {UI_SIZE_KIND_PIXELS, windowDim.y - titleBarHeight});
			CreateUiBox();
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 0.5});
				CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER);
				UiParent()
				{
					if (loadedTexture.id)
					{
						if (!imageIsBroken)
						{
							G_UI_INPUTS->texture = loadedTexture;
							SetUiAxis({UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT}, {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT});
							CreateUiBox(UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT);
						}
						else
						{
							String string = STRING("Congulations! You broke the image. (undo with Ctrl-Z)");
							SetUiAxis({UI_SIZE_KIND_TEXT, 1}, {UI_SIZE_KIND_TEXT, 1});
							CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_CENTER_IN_PARENT, string);
						}
					}
					else
					{
						String string = STRING("Drop any image into the window for editing.");
						SetUiAxis({UI_SIZE_KIND_TEXT, 1}, {UI_SIZE_KIND_TEXT, 1});
						CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_CENTER_IN_PARENT, string);
					}
				}
				SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 0.5});
				CreateUiBox(UI_FLAG_DRAW_BORDER);
				UiParent()
				{
					if (canvas->textureVisualizedFilteredRootImage.id)
					{
						G_UI_INPUTS->texture = canvas->textureVisualizedFilteredRootImage;
						SetUiAxis({UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT}, {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT});
						CreateUiBox(UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT);
					}
					if (canvas->textureDrawing.id)
					{
						G_UI_INPUTS->texture = canvas->textureDrawing;
						SetUiAxis({UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT}, {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT});
						CreateUiBox(UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT | UI_FLAG_INTERACTABLE, STRING(G_UI_HASH_TAG) + G_CANVAS_STRING_TAG_CHARS);
					}
				}
			}
		}

		float toolbarWidth = G_TOOLBOX_WIDTH_AND_HEIGHT * 2;

		SetUiAxis({UI_SIZE_KIND_PIXELS, toolbarWidth}, {UI_SIZE_KIND_PIXELS});
		G_UI_INPUTS->relativePixelPosition = v2{0, (float)titleBarHeight};
		uiSettings->backColor = Color{191, 191, 191, 255};
		CreateUiBox(UI_FLAG_DRAW_BACKGROUND);
		UiParent()
		{
			ReactiveUiColorState uiColorState = {};

			uiSettings->frontColor = BLACK;
			uiSettings->borderColor = GRAY;

			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
				CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
				CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
				CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
				CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
				CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
				CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
				CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
				CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
				CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
				CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
			}

			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, 40});
			CreateUiBox();

			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				CreateBrushEffectButton(BRUSH_EFFECT_ERASE_EFFECT, STRING("Ers"), Color{245, 245, 245, 255}, COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE, &currentBrush);
				CreateBrushEffectButton(BRUSH_EFFECT_REMOVE, STRING("Rmv"), G_BRUSH_EFFECT_COLORS[BRUSH_EFFECT_REMOVE], COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE, &currentBrush);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				CreateBrushEffectButton(BRUSH_EFFECT_MAX, STRING("Max"), G_BRUSH_EFFECT_COLORS[BRUSH_EFFECT_MAX], COMMAND_SWITCH_BRUSH_EFFECT_TO_MAX, &currentBrush);
				CreateBrushEffectButton(BRUSH_EFFECT_SHIFT, STRING("Sft"), G_BRUSH_EFFECT_COLORS[BRUSH_EFFECT_SHIFT], COMMAND_SWITCH_BRUSH_EFFECT_TO_SHIFT, &currentBrush);
			}
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			UiParent()
			{
				CreateBrushEffectButton(BRUSH_EFFECT_RANDOM, STRING("Rnd"), G_BRUSH_EFFECT_COLORS[BRUSH_EFFECT_RANDOM], COMMAND_SWITCH_BRUSH_EFFECT_TO_RANDOM, &currentBrush);
			}

			// SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
			// CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
			// UiParent()
			// {
			// }

			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, 40});
			CreateUiBox();

			uiSettings->backColor = Color{191, 191, 191, 255};
			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_TEXT});
			CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_CENTERED, U32ToString(currentBrush.size, StringArena()));

			ReactiveUiColor uiColorParent = {};
			uiColorParent.down = Color{220, 220, 220, 255};
			uiColorParent.hovered = Color{200, 200, 200, 255};
			uiColorParent.neutral = Color{180, 180, 180, 255};
			String stringKey = STRING("brushSizeSlider");
			UiBox *sliderUiBox = GetUiBoxLastFrameOfStringKey(stringKey);
			uiSettings->backColor = GetReactiveColor(sliderUiBox, uiColorParent, false);
			G_UI_INPUTS->sliderAction = SLIDER_ACTION_BRUSH_SIZE;

			Slider slider = brushSizeSlider;
			float value = MapNormalizeF32(brushSizeSlider.min, (f32) *brushSizeSlider.unsignedIntToChange, brushSizeSlider.max);
			G_UI_INPUTS->value = value;

			SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT * 0.5f});
			CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_INTERACTABLE | UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER, G_UI_HASH_TAG + stringKey);
			UiParent()
			{
				SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, value}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 1});
				ReactiveUiColor uiColorChild = {};
				uiColorChild.down = Color{20, 131, 255, 255};
				uiColorChild.hovered = Color{10, 131, 251, 255};
				uiColorChild.neutral = Color{0, 121, 241, 255};
				uiSettings->backColor = GetReactiveColor(sliderUiBox, uiColorChild, false);
				CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER);
			}

			SetUiAxis({UI_SIZE_KIND_PIXELS, toolbarWidth}, {UI_SIZE_KIND_PIXELS, toolbarWidth});

			uiSettings->backColor = G_BRUSH_EFFECT_COLORS[currentBrush.brushEffect];
			if (currentBrush.brushEffect == BRUSH_EFFECT_ERASE_EFFECT)
				uiSettings->backColor = Color{245, 245, 245, 255};
			uiSettings->borderColor = BLACK;
			CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
		}

		if (G_NOTIFICATION_MESSAGE.length)
		{
			SetUiAxis({UI_SIZE_KIND_PIXELS, (f32) windowDim.x}, {UI_SIZE_KIND_TEXT});
			uiSettings->frontColor = Color{255, 255, 255, (unsigned char)(G_NOTIFICATION_ALPHA * 255)};
			uiSettings->backColor = Color{100, 100, 100, (unsigned char)(G_NOTIFICATION_ALPHA * 255)};
			G_UI_INPUTS->relativePixelPosition = v2{0, titleBarHeight};
			CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT, G_NOTIFICATION_MESSAGE);

			G_NOTIFICATION_ALPHA -= 0.001f;
			if (G_NOTIFICATION_ALPHA <= 0)
			{
				G_NOTIFICATION_MESSAGE = {};
				G_NOTIFICATION_ALPHA = {};
			}
		}

		SetUiAxis({UI_SIZE_KIND_TEXT}, {UI_SIZE_KIND_TEXT});
		uiSettings->frontColor = BLACK;
		u32 fps = GetFPS();
		String fpsString = STRING("FPS: ") + U32ToString(fps, StringArena());
		if (fps < 60)
			uiSettings->frontColor = RED;
		CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT, fpsString);

		uiSettings->frontColor = DARKGRAY;
		G_UI_INPUTS->relativePixelPosition = v2{windowDim.x * 0.07f, 2};
		String label = STRING("Use 0-5 to toggle differnet PNG filter algorythms");
		CreateUiBox(UI_FLAG_DRAW_TEXT, label);

		uiSettings->frontColor = BLACK;
		G_UI_INPUTS->relativePixelPosition = v2{windowDim.x * 0.45f, 2};
		label = STRING("PNG Filter: ") + G_PNG_FILTER_NAMES[canvas->currentPNGFilterType];
		CreateUiBox(UI_FLAG_DRAW_TEXT, label);

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
		CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT, label);

		int uiBoxArrayIndexThisFrame = GetFrameModIndexThisFrame();

		if (G_UI_STATE->uiBoxCount)
		{
			for (int i = 1;
					i < G_UI_STATE->uiBoxCount;
					i++)
			{
				UiBox *uiBox = &G_UI_STATE->uiBoxes[uiBoxArrayIndexThisFrame][i];

				for (int j = 0;
						j < ARRAY_COUNT(uiBox->uiSettings.uiSizes);
						j++)
				{
					UiSize uiSize = uiBox->uiSettings.uiSizes[j];
					switch (uiSize.kind)
					{
						case UI_SIZE_KIND_TEXTURE:
							{
								v2 dim = GetTextureDim(uiBox->uiInputs.texture);
								uiBox->rect.dim.elements[j] = dim.elements[j];
								break;
							}
						case UI_SIZE_KIND_PIXELS:
							{
								uiBox->rect.dim.elements[j] = uiSize.value;
								break;
							}
						case UI_SIZE_KIND_TEXT:
							{
								uiBox->rect.dim.elements[j] = uiBox->textDim.elements[j];
							}
						case UI_SIZE_KIND_PERCENT_OF_PARENT:
						case UI_SIZE_KIND_CHILDREN_OF_SUM:
						case UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT:
							break;

							InvalidDefaultCase
					}
				}
			}

			for (int i = 1;
					i < G_UI_STATE->uiBoxCount;
					i++)
			{
				UiBox *uiBox = &G_UI_STATE->uiBoxes[uiBoxArrayIndexThisFrame][i];

				if (!uiBox->parent)
				{
					CalculateUiUpwardsDependentSizes(uiBox);
					CalculateUiDownwardsDependentSizes(uiBox);
					CalculateUiRelativePositions(uiBox);
					CalculateUiPosGivenReletativePositions(uiBox);
					RenderUiEntries(uiBox, windowDim);
				}
			}
		}

		EndDrawing();

		ArenaReset(&gameMemory.temporaryArena);
		Arena *memoryArenaLastFrame = GetTwoFrameArenaLastFrame(&gameMemory);
		ArenaReset(memoryArenaLastFrame);

		G_CURRENT_FRAME++;
		pngFilterLastFrame = stbi_write_force_png_filter;
	}

	RayCloseWindow();
}


#include "base/base.h"

//NOTE: (Ahmayk) hack to allow raw openGL calls in our code 
#include "../includes/raylib/src/external/glad.h" // GLAD extensions loading library, includes OpenGL headers

#include "../includes/raylib/src/raylib.h"
#include "../includes//raylib//src/external/glfw/include/GLFW/glfw3.h"
#include "../includes/raylib/src/rlgl.h"

#include "ui/ui_raylib.h"
#include "widgets.h"
#include "vn_math_external.h"
#include "platform_win32.h"
#include "main.h"
#include "imageFile.h"
#include "imageProcess.h"
#include "imageDraw.h"

#include "assets/font.h"
#include "assets/defaultImage.h"

#include <cstring>  //memcpy

//NOTE: (Ahmayk) force us to delete printfs in release mode
#if DEBUG_MODE
#include  "stdio.h"
#endif

AppCommand *PushAppCommand(AppCommandBuffer *appCommandBuffer)
{
	AppCommand *result;
	if (ASSERT(appCommandBuffer->count < appCommandBuffer->size))
	{
		result = &appCommandBuffer->appCommands[appCommandBuffer->count++];
	}
	else
	{
		static AppCommand stub = {};
		result = &stub;
	}
	*result = {};
	return result;
}

//NOTE: (Ahmayk) This needs to be redesigned to include key modifiers 
static KeyboardKey COMMAND_KEY_BINDINGS[] = {
	KEY_NULL,
	KEY_R,
	KEY_A,
	KEY_S,
	KEY_N,
	KEY_B,
	KEY_E,
	KEY_T,
	KEY_NULL,
	KEY_NULL,
};

b32 IsCommandKeyBindingDown(COMMAND command)
{
	b32 result = false;
	KeyboardKey key = {};
	if (ASSERT(command < ARRAY_COUNT(COMMAND_KEY_BINDINGS)))
	{
		key = COMMAND_KEY_BINDINGS[command];
	}
	if (key && IsKeyDown(key))
	{
		result = true;
	}
	return result;
}

b32 IsCommandKeyBindingPressed(COMMAND command)
{
	b32 result = false;
	KeyboardKey key = {};
	if (ASSERT(command < ARRAY_COUNT(COMMAND_KEY_BINDINGS)))
	{
		key = COMMAND_KEY_BINDINGS[command];
	}
	if (key && IsKeyPressed(key))
	{
		result = true;
	}
	return result;
}

//TODO: (Ahmayk) remove and replace with better error reporting/messaging system
static NotificationMessage notificationMessage = {};

void InitNotificationMessage(String string, Arena *circularNotificationBuffer)
{
    notificationMessage.string = ReallocString(string, circularNotificationBuffer);
	notificationMessage.alpha = 1.0f;
}

AppState *InitApp(GameMemory *gameMemory, u32 threadCount)
{
	AppState *appState = ARENA_PUSH_STRUCT(&gameMemory->permanentArena, AppState);
	Canvas *canvas = &appState->canvas;
	canvas->currentPNGFilterType = PNG_FILTER_TYPE_OPTIMAL;

	bool imageIsBroken = {};

	appState->processedImages = ARENA_PUSH_ARRAY(&gameMemory->permanentArena, threadCount, ProcessedImage);
	for (u32 i = 0; i < threadCount; i++)
	{
		ProcessedImage *processedImage = appState->processedImages + i;
		processedImage->rootImageRaw = &appState->rootImageRaw;
		processedImage->canvas = canvas;
		processedImage->index = i;
	}

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

	appState->defaultFont = LoadFontFromMemory(".otf", PAINT_FONT_DATA, ARRAY_COUNT(PAINT_FONT_DATA), 12, 0, 0);
	appState->defaultUiFont.id = appState->defaultFont.texture.id;
	appState->defaultUiFont.data = &appState->defaultFont;

	appState->currentBadpaintPixelType = BADPAINT_PIXEL_TYPE_REMOVE;
	//appState->toolSize = 10;
	appState->toolSize = 50;
	appState->currentTool = BADPAINT_TOOL_PENCIL;

	BADPAINT_TOOL_TYPE badpaintSpriteSheetOrder[] = {
		BADPAINT_TOOL_TEST,
		BADPAINT_TOOL_PENCIL,
		BADPAINT_TOOL_ERASER,
	};
	appState->toolbrushSpriteSheet = LoadTexture("buttonSprites.png");
	i32 spriteSheetBoxSize = 30;
	for (u32 i = 0; i < ARRAY_COUNT(badpaintSpriteSheetOrder); i++)
	{
		Tool *tool = &appState->tools[badpaintSpriteSheetOrder[i]];
		for (i32 interactionIndex = 0; interactionIndex < INTERACTION_STATE_COUNT; interactionIndex++)
		{
			tool->uiTextureViews[interactionIndex] = UiRaylibTextureToUiTextureView(&appState->toolbrushSpriteSheet);
			tool->uiTextureViews[interactionIndex].viewRect.pos = iv2{spriteSheetBoxSize * interactionIndex, spriteSheetBoxSize * (i32) i};
			tool->uiTextureViews[interactionIndex].viewRect.dim = iv2{spriteSheetBoxSize, spriteSheetBoxSize};
		}
	}

	appState->rootImageRaw = LoadDataIntoRawImage(&DEFAULT_IMAGE_DATA[0], ARRAY_COUNT(DEFAULT_IMAGE_DATA), gameMemory);
	if (appState->rootImageRaw.dataSize)
	{
		InitializeNewImage(gameMemory, &appState->rootImageRaw, canvas, &appState->loadedTexture, appState->processedImages, threadCount);
	}

	return appState;
}

void RunApp(PlatformWorkQueue *threadWorkQueue, GameMemory *gameMemory, unsigned int threadCount)
{
	AppState *appState = InitApp(gameMemory, threadCount);
	UiState *uiState = UiInit(&gameMemory->permanentArena);

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
#if OS_WINDOWS
			__debugbreak();
#else
#error Unimplemented platform crash thing here
#endif
		}

		//NOTE: (Ahmayk) this nonsense is so that we can make a pointer to a variable on the stack
		//havving this be a pointer makes code refernecing this easier to maintain when we move code from here to elsewhere
		FrameState _frameState = {};
		FrameState *frameState = &_frameState;

		frameState->windowDim.x = GetScreenWidth();
		frameState->windowDim.y = GetScreenHeight();
		frameState->mousePixelPos = iv2{GetMouseX(), GetMouseY()};
		frameState->mousePixelPosPrevious.x = frameState->mousePixelPos.x - (i32) GetMouseDelta().x;
		frameState->mousePixelPosPrevious.y = frameState->mousePixelPos.y - (i32) GetMouseDelta().y;
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
		{
			appState->lastPressedUiHash = {};
		}

		if (IsFileDropped())
		{
			FilePathList droppedFiles = LoadDroppedFiles();
			char *fileName = droppedFiles.paths[0];
			ArenaMarker loadMarker = ArenaPushMarker(&gameMemory->temporaryArena);
			unsigned int fileSize = {};
			u8 *fileData = LoadDataFromDisk(fileName, &fileSize, &gameMemory->temporaryArena);
			const char *getFileExtension = GetFileExtension(fileName);
			if (fileData)
			{
				//NOTE: (Ahmayk) prints error internally (need to change)
				appState->rootImageRaw = LoadDataIntoRawImage(fileData, fileSize, gameMemory);
			}
			else
			{
				String notification = STRING("Oops! I failed to read the file at all! Sorry! Guess you're out of luck pal. No badpaint for that file today.");
				InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
			}
			UnloadDroppedFiles(droppedFiles);
			ArenaPopMarker(loadMarker);

			if (appState->rootImageRaw.dataSize)
			{
				InitializeNewImage(gameMemory, &appState->rootImageRaw, &appState->canvas, &appState->loadedTexture, appState->processedImages, threadCount);
				if (appState->rootImageRaw.dataSize > MegaByte * 500)
				{
					String notification = STRING("You like to play dangerously, don't you?");
					InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
				}
				else if (appState->rootImageRaw.dataSize > MegaByte * 100)
				{
					String notification = STRING("This image is CHUNKY! Some things might be a little slow.");
					InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
				}
			}
		}

		frameState->appCommandBuffer.size = 50;
		frameState->appCommandBuffer.appCommands = ARENA_PUSH_ARRAY_MARKER(&gameMemory->temporaryArena, frameState->appCommandBuffer.size, AppCommand, &frameState->appCommandBuffer.arenaMarker);

		for (u32 i = 0; i < COMMAND_COUNT; i++)
		{
			if (IsCommandKeyBindingPressed((COMMAND) i))
			{
				AppCommand *appCommand = PushAppCommand(&frameState->appCommandBuffer);
				appCommand->command = (COMMAND) i;
			}
		}

		UiBuffer *uiBufferLastFrame = &uiState->uiBuffers[1 - uiState->uiBufferIndex];
		for (u32 i = 0; i < uiBufferLastFrame->uiBlockCount; i++)
		{
			UiBlock *uiBlock = &uiBufferLastFrame->uiBlocks[i];
			if ((uiBlock->flags & UI_FLAG_INTERACTABLE) && ASSERT(uiBlock->hash))
			{
				if (IsInRectV2(frameState->mousePixelPos, uiBlock->rect))
				{
					frameState->uiInteractionHashes.hashMouseHover = uiBlock->hash;
					if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
					{
						frameState->uiInteractionHashes.hashMousePressed = uiBlock->hash;
						appState->lastPressedUiHash = uiBlock->hash;
					}
					if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
					{
						frameState->uiInteractionHashes.hashMouseDown = uiBlock->hash;
					}
				}
			}
		}

		//----------------------------------------------------
		//------------------------UI--------------------------
		//----------------------------------------------------

		UiBuffer *uiBufferCurrent = &uiState->uiBuffers[uiState->uiBufferIndex];
		// NOTE: We start at 1 so that we always have a null uiBlock
		uiBufferCurrent->uiBlockCount = 1;
		uiState->parentStackCount = {};

		float titleBarHeight = 20;

		UiBlockColors defaultBlockColors = {};
		defaultBlockColors.backColor = ColorU32{191, 191, 191, 255};
		defaultBlockColors.frontColor = COLORU32_BLACK;
		defaultBlockColors.borderColor = COLORU32_DARKGRAY;

		UiBlock *root= UiCreateBlock(uiState);
		root->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, (f32) frameState->windowDim.x};
		root->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, (f32) frameState->windowDim.y};
		root->uiChildLayoutType = UI_CHILD_LAYOUT_TOP_TO_BOTTOM;
		UI_PARENT_SCOPE(uiState, root)
		{
			float menuBarHeight = 20;
			UiBlock *menuBar = UiCreateBlock(uiState);
			menuBar->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
			menuBar->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, menuBarHeight};
			menuBar->uiChildAlignTypes[UI_AXIS_Y] = UI_CHILD_ALIGN_CENTER;
			UI_PARENT_SCOPE(uiState, menuBar)
			{
				String tempMenuStrings[] = {STRING("File"), STRING("Edit"), STRING("Image")};
				for (u32 i = 0; i < ARRAY_COUNT(tempMenuStrings); i++)
				{
					UiBlock *m = UiCreateBlock(uiState);
					m->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, 10};
					m->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};

					UiBlock *b = UiCreateBlock(uiState);
					b->flags = UI_FLAG_DRAW_TEXT;
					b->uiSizes[UI_AXIS_X] = {UI_SIZE_TEXT};
					b->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
					b->uiTextAlignTypes[UI_AXIS_X] = UI_TEXT_ALIGN_CENTER;
					b->uiTextAlignTypes[UI_AXIS_Y] = UI_TEXT_ALIGN_CENTER;
					b->string = tempMenuStrings[i];
					b->uiFont = appState->defaultUiFont;
					b->uiBlockColors.frontColor = COLORU32_BLACK;
				}

				UiBlock *menuBarLine = UiCreateBlock(uiState);
				menuBarLine->flags = UI_FLAG_DRAW_LINE_BOTTOMLEFT_TOPRIGHT;
				menuBarLine->uiPosition[UI_AXIS_X] = {UI_POSITION_RELATIVE, 7};
				menuBarLine->uiPosition[UI_AXIS_Y] = {UI_POSITION_PERCENT_OF_PARENT, 0.3f};
				menuBarLine->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
				menuBarLine->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 0.7f};
				menuBarLine->uiBlockColors.frontColor = COLORU32_BLACK;
			}

			UiBlock *body = UiCreateBlock(uiState);
			body->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
			body->uiSizes[UI_AXIS_Y] = {UI_SIZE_FILL};
			UI_PARENT_SCOPE(uiState, body)
			{
				UiBlock *toolbar = UiCreateBlock(uiState);
				//toolbar->flags = UI_FLAG_DRAW_BACKGROUND;
				toolbar->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, 65};
				toolbar->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
				toolbar->uiChildLayoutType = UI_CHILD_LAYOUT_TOP_TO_BOTTOM;
				//toolbar->uiBlockColors.backColor = ColorU32{100, 100, 100, 100};
				UI_PARENT_SCOPE(uiState, toolbar)
				{
					{
						UiBlock *b = UiCreateBlock(uiState);
						b->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
						b->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, 5};
					}

					WidgetToolButton(uiState, appState, frameState, BADPAINT_TOOL_PENCIL, COMMAND_SWITCH_TOOL_TO_PENCIL);
					WidgetToolButton(uiState, appState, frameState, BADPAINT_TOOL_ERASER, COMMAND_SWITCH_TOOL_TO_ERASER);
					WidgetToolButton(uiState, appState, frameState, BADPAINT_TOOL_TEST, COMMAND_SWITCH_TOOL_TO_TEST);

					{
						UiBlock *b = UiCreateBlock(uiState);
						b->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
						b->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, 50};
					}

					WidgetBrushEffectButton(uiState, appState, frameState, BADPAINT_PIXEL_TYPE_REMOVE, STRING("Remv"), COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_REMOVE);
					WidgetBrushEffectButton(uiState, appState, frameState, BADPAINT_PIXEL_TYPE_MAX, STRING("Max"), COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_MAX);
					WidgetBrushEffectButton(uiState, appState, frameState, BADPAINT_PIXEL_TYPE_SHIFT, STRING("Sft"), COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_SHIFT);
					WidgetBrushEffectButton(uiState, appState, frameState, BADPAINT_PIXEL_TYPE_RANDOM, STRING("Rand"), COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_RANDOM);

					{
						UiBlock *seperator = UiCreateBlock(uiState);
						seperator->uiPosition[UI_AXIS_X] = {UI_POSITION_PERCENT_OF_PARENT, 1};
						seperator->uiPosition[UI_AXIS_Y] = {UI_POSITION_RELATIVE, 0};
						seperator->flags = UI_FLAG_DRAW_LINE_BOTTOMLEFT_TOPRIGHT;
						seperator->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, 4};
						seperator->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
						seperator->uiBlockColors.frontColor = COLORU32_BLACK;
					}
				}

				UiBlock *panelArea = UiCreateBlock(uiState);
				panelArea->uiSizes[UI_AXIS_X] = {UI_SIZE_FILL};
				panelArea->uiSizes[UI_AXIS_Y] = {UI_SIZE_FILL};
				UI_PARENT_SCOPE(uiState, panelArea)
				{

#if 0
					f32 relativeX = 0.5f;
					u32 leftPanelHash = Murmur3String("leftPanel");
					UiBlock *leftPanelPrev = GetUiBlockOfHashLastFrame(uiState, leftPanelHash);
					if (leftPanelPrev->hash)
					{
						f32 middleOfScreenX = windowDim.x * 0.5f;
						f32 moveRange = windowDim.x * 0.8f;
						f32 posX = ClampF32(middleOfScreenX - (moveRange * 0.5f), mousePixelPos.x, middleOfScreenX + (moveRange * 0.5f));
					}
#endif
					static b32 first = false;
					if (!first)
					{
						first = true;
						appState->rootUiPanel.childSplitAxis = UI_AXIS_X;
						appState->rootUiPanel.hash = Murmur3String("mainPanels");
						UiPanelPair panelPair1 = SplitPanel(&appState->rootUiPanel, &gameMemory->permanentArena, UI_AXIS_X, 0.85f);

						UiPanelPair panelPairImages = SplitPanel(panelPair1.uiPanel1, &gameMemory->permanentArena, UI_AXIS_X, 0.5f);
						panelPairImages.uiPanel1->uiPanelType = UI_PANEL_TYPE_FINAL_TEXTURE;
						panelPairImages.uiPanel2->uiPanelType = UI_PANEL_TYPE_CANVAS;

						UiPanelPair panelPairRightSidebar = SplitPanel(panelPair1.uiPanel2, &gameMemory->permanentArena, UI_AXIS_Y, 0.2f);
						panelPairRightSidebar.uiPanel1->uiPanelType = UI_PANEL_TYPE_NULL;
						panelPairRightSidebar.uiPanel2->uiPanelType = UI_PANEL_TYPE_LAYERS;
					}

					BuildPanelTree(uiState, appState, frameState, &appState->rootUiPanel);
				}
			}
		}

		//----------------------------------------------------
		//-----------------------APP--------------------------
		//----------------------------------------------------

		Canvas *canvas = &appState->canvas;
		AppCommandBuffer *appCommandBuffer = &frameState->appCommandBuffer;
		for (u32 commandIndex = 0; commandIndex < appCommandBuffer->count; commandIndex++)
		{
			AppCommand *appCommand = &appCommandBuffer->appCommands[commandIndex];
			switch(appCommand->command)
			{
				case COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_REMOVE:
				{
					appState->currentBadpaintPixelType = BADPAINT_PIXEL_TYPE_REMOVE;
				} break;
				case COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_MAX:
				{
					appState->currentBadpaintPixelType = BADPAINT_PIXEL_TYPE_MAX;
				} break;
				case COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_SHIFT:
				{
					appState->currentBadpaintPixelType = BADPAINT_PIXEL_TYPE_SHIFT;
				} break;
				case COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_RANDOM:
				{
					appState->currentBadpaintPixelType = BADPAINT_PIXEL_TYPE_RANDOM;
				} break;
				case COMMAND_SWITCH_TOOL_TO_PENCIL:
				{
					appState->currentTool = BADPAINT_TOOL_PENCIL;
				} break;
				case COMMAND_SWITCH_TOOL_TO_ERASER:
				{
					appState->currentTool = BADPAINT_TOOL_ERASER;
				} break;
				case COMMAND_SWITCH_TOOL_TO_TEST:
				{
					appState->currentTool = BADPAINT_TOOL_TEST;
				} break;
				case COMMAND_EXPORT_IMAGE:
				{
					if (appState->loadedTexture.height && appState->loadedTexture.width)
					{
						ArenaMarker arenaMarker = ArenaPushMarker(&gameMemory->temporaryArena);
						String filePath = AllocateString(256, &gameMemory->temporaryArena);
						u32 filepathLength;
						b32 success = GetPngImageFilePathFromUser(filePath.chars, filePath.length, &filepathLength);
						filePath.length = filepathLength;
						if (success && filePath.length)
						{
							Image exportImgae = LoadImageFromTexture(appState->loadedTexture);
							ExportImage(exportImgae, filePath);

							String notification = STRING("You have given new life to: ") + filePath;
							InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
						}
						else
						{
							String notification = STRING("Sorry the save failed. You fail too. You suck. Sorry.");
							InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
						}
						ArenaPopMarker(arenaMarker);
					}
					else
					{
						String notification = STRING("Bruh.");
						InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
					}
				} break;
				case COMMAND_PAINT_ON_CANVAS_BETWEEN_POSITIONS:
				{
					if (!appState->imageIsBroken)
					{
						b32 drewSomething = false;

						iv2 startPosIV2;
						startPosIV2.x = (u32) RoundF32(appCommand->value1V2.x);
						startPosIV2.y = (u32) RoundF32(appCommand->value1V2.y);
						iv2 endPosIV2;
						endPosIV2.x = (u32) RoundF32(appCommand->value2V2.x);
						endPosIV2.y = (u32) RoundF32(appCommand->value2V2.y);

						switch(appState->currentTool)
						{
							case BADPAINT_TOOL_PENCIL:
							{
								BadpaintPixel badpaintPixel = {};
								badpaintPixel.badpaintPixelType = appState->currentBadpaintPixelType;
								badpaintPixel.r1RandomValue = (u8) RandomInRangeI32(0, 255);
								badpaintPixel.processBatchIndex = canvas->processBatchIndex;
								drewSomething = CanvasDrawCircleStroke(&canvas->rootBadpaintPixels, startPosIV2, endPosIV2, appState->toolSize, badpaintPixel);
							} break;
							case BADPAINT_TOOL_ERASER:
							{
								BadpaintPixel badpaintPixel = {};
								badpaintPixel.processBatchIndex = canvas->processBatchIndex;
								drewSomething = CanvasDrawCircleStroke(&canvas->rootBadpaintPixels, startPosIV2, endPosIV2, appState->toolSize, badpaintPixel);
							} break;
							case BADPAINT_TOOL_TEST:
							{
								i32 halfBrushSize = (i32) RoundF32(appState->toolSize * 0.5f);
								i32 minX = MinI32(startPosIV2.x, endPosIV2.x) - halfBrushSize; 
								i32 minY = MinI32(startPosIV2.y, endPosIV2.y) - halfBrushSize; 
								i32 maxX = MaxI32(startPosIV2.x, endPosIV2.x) + halfBrushSize;
								i32 maxY = MaxI32(startPosIV2.y, endPosIV2.y) + halfBrushSize;
								for (u32 i = 0; i < 100; i++)
								{
									iv2 randomPoint2;
									randomPoint2.x = RandomInRangeI32(minX, maxX);
									randomPoint2.y = RandomInRangeI32(minY, maxY);
									iv2 randomPoint1;
									randomPoint1.x = RandomInRangeI32(minX, maxX);
									randomPoint1.y = RandomInRangeI32(minY, maxY);
									ImageSwapPoints(&appState->rootImageRaw, randomPoint1, randomPoint2);
									drewSomething = true;
								}
							} break;
							InvalidDefaultCase
						}

						if (drewSomething)
						{
							canvas->proccessAsap = true;
							canvas->saveRollbackOnNextPress = true;
							canvas->dataOnCanvas = true;
						}
					}

				} break;
				InvalidDefaultCase
			}
		}

		ArenaPopMarker(appCommandBuffer->arenaMarker);

		//TODO: (Ahmayk) Reenable undo when you have a plan to implement it properly
#if 0
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


		//TODO: (Ahmayk) turn into app commands
		if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown((KEY_RIGHT_CONTROL))) && IsKeyPressed(KEY_Z))
		{
			if (canvas->rollbackIndexNext != canvas->rollbackIndexStart)
			{
				canvas->rollbackIndexNext =	ModBackU32(canvas->rollbackIndexNext, canvas->rollbackSizeCount - 1);
				u8 *rollbackImage = &canvas->rollbackImageData[(canvas->drawnImageData.dataSize * canvas->rollbackIndexNext)];
				memcpy(canvas->drawnImageData.dataU8, rollbackImage, canvas->drawnImageData.dataSize);
				memset(canvas->drawingRectDirtyListFrame, 1, canvas->drawingRectCount * sizeof(b32));
				canvas->proccessAsap = true;
				appState->imageIsBroken = false;
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
				appState->imageIsBroken = false;
				canvas->rollbackHasRolledBackOnce = true;
				canvas->saveRollbackOnNextPress = false;
				canvas->dataOnCanvas = false;
				//printf("UNDO! start: %d next: %d\n", canvas->rollbackIndexStart, canvas->rollbackIndexNext);
			}
			else if (canvas->rollbackStartHasProgressed)
			{
				String notification = STRING("Tragedy has struck! For you must now live with your mistakes. You've run out of undos!");
				InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
			}
			else if (canvas->rollbackHasRolledBackOnce)
			{
				String notification = STRING("You cannot go before the begining of time, my friend. (Nothing else to undo)");
				InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
			}
			else
			{
				String notification = STRING("You must go forward before you can go backwards. (No undo history yet! Draw something!!!!)");
				InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
			}
		}
#endif

		//TODO: (Ahmayk) turn into app commands
		if (IsKeyPressed(KEY_ZERO))
		{
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_NONE;
			canvas->proccessAsap = true;
		}
		if (IsKeyPressed(KEY_ONE))
		{
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_SUB;
			canvas->proccessAsap = true;
		}
		if (IsKeyPressed(KEY_TWO))
		{
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_UP;
			canvas->proccessAsap = true;
		}
		if (IsKeyPressed(KEY_THREE))
		{
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_AVERAGE;
			canvas->proccessAsap = true;
		}
		if (IsKeyPressed(KEY_FOUR))
		{
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_PAETH;
			canvas->proccessAsap = true;
		}
		if (IsKeyPressed(KEY_FIVE))
		{
			canvas->currentPNGFilterType = PNG_FILTER_TYPE_OPTIMAL;
			canvas->proccessAsap = true;
		}

		if (canvas->initialized && canvas->proccessAsap)
		{
			ProcessedImage *processedImage = GetFreeProcessedImage(appState->processedImages, threadCount);
			if (processedImage)
			{
				Arena *arenaFiltered = ArenaGroupPushArena(&gameMemory->conversionArenaGroup);
				Arena *arenaVisualized = ArenaGroupPushArena(&gameMemory->conversionArenaGroup);
				Arena *arenaFinal = ArenaGroupPushArena(&gameMemory->conversionArenaGroup);
				if (arenaFiltered && arenaVisualized && arenaFinal)
				{
					processedImage->arenaFiltered = arenaFiltered;
					processedImage->arenaVisualized = arenaVisualized;
					processedImage->arenaFinal = arenaFinal;
					memcpy(processedImage->dirtyRectsInProcess, canvas->rootBadpaintPixels.drawingRectDirtyListProcess, canvas->rootBadpaintPixels.drawingRectCount * sizeof(b32));
					canvas->proccessAsap = false;
					processedImage->active = true;
					processedImage->processBatchIndex = canvas->processBatchIndex;
					canvas->processBatchIndex++;
					//NOTE: (Ahmayk) skip 0 as this represents already processed 
					if (canvas->processBatchIndex == 0)
					{
						canvas->processBatchIndex++;
					}
					memset(canvas->rootBadpaintPixels.drawingRectDirtyListProcess, 0, canvas->rootBadpaintPixels.drawingRectCount * sizeof(b32));
					PlatformAddThreadWorkEntry(threadWorkQueue, ProcessImageOnThread, (void *)processedImage);
					// Print("Queueing Work on thread " + IntToString(processedImage->index));
				}
				else
				{
					ArenaResetAndMarkAsReadyForAssignment(arenaFiltered);
					ArenaResetAndMarkAsReadyForAssignment(arenaVisualized);
					ArenaResetAndMarkAsReadyForAssignment(arenaFinal);
				}
			}
		}

		ProcessedImage *latestCompletedProcessedImage = {};
		for (u32 threadIndex = 0; threadIndex < threadCount; threadIndex++)
		{
			ProcessedImage *processedImageOfIndex = appState->processedImages + threadIndex;
			if (processedImageOfIndex->active && processedImageOfIndex->processingComplete)
			{
				for (u32 rectIndex = 0; rectIndex < canvas->rootBadpaintPixels.drawingRectCount; rectIndex++)
				{
					if (processedImageOfIndex->dirtyRectsInProcess[rectIndex])
					{
						canvas->rootBadpaintPixels.drawingRectDirtyListFrame[rectIndex] = true;
						RectIV2 drawingRect = GetDrawingRectFromIndex(canvas->rootBadpaintPixels.dim, canvas->rootBadpaintPixels.drawingRectDim, rectIndex);
						u32 pixelIndex = 0;
						u32 startY = drawingRect.pos.y;
						u32 endY = startY + drawingRect.dim.y;
						for (u32 y = startY; y < endY; y++)
						{
							u32 startIndex = (canvas->rootBadpaintPixels.dim.x * y) + drawingRect.pos.x;
							u32 endIndex = startIndex + drawingRect.dim.x;
							for (u32 i = startIndex; i < endIndex; i++)
							{
								BadpaintPixel *badpaintPixel = &canvas->rootBadpaintPixels.dataBadpaintPixel[i];
								if (badpaintPixel->processBatchIndex == processedImageOfIndex->processBatchIndex)
								{
									badpaintPixel->processBatchIndex = 0;
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
				iv2 finalImageDim = latestCompletedProcessedImage->finalProcessedImageRaw.dim;
				ArenaMarker marker = {};
				b32 *finalImageDirtyRects = ARENA_PUSH_ARRAY_MARKER(&gameMemory->temporaryArena, canvas->finalImageRectCount, b32, &marker);
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

					glBindTexture(GL_TEXTURE_2D, appState->loadedTexture.id);
					glPixelStorei(GL_UNPACK_ROW_LENGTH, appState->loadedTexture.width);
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
			}
			else
			{
				appState->imageIsBroken = true;
			}

			if (latestCompletedProcessedImage->imageFilteredVisualized.dataU8)
			{
				ImageRawRGBA32 *visualizedImage = &latestCompletedProcessedImage->imageFilteredVisualized;
				if (!canvas->textureVisualizedFilteredRootImage.id)
				{
					canvas->textureVisualizedFilteredRootImage.id = rlLoadTexture(visualizedImage->dataU8, visualizedImage->dim.x, visualizedImage->dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
					canvas->textureVisualizedFilteredRootImage.width = visualizedImage->dim.x;
					canvas->textureVisualizedFilteredRootImage.height = visualizedImage->dim.y; 
					canvas->textureVisualizedFilteredRootImage.mipmaps = 1;
					canvas->textureVisualizedFilteredRootImage.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
				}
				RectIV2 rect = {};
				rect.dim = visualizedImage->dim;
				UpdateRectInTexture(&canvas->textureVisualizedFilteredRootImage, visualizedImage->dataU8, rect);
			}

			ResetProcessedImage(latestCompletedProcessedImage, canvas);
		}

		b32 somethingToDraw = false;
		for (u32 rectIndex = 0; rectIndex < canvas->rootBadpaintPixels.drawingRectCount; rectIndex++)
		{
			if (canvas->rootBadpaintPixels.drawingRectDirtyListFrame[rectIndex])
			{
				somethingToDraw = true;
			}
		}

		if (somethingToDraw)
		{
			TextureGPU *textureGPU = &canvas->textureGPUDrawing;

			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, textureGPU->pboIDs[textureGPU->currentPboID]);
			ColorU32 *pixels = (ColorU32 *) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
			if (ASSERT(pixels))
			{
				for (u32 rectIndex = 0; rectIndex < canvas->rootBadpaintPixels.drawingRectCount; rectIndex++)
				{
					if (canvas->rootBadpaintPixels.drawingRectDirtyListFrame[rectIndex])
					{
						RectIV2 drawingRect = GetDrawingRectFromIndex(canvas->rootBadpaintPixels.dim, canvas->rootBadpaintPixels.drawingRectDim, rectIndex);
						u32 startY = drawingRect.pos.y;
						u32 endY = startY + drawingRect.dim.y;
						for (u32 y = startY; y < endY; y++)
						{
							u32 startIndex = (canvas->rootBadpaintPixels.dim.x * y) + drawingRect.pos.x;
							u32 endIndex = startIndex + drawingRect.dim.x;
							for (u32 i = startIndex; i < endIndex; i++)
							{
								BadpaintPixel *badpaintPixel = &canvas->rootBadpaintPixels.dataBadpaintPixel[i];
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

			for (u32 rectIndex = 0; rectIndex < canvas->rootBadpaintPixels.drawingRectCount; rectIndex++)
			{
				if (canvas->rootBadpaintPixels.drawingRectDirtyListFrame[rectIndex])
				{
					RectIV2 drawingRect = GetDrawingRectFromIndex(canvas->rootBadpaintPixels.dim, canvas->rootBadpaintPixels.drawingRectDim, rectIndex);
					GLintptr offset = (drawingRect.pos.y * textureGPU->dim.x + drawingRect.pos.x) * sizeof(ColorU32);
					glTexSubImage2D(GL_TEXTURE_2D, 0, drawingRect.pos.x, drawingRect.pos.y, drawingRect.dim.x, drawingRect.dim.y, GL_RGBA, GL_UNSIGNED_BYTE, (void*)offset);
					canvas->rootBadpaintPixels.drawingRectDirtyListFrame[rectIndex] = false;
				}
			}

			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			textureGPU->currentPboID = ModNextU32(textureGPU->currentPboID, ARRAY_COUNT(textureGPU->pboIDs));

			GenTextureMipmaps(&textureGPU->texture);
		}


		//----------------------------------------------------
		//----------------------RENDER------------------------
		//----------------------------------------------------

		UiRaylibProcessStrings(uiBufferCurrent);
		UiLayoutBlocks(uiBufferCurrent, frameState->windowDim, &gameMemory->temporaryArena);

#if DEBUG_MODE
		if (IsKeyDown(KEY_L))
		{
			for (u32 i = 1; i < uiBufferCurrent->uiBlockCount; i++)
			{
				uiBufferCurrent->uiBlocks[i].flags |= UI_FLAG_DRAW_BORDER;
				uiBufferCurrent->uiBlocks[i].uiBlockColors.borderColor = ColorU32{255, 255, 0, 255};
			}
		}
#endif

		BeginDrawing();

		ClearBackground(Color{192, 192, 192, 255});

		UiRaylibRenderBlocks(uiBufferCurrent);

#if DEBUG_MODE
		if (IsKeyDown(KEY_P))
		{
			for (u32 i = 1; i < uiBufferCurrent->uiBlockCount; i++)
			{
				if (uiBufferCurrent->uiBlocks[i].parent)
				{
					i32 startPosX = (i32) uiBufferCurrent->uiBlocks[i].parent->rect.pos.x;
					i32 startPosY = (i32) uiBufferCurrent->uiBlocks[i].parent->rect.pos.y;
					i32 endPosX = (i32) uiBufferCurrent->uiBlocks[i].rect.pos.x;
					i32 endPosY = (i32) uiBufferCurrent->uiBlocks[i].rect.pos.y;
					DrawLine(startPosX, startPosY, endPosX, endPosY, RED);
				}
			}
		}
#endif

		EndDrawing();

		ArenaReset(&gameMemory->temporaryArena);

		UiEndFrame(uiState);

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

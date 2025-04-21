
#include "base/base.h"

//NOTE: (Ahmayk) hack to allow raw openGL calls in our code 
#include "../includes/raylib/src/external/glad.h" // GLAD extensions loading library, includes OpenGL headers

#include "../includes/raylib/src/raylib.h"
#include "../includes//raylib//src/external/glfw/include/GLFW/glfw3.h"

#include "ui/ui_raylib.h"
#include "widgets.h"
#include "vn_math_external.h"
#include "platform_win32.h"
#include "main.h"

#include "assets/font.h"
#include "assets/defaultImage.h"

#include <cstring>  //memcpy

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
	KEY_P,
	KEY_E,
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

void RunApp(PlatformWorkQueue *threadWorkQueue, GameMemory gameMemory, unsigned int threadCount)
{
	AppState *appState = ARENA_PUSH_STRUCT(&gameMemory.permanentArena, AppState);
	ImageRawRGBA32 *rootImageRaw = ARENA_PUSH_STRUCT(&gameMemory.permanentArena, ImageRawRGBA32);

	Canvas *canvas = &appState->canvas;
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

	UiState *uiState = UiInit(&gameMemory.permanentArena);

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

	Font defaultFont = LoadFontFromMemory(".otf", PAINT_FONT_DATA, ARRAY_COUNT(PAINT_FONT_DATA), 12, 0, 0);
	appState->defaultUiFont.id = defaultFont.texture.id;
	appState->defaultUiFont.data = &defaultFont;

	appState->currentBrushEffect = BADPAINT_BRUSH_EFFECT_REMOVE;
	//appState->toolSize = 10;
	appState->toolSize = 50;
	appState->currentTool = BADPAINT_TOOL_PENCIL;

	v2 pressedMousePos = {};
	//TODO: (Ahmayk) Remove
	u32 draggedHash = {};

	Texture toolbrushSpriteSheet = LoadTexture("buttonSprites.png");
	i32 spriteSheetBoxSize = 30;
	for (i32 toolIndex = 0; toolIndex < BADPAINT_TOOL_COUNT; toolIndex++)
	{
		Tool *tool = &appState->tools[toolIndex];
		for (i32 interactionIndex = 0; interactionIndex < INTERACTION_STATE_COUNT; interactionIndex++)
		{
			tool->uiTextureViews[interactionIndex] = UiRaylibTextureToUiTextureView(&toolbrushSpriteSheet);
			tool->uiTextureViews[interactionIndex].viewRect.pos = iv2{spriteSheetBoxSize * interactionIndex, spriteSheetBoxSize * toolIndex};
			tool->uiTextureViews[interactionIndex].viewRect.dim = iv2{spriteSheetBoxSize, spriteSheetBoxSize};
		}
	}

	*rootImageRaw = LoadDataIntoRawImage(&DEFAULT_IMAGE_DATA[0], ARRAY_COUNT(DEFAULT_IMAGE_DATA), &gameMemory);
	if (rootImageRaw->dataSize)
	{
		InitializeNewImage(&gameMemory, rootImageRaw, canvas, &appState->loadedTexture, processedImages, threadCount);
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
				InitializeNewImage(&gameMemory, rootImageRaw, canvas, &appState->loadedTexture, processedImages, threadCount);
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

		AppCommandBuffer appCommandBuffer = {};
		appCommandBuffer.size = 50;
		appCommandBuffer.appCommands = ARENA_PUSH_ARRAY_MARKER(&gameMemory.temporaryArena, appCommandBuffer.size, AppCommand, &appCommandBuffer.arenaMarker);

		for (u32 i = 0; i < COMMAND_COUNT; i++)
		{
			if (IsCommandKeyBindingPressed((COMMAND) i))
			{
				AppCommand *appCommand = PushAppCommand(&appCommandBuffer);
				appCommand->command = (COMMAND) i;
			}
		}

		UiInteractionHashes uiInteractionHashes = {};

		UiBuffer *uiBufferLastFrame = &uiState->uiBuffers[1 - uiState->uiBufferIndex];
		for (u32 i = 0; i < uiBufferLastFrame->uiBlockCount; i++)
		{
			UiBlock *uiBlock = &uiBufferLastFrame->uiBlocks[i];
			if ((uiBlock->flags & UI_FLAG_INTERACTABLE) && ASSERT(uiBlock->hash))
			{
				if (IsInRectV2(mousePixelPos, uiBlock->rect))
				{
					uiInteractionHashes.hashMouseHover = uiBlock->hash;
					if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
					{
						uiInteractionHashes.hashMousePressed = uiBlock->hash;
						draggedHash = uiBlock->hash;
					}
					if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
					{
						uiInteractionHashes.hashMouseDown = uiBlock->hash;
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
		root->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, (f32) windowDim.x};
		root->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, (f32) windowDim.y};
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

					if (WidgetToolButton(uiState, appState, &uiInteractionHashes, BADPAINT_TOOL_PENCIL, COMMAND_SWITCH_TOOL_TO_PENCIL))
					{
						AppCommand *appCommand = PushAppCommand(&appCommandBuffer);
						appCommand->command = COMMAND_SWITCH_TOOL_TO_PENCIL;
					}
					if (WidgetToolButton(uiState, appState, &uiInteractionHashes, BADPAINT_TOOL_ERASER, COMMAND_SWITCH_TOOL_TO_ERASER))
					{
						AppCommand *appCommand = PushAppCommand(&appCommandBuffer);
						appCommand->command = COMMAND_SWITCH_TOOL_TO_ERASER;
					}

					{
						UiBlock *b = UiCreateBlock(uiState);
						b->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
						b->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, 50};
					}

					if (WidgetBrushEffectButton(uiState, appState, &uiInteractionHashes, BADPAINT_BRUSH_EFFECT_REMOVE, STRING("Rmv"), COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE))
					{
						AppCommand *appCommand = PushAppCommand(&appCommandBuffer);
						appCommand->command = COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE;
					}

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
						UiPanelPair panelPair1 = SplitPanel(&appState->rootUiPanel, &gameMemory.permanentArena, UI_AXIS_X, 0.85f);

						UiPanelPair panelPairImages = SplitPanel(panelPair1.uiPanel1, &gameMemory.permanentArena, UI_AXIS_X, 0.5f);
						panelPairImages.uiPanel1->uiPanelType = UI_PANEL_TYPE_FINAL_TEXTURE;
						panelPairImages.uiPanel2->uiPanelType = UI_PANEL_TYPE_CANVAS;

						UiPanelPair panelPairRightSidebar = SplitPanel(panelPair1.uiPanel2, &gameMemory.permanentArena, UI_AXIS_Y, 0.2f);
						panelPairRightSidebar.uiPanel1->uiPanelType = UI_PANEL_TYPE_NULL;
						panelPairRightSidebar.uiPanel2->uiPanelType = UI_PANEL_TYPE_LAYERS;
					}

					BuildPanelTree(uiState, appState, &uiInteractionHashes, &appState->rootUiPanel);
				}
			}
		}

		//----------------------------------------------------
		//-----------------------APP--------------------------
		//----------------------------------------------------

		for (u32 i = 0; i < appCommandBuffer.count; i++)
		{
			switch(appCommandBuffer.appCommands[i].command)
			{
				case COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE:
				{
					appState->currentBrushEffect = BADPAINT_BRUSH_EFFECT_REMOVE;
				} break;
				case COMMAND_SWITCH_BRUSH_EFFECT_TO_MAX:
				{
					appState->currentBrushEffect = BADPAINT_BRUSH_EFFECT_MAX;
				} break;
				case COMMAND_SWITCH_BRUSH_EFFECT_TO_SHIFT:
				{
					appState->currentBrushEffect = BADPAINT_BRUSH_EFFECT_SHIFT;
				} break;
				case COMMAND_SWITCH_BRUSH_EFFECT_TO_RANDOM:
				{
					appState->currentBrushEffect = BADPAINT_BRUSH_EFFECT_RANDOM;
				} break;
				case COMMAND_SWITCH_TOOL_TO_PENCIL:
				{
					appState->currentTool = BADPAINT_TOOL_PENCIL;
				} break;
				case COMMAND_SWITCH_TOOL_TO_ERASER:
				{
					appState->currentTool = BADPAINT_TOOL_ERASER;
				} break;
				case COMMAND_EXPORT_IMAGE:
				{
					if (appState->loadedTexture.height && appState->loadedTexture.width)
					{
						ArenaMarker arenaMarker = ArenaPushMarker(&gameMemory.temporaryArena);
						String filePath = AllocateString(256, &gameMemory.temporaryArena);
						u32 filepathLength;
						b32 success = GetPngImageFilePathFromUser(filePath.chars, filePath.length, &filepathLength);
						filePath.length = filepathLength;
						if (success && filePath.length)
						{
							Image exportImgae = LoadImageFromTexture(appState->loadedTexture);
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
				InvalidDefaultCase
			}
		}

		ArenaPopMarker(appCommandBuffer.arenaMarker);

		//TODO: (Ahmayk) painting now needs to be an app command generated form UI!
		UiBlock *canvasUiBlock = UiGetBlockOfHashLastFrame(uiState, 0);
		UiBlock *finalTextureUiBlock = UiGetBlockOfHashLastFrame(uiState, 0);

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
#if 0
		if (canvasUiBlock && canvasUiBlock->hash == uiInteractionHashes.hashMouseHover)
		{
			f32 scale = canvas->drawnImageData.dim.x / canvasUiBlock->rect.dim.x;
			cursorPosInDrawnImagePrevious = (mousePixelPos - RayVectorToV2(GetMouseDelta()) - canvasUiBlock->rect.pos) * scale;
			cursorPosInDrawnImage = (mousePixelPos - canvasUiBlock->rect.pos) * scale;
			isDownOnPaintable = canvasUiBlock->hash == uiInteractionHashes.hashMouseDown;
			isHoveredOnPaintable = true;
		}
		if (finalTextureUiBlock && finalTextureUiBlock->hash == uiInteractionHashes.hashMouseHover)
		{
			f32 scale = rootImageRaw->dim.x / finalTextureUiBlock->rect.dim.x;
			cursorPosInDrawnImagePrevious = (mousePixelPos - RayVectorToV2(GetMouseDelta()) - finalTextureUiBlock->rect.pos) * scale;
			cursorPosInDrawnImage = (mousePixelPos - finalTextureUiBlock->rect.pos) * scale;
			isDownOnPaintable = finalTextureUiBlock->hash == uiInteractionHashes.hashMouseDown;
			isHoveredOnPaintable = true;
		}
#endif

		if (!imageIsBroken && isDownOnPaintable)
		{
			b32 drewSomething = false;
			switch(appState->currentTool)
			{
				case BADPAINT_TOOL_PENCIL:
				{
					Color colorToPaint = {};
					colorToPaint.r = (u8) appState->currentBrushEffect;
					colorToPaint.g = (u8) RandomInRangeI32(0, 255);
					colorToPaint.a = (u8) canvas->processBatchIndex;
					iv2 startPosIV2;
					startPosIV2.x = (u32) RoundF32(cursorPosInDrawnImagePrevious.x);
					startPosIV2.y = (u32) RoundF32(cursorPosInDrawnImagePrevious.y);
					iv2 endPosIV2;
					endPosIV2.x = (u32) RoundF32(cursorPosInDrawnImage.x);
					endPosIV2.y = (u32) RoundF32(cursorPosInDrawnImage.y);
					drewSomething = CanvasDrawCircleStroke(canvas, startPosIV2, endPosIV2, appState->toolSize, colorToPaint);
				} break;
				case BADPAINT_TOOL_ERASER:
				{
					Color colorToPaint = {};
					colorToPaint.a = (u8) canvas->processBatchIndex;
					iv2 startPosIV2;
					startPosIV2.x = (u32) RoundF32(cursorPosInDrawnImagePrevious.x);
					startPosIV2.y = (u32) RoundF32(cursorPosInDrawnImagePrevious.y);
					iv2 endPosIV2;
					endPosIV2.x = (u32) RoundF32(cursorPosInDrawnImage.x);
					endPosIV2.y = (u32) RoundF32(cursorPosInDrawnImage.y);
					drewSomething = CanvasDrawCircleStroke(canvas, startPosIV2, endPosIV2, appState->toolSize, colorToPaint);
				} break;
			}

			if (drewSomething)
			{
				canvas->proccessAsap = true;
				canvas->saveRollbackOnNextPress = true;
				canvas->dataOnCanvas = true;
			}
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

		//TODO: (Ahmayk) turn into app commands
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
			ColorU32 *pixels = (ColorU32 *) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
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
								ColorU32 canvasPixel = ((ColorU32 *)canvas->drawnImageData.dataU8)[i];
								ColorU32 *outPixel = (pixels + i);
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
					GLintptr offset = (drawingRect.pos.y * canvas->textureDrawing.width + drawingRect.pos.x) * sizeof(ColorU32);
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
		//----------------------RENDER------------------------
		//----------------------------------------------------

		UiRaylibProcessStrings(uiBufferCurrent);
		UiLayoutBlocks(uiBufferCurrent, windowDim, &gameMemory.temporaryArena);

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

#if 0
		if (isHoveredOnPaintable)
		{
			v2 normalizedRelativePos = cursorPosInDrawnImage / canvas->drawnImageData.dim;
			if (canvasUiBlock)
			{
				v2 hoverPos = canvasUiBlock->rect.pos + (canvasUiBlock->rect.dim * normalizedRelativePos);
				Color hoverCursorColor = ColorU32ToRayColor( BRUSH_EFFECT_COLORS_PROCESSING[appState->currentBrushEffect]);
				f32 size = appState->toolSize * SafeDivideF32(canvasUiBlock->rect.dim.x, (f32) canvas->drawnImageData.dim.x);
				DrawCircle((i32)hoverPos.x, (i32)hoverPos.y, size, hoverCursorColor);
			}
			if (finalTextureUiBlock)
			{
				v2 hoverPos = finalTextureUiBlock->rect.pos + (finalTextureUiBlock->rect.dim * normalizedRelativePos);
				Color outlineColor = Color{0, 0, 0, 100};
				f32 size = appState->toolSize * SafeDivideF32(finalTextureUiBlock->rect.dim.x, (f32) canvas->drawnImageData.dim.x);
				DrawCircleLines((i32)hoverPos.x, (i32)hoverPos.y, size, outlineColor);
			}
		}
#endif

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

		ArenaReset(&gameMemory.temporaryArena);

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

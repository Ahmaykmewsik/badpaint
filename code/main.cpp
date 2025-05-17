
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

//NOTE: (Ahmayk) This needs to be redesigned to include key modifiers 
static KeyboardKey COMMAND_KEY_BINDINGS[] = {
	KEY_NULL,
	KEY_NULL,
	KEY_R,
	KEY_A,
	KEY_S,
	KEY_N,
	KEY_C,
	KEY_B,
	KEY_E,
	KEY_Y,
	KEY_T,
	KEY_NULL,
	KEY_NULL,
	KEY_NULL,
	KEY_NULL,
};

b32 IsCommandKeyBindingDown(u32 command)
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

b32 IsCommandKeyBindingPressed(u32 command)
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

	appState->processedImageCount = threadCount; 
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
		InitializeNewImage(gameMemory, appState);
	}

	return appState;
}

void BuildUi(UiState *uiState, AppState *appState, AppCommandBuffer *appCommandBuffer, GameMemory *gameMemory)
{
	UiResetCurrentUiBuffer(uiState);

	UiBlockColors defaultBlockColors = {};
	defaultBlockColors.backColor = ColorU32{191, 191, 191, 255};
	defaultBlockColors.frontColor = COLORU32_BLACK;
	defaultBlockColors.borderColor = COLORU32_DARKGRAY;

	UiBlock defaultStyleBlock = {};
	defaultStyleBlock.uiBlockColors.backColor = APP_MAIN_BACKGROUND_COLOR;
	defaultStyleBlock.uiBlockColors.borderColor = COLORU32_GRAY;
	defaultStyleBlock.uiBlockColors.frontColor = COLORU32_BLACK;
	defaultStyleBlock.uiFont = appState->defaultUiFont;

	UiBlock *root= UiCreateBlock(uiState);
	root->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, (f32) uiState->uiInteractionState.uiInteractionFrameInput.windowDim.x};
	root->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, (f32) uiState->uiInteractionState.uiInteractionFrameInput.windowDim.y};
	root->uiChildLayoutType = UI_CHILD_LAYOUT_TOP_TO_BOTTOM;
	UI_PARENT_SCOPE(uiState, root)
	{
		static MenuBarState menuBarState = {};
		UiBlock *menuBar = WidgetMenuBar(uiState, &menuBarState, Murmur3String("TitleBarMenu"));
		UI_PARENT_SCOPE(uiState, menuBar)
		{
			UiBlock menuBarButtonStyleBlock = defaultStyleBlock; 
			menuBarButtonStyleBlock.padding = UiPadding{6, 6, 2, 0};

			UiBlock optionButtonStyle = defaultStyleBlock; 
			optionButtonStyle.padding = UiPadding{6, 6, 3, 1};

			{
				UiBlock *dropdownButton = WidgetMenuBarButton(uiState, &menuBarState, STRING("File"), &menuBarButtonStyleBlock);
				dropdownButton->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW;
				if (menuBarState.hashOpenMenuBarButton == dropdownButton->hash)
				{
					UI_PARENT_SCOPE(uiState, dropdownButton)
					{
						UiBlock *menuPanel = WidgetMenuPanel(uiState, &defaultStyleBlock);
						menuPanel->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 1;
						UI_PARENT_SCOPE(uiState, menuPanel)
						{
							UiBlock *menu = WidgetMenuOptionButton(uiState, &menuBarState, STRING("Import Image..."), menuPanel->hash, appCommandBuffer, COMMAND_OPEN_IMPORT_DIALOGUE, &optionButtonStyle);
							menu->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 2;
							menu->firstChild->next->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 2;

							menu = WidgetMenuOptionButton(uiState, &menuBarState, STRING("Export Image..."), menuPanel->hash,  appCommandBuffer, COMMAND_EXPORT_IMAGE, &optionButtonStyle);
							menu->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 3;
							menu->firstChild->next->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 4;
						}
					}
				}
			}
			{
				UiBlock *dropdownButton = WidgetMenuBarButton(uiState, &menuBarState, STRING("Edit"), &menuBarButtonStyleBlock);
				dropdownButton->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW;
				if (menuBarState.hashOpenMenuBarButton == dropdownButton->hash)
				{
					UI_PARENT_SCOPE(uiState, dropdownButton)
					{
						UiBlock *menuPanel = WidgetMenuPanel(uiState, &defaultStyleBlock);
						menuPanel->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 1;
						UI_PARENT_SCOPE(uiState, menuPanel)
						{
							UiBlock *menu = WidgetMenuOptionButton(uiState, &menuBarState, STRING("Undo"), menuPanel->hash, appCommandBuffer, COMMAND_UNIMPLEMENTED, &optionButtonStyle);
							menu->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 2;
							menu->firstChild->next->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 2;

							menu = WidgetMenuOptionButton(uiState, &menuBarState, STRING("Redo"), menuPanel->hash,  appCommandBuffer, COMMAND_UNIMPLEMENTED, &optionButtonStyle);
							menu->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 3;
							menu->firstChild->next->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 4;

							UiBlock *s = WidgetSeperatorX(uiState, 1, COLORU32_GRAY, 4);
							s->firstChild->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 100;

							menu = WidgetMenuOptionButton(uiState, &menuBarState, STRING("Commit Felony"), menuPanel->hash, appCommandBuffer, COMMAND_UNIMPLEMENTED, &optionButtonStyle);
							menu->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 3;
							menu->firstChild->next->depthLayer = UI_APP_DEPTH_HOVERING_WINDOW + 4;
						}
					}
				}
			}

		}

		UiBlock *body = UiCreateBlock(uiState);
		body->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
		body->uiSizes[UI_AXIS_Y] = {UI_SIZE_FILL};
		UI_PARENT_SCOPE(uiState, body)
		{
			UiBlock *toolbar = UiCreateBlock(uiState);
			toolbar->flags = UI_FLAG_DRAW_BORDER;
			toolbar->uiSizes[UI_AXIS_X] = {UI_SIZE_FIT_CHILDREN};
			toolbar->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
			toolbar->uiChildLayoutType = UI_CHILD_LAYOUT_TOP_TO_BOTTOM;
			toolbar->uiChildAlignTypes[UI_AXIS_X] = UI_CHILD_ALIGN_CENTER;
			toolbar->uiBlockColors.borderColor = ColorU32{100, 100, 100, 100};
			UI_PARENT_SCOPE(uiState, toolbar)
			{
				{
					UiBlock *b = UiCreateBlock(uiState);
					b->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
					b->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, 5};
				}

				WidgetToolButton(uiState, appState, appCommandBuffer, STRING("P"), BADPAINT_TOOL_PENCIL, COMMAND_SWITCH_TOOL_TO_PENCIL);
				WidgetToolButton(uiState, appState, appCommandBuffer, STRING("E"), BADPAINT_TOOL_ERASER, COMMAND_SWITCH_TOOL_TO_ERASER);
				WidgetToolButton(uiState, appState, appCommandBuffer, STRING("Y"), BADPAINT_TOOL_SPRAYCAN, COMMAND_SWITCH_TOOL_TO_SPAYCAN);

				{
					UiBlock *b = UiCreateBlock(uiState);
					b->uiSizes[UI_AXIS_X] = {UI_SIZE_PERCENT_OF_PARENT, 1};
					b->uiSizes[UI_AXIS_Y] = {UI_SIZE_PIXELS, 50};
				}

				WidgetBrushEffectButton(uiState, appState, appCommandBuffer, BADPAINT_PIXEL_TYPE_REMOVE, STRING("Remv"), COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_REMOVE);
				WidgetBrushEffectButton(uiState, appState, appCommandBuffer, BADPAINT_PIXEL_TYPE_MAX, STRING("Max"), COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_MAX);
				WidgetBrushEffectButton(uiState, appState, appCommandBuffer, BADPAINT_PIXEL_TYPE_SHIFT, STRING("Sft"), COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_SHIFT);
				WidgetBrushEffectButton(uiState, appState, appCommandBuffer, BADPAINT_PIXEL_TYPE_RANDOM, STRING("Rand"), COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_RANDOM);
				WidgetBrushEffectButton(uiState, appState, appCommandBuffer, BADPAINT_PIXEL_TYPE_COPY_OTHER_PIXEL, STRING("Copy"), COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_COPY_OTHER_PIXEL);

#if 0
				{
					UiBlock *seperator = UiCreateBlock(uiState);
					seperator->uiPosition[UI_AXIS_X] = {UI_POSITION_PERCENT_OF_PARENT, 1};
					seperator->uiPosition[UI_AXIS_Y] = {UI_POSITION_RELATIVE, 0};
					seperator->flags = UI_FLAG_DRAW_LINE_BOTTOMLEFT_TOPRIGHT;
					seperator->uiSizes[UI_AXIS_X] = {UI_SIZE_PIXELS, 4};
					seperator->uiSizes[UI_AXIS_Y] = {UI_SIZE_PERCENT_OF_PARENT, 1};
					seperator->uiBlockColors.frontColor = COLORU32_BLACK;
				}
#endif
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
#if 1
					appState->rootUiPanel.childSplitAxis = UI_AXIS_X;
					appState->rootUiPanel.hash = Murmur3String("mainPanels");
					UiPanelPair panelPair1 = SplitPanel(&appState->rootUiPanel, &gameMemory->permanentArena, UI_AXIS_X, 0.85f);

					UiPanelPair panelPairTopBottom = SplitPanel(panelPair1.uiPanel1, &gameMemory->permanentArena, UI_AXIS_X, 0.3333f);
					panelPairTopBottom.uiPanel1->uiPanelType = UI_PANEL_TYPE_ROOT_IMAGE;
					UiPanelPair panelPairDrawing = SplitPanel(panelPairTopBottom.uiPanel2, &gameMemory->permanentArena, UI_AXIS_X, 0.5);
					panelPairDrawing.uiPanel1->uiPanelType = UI_PANEL_TYPE_PNG_FILTERED;
					panelPairDrawing.uiPanel2->uiPanelType = UI_PANEL_TYPE_FINAL_IMAGE;

					UiPanelPair panelPairRightSidebar = SplitPanel(panelPair1.uiPanel2, &gameMemory->permanentArena, UI_AXIS_Y, 0.2f);
					panelPairRightSidebar.uiPanel1->uiPanelType = UI_PANEL_TYPE_NULL;
					panelPairRightSidebar.uiPanel2->uiPanelType = UI_PANEL_TYPE_LAYERS;
#else
					appState->rootUiPanel.childSplitAxis = UI_AXIS_X;
					appState->rootUiPanel.hash = Murmur3String("mainPanels");
					UiPanelPair panelPair1 = SplitPanel(&appState->rootUiPanel, &gameMemory->permanentArena, UI_AXIS_X, 0.85f);
					panelPair1.uiPanel1->uiPanelType = UI_PANEL_TYPE_NULL;
					panelPair1.uiPanel2->uiPanelType = UI_PANEL_TYPE_LAYERS;
#endif
				}

				BuildPanelTree(uiState, appState, appCommandBuffer, &appState->rootUiPanel);
			}
		}
	}
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

		UiInteractionFrameInput uiInteractionFrameInput = {};
		uiInteractionFrameInput.windowDim.x = GetScreenWidth();
		uiInteractionFrameInput.windowDim.y = GetScreenHeight();
		uiInteractionFrameInput.mousePixelPos = iv2{GetMouseX(), GetMouseY()};
		uiInteractionFrameInput.isMouseLeftDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
		uiInteractionFrameInput.isMouseLeftPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
		uiInteractionFrameInput.isMouseLeftReleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
		UiInteractionStateUpdate(uiState, &uiInteractionFrameInput);

		//NOTE: (Ahmayk) this nonsense is so that we can make a pointer to a variable on the stack
		//having this be a pointer makes code refernecing this easier to maintain when we move code from here to elsewhere
		AppCommandBuffer _appCommandBuffer = {};
		AppCommandBuffer *appCommandBuffer = &_appCommandBuffer;
		appCommandBuffer->size = 50;
		appCommandBuffer->appCommands = ARENA_PUSH_ARRAY_MARKER(&gameMemory->temporaryArena, appCommandBuffer->size, AppCommand, {});

		for (u32 i = 0; i < COMMAND_COUNT; i++)
		{
			if (IsCommandKeyBindingPressed((COMMAND) i))
			{
				AppCommand *appCommand = PushAppCommand(appCommandBuffer);
				appCommand->command = (COMMAND) i;
			}
		}

		if (IsFileDropped())
		{
			FilePathList droppedFiles = LoadDroppedFiles();
			AppCommand *appCommand = PushAppCommand(appCommandBuffer);
			appCommand->command = COMMAND_IMPORT_FILE_PATH;
			appCommand->value1String = CreateString(droppedFiles.paths[0], StringArena());
			UnloadDroppedFiles(droppedFiles);
		}

		//----------------------------------------------------
		//------------------------UI--------------------------
		//----------------------------------------------------

		BuildUi(uiState, appState, appCommandBuffer, gameMemory);

		//----------------------------------------------------
		//-----------------------APP--------------------------
		//----------------------------------------------------

		b32 uiNeedsRebuild = false;
		Canvas *canvas = &appState->canvas;
		for (u32 commandIndex = 0; commandIndex < appCommandBuffer->count; commandIndex++)
		{
			AppCommand *appCommand = &appCommandBuffer->appCommands[commandIndex];
			switch(appCommand->command)
			{
				case COMMAND_UNIMPLEMENTED:
				{
					//TODO: (Ahmayk) put something on the screen
				} break;
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
				case COMMAND_SWITCH_BADPAINT_PIXEL_TYPE_TO_COPY_OTHER_PIXEL:
				{
					appState->currentBadpaintPixelType = BADPAINT_PIXEL_TYPE_COPY_OTHER_PIXEL;
				} break;
				case COMMAND_SWITCH_TOOL_TO_PENCIL:
				{
					appState->currentTool = BADPAINT_TOOL_PENCIL;
				} break;
				case COMMAND_SWITCH_TOOL_TO_ERASER:
				{
					appState->currentTool = BADPAINT_TOOL_ERASER;
				} break;
				case COMMAND_SWITCH_TOOL_TO_SPAYCAN:
				{
					appState->currentTool = BADPAINT_TOOL_SPRAYCAN;
				} break;
				case COMMAND_SWITCH_TOOL_TO_TEST:
				{
					appState->currentTool = BADPAINT_TOOL_TEST;
				} break;
				case COMMAND_OPEN_IMPORT_DIALOGUE:
				{
					//TODO, use platform api
				} break;
				case COMMAND_IMPORT_FILE_PATH:
				{
					ArenaMarker loadMarker = ArenaPushMarker(&gameMemory->temporaryArena);
					unsigned int fileSize = {};
					u8 *fileData = LoadDataFromDisk(appCommand->value1String, &fileSize, &gameMemory->temporaryArena);
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

					if (appState->rootImageRaw.dataSize)
					{
						InitializeNewImage(gameMemory, appState);
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
					ArenaPopMarker(loadMarker);

					uiNeedsRebuild = true;
				} break;
				case COMMAND_EXPORT_IMAGE:
				{
					if (canvas->textureGPUFinal.texture.id)
					{
						ArenaMarker arenaMarker = ArenaPushMarker(&gameMemory->temporaryArena);
						String filePath = AllocateString(256, &gameMemory->temporaryArena);
						u32 filepathLength;
						b32 success = GetPngImageFilePathFromUser(filePath.chars, filePath.length, &filepathLength);
						filePath.length = filepathLength;
						if (success && filePath.length)
						{
							Image exportImgae = LoadImageFromTexture(canvas->textureGPUFinal.texture);
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

						UI_PANEL_TYPE uiPanelType = (UI_PANEL_TYPE) appCommand->value3U32;
						ImageBadpaintPixels *imageBadpaintPixels = {};
						switch(uiPanelType)
						{
							case UI_PANEL_TYPE_ROOT_IMAGE: imageBadpaintPixels = &canvas->badpaintPixelsRootImage; break;
							case UI_PANEL_TYPE_PNG_FILTERED: imageBadpaintPixels = &canvas->badpaintPixelsPNGFiltered; break;
							case UI_PANEL_TYPE_FINAL_IMAGE: imageBadpaintPixels = &canvas->badpaintPixelsFinalImage; break;
							InvalidDefaultCase;
						};

						if (imageBadpaintPixels)
						{
							switch(appState->currentTool)
							{
								case BADPAINT_TOOL_PENCIL:
								{
									BadpaintPixel badpaintPixel = CreateBadpaintPixel(appState->currentBadpaintPixelType, imageBadpaintPixels, endPosIV2, appState->toolSize, canvas->processBatchIndex);
									drewSomething = CanvasDrawCircleStroke(imageBadpaintPixels, startPosIV2, endPosIV2, appState->toolSize, &badpaintPixel);
								} break;
								case BADPAINT_TOOL_ERASER:
								{
									BadpaintPixel badpaintPixel = CreateBadpaintPixel(BADPAINT_PIXEL_TYPE_NONE, imageBadpaintPixels, endPosIV2, appState->toolSize, canvas->processBatchIndex);
									drewSomething = CanvasDrawCircleStroke(imageBadpaintPixels, startPosIV2, endPosIV2, appState->toolSize, &badpaintPixel);
								} break;
								case BADPAINT_TOOL_SPRAYCAN:
								{
									BadpaintPixel badpaintPixel = CreateBadpaintPixel(appState->currentBadpaintPixelType, imageBadpaintPixels, endPosIV2, appState->toolSize, canvas->processBatchIndex);
									drewSomething = CanvasDrawSpray(imageBadpaintPixels, endPosIV2, appState->toolSize, &badpaintPixel);
								}
								case BADPAINT_TOOL_TEST:
								{
								} break;
								InvalidDefaultCase
							}
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

		if (uiNeedsRebuild)
		{
			BuildUi(uiState, appState, appCommandBuffer, gameMemory);
			//NOTE: (Ahmayk) appCommands may be added onto the frameState, but we ignore them since we've already built this once
		}

		//ArenaPopMarker(frameState->appCommandBuffer.arenaMarker);

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
				Arena *arenaRoot = ArenaGroupPushArena(&gameMemory->conversionArenaGroup);
				Arena *arenaFiltered = ArenaGroupPushArena(&gameMemory->conversionArenaGroup);
				Arena *arenaVisualized = ArenaGroupPushArena(&gameMemory->conversionArenaGroup);
				Arena *arenaFinal = ArenaGroupPushArena(&gameMemory->conversionArenaGroup);
				if (arenaFiltered && arenaVisualized && arenaFinal)
				{
					processedImage->arenaRoot = arenaRoot;
					processedImage->arenaFiltered = arenaFiltered;
					processedImage->arenaVisualized = arenaVisualized;
					processedImage->arenaFinal = arenaFinal;
					memcpy(processedImage->dirtyRectsInProcess, canvas->badpaintPixelsRootImage.drawingRectDirtyListProcess, canvas->badpaintPixelsRootImage.drawingRectCount * sizeof(b32));
					canvas->proccessAsap = false;
					processedImage->active = true;
					processedImage->processBatchIndex = canvas->processBatchIndex;
					canvas->processBatchIndex++;
					//NOTE: (Ahmayk) skip 0 as this represents already processed 
					if (canvas->processBatchIndex == 0)
					{
						canvas->processBatchIndex++;
					}
					memset(canvas->badpaintPixelsRootImage.drawingRectDirtyListProcess, 0, canvas->badpaintPixelsRootImage.drawingRectCount * sizeof(b32));
					PlatformAddThreadWorkEntry(threadWorkQueue, ProcessImageOnThread, (void *)processedImage);
					// Print("Queueing Work on thread " + IntToString(processedImage->index));
				}
				else
				{
					ArenaResetAndMarkAsReadyForAssignment(arenaRoot);
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
				for (u32 rectIndex = 0; rectIndex < canvas->badpaintPixelsRootImage.drawingRectCount; rectIndex++)
				{
					if (processedImageOfIndex->dirtyRectsInProcess[rectIndex])
					{
						canvas->badpaintPixelsRootImage.drawingRectDirtyListFrame[rectIndex] = true;
						RectIV2 drawingRect = GetDrawingRectFromIndex(canvas->badpaintPixelsRootImage.dim, canvas->badpaintPixelsRootImage.drawingRectDim, rectIndex);
						u32 pixelIndex = 0;
						u32 startY = drawingRect.pos.y;
						u32 endY = startY + drawingRect.dim.y;
						for (u32 y = startY; y < endY; y++)
						{
							u32 startIndex = (canvas->badpaintPixelsRootImage.dim.x * y) + drawingRect.pos.x;
							u32 endIndex = startIndex + drawingRect.dim.x;
							for (u32 i = startIndex; i < endIndex; i++)
							{
								BadpaintPixel *badpaintPixel = &canvas->badpaintPixelsRootImage.dataBadpaintPixel[i];
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
					}
				}

				if (atLeastOneDirtyRect)
				{
					TextureGPU *textureGPU = &canvas->textureGPUFinal;
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, textureGPU->pboIDs[textureGPU->currentPboID]);
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

					glBindTexture(GL_TEXTURE_2D, canvas->textureGPUFinal.texture.id);
					glPixelStorei(GL_UNPACK_ROW_LENGTH, canvas->textureGPUFinal.dim.x);
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
					textureGPU->currentPboID = ModNextU32(textureGPU->currentPboID, ARRAY_COUNT(textureGPU->pboIDs));

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
				RectIV2 rect = {};
				rect.dim = visualizedImage->dim;
				UpdateRectInTexture(&canvas->textureGPUPNGFiltered.texture, visualizedImage->dataU8, rect);
				UpdateRectInTexture(&canvas->textureGPURoot.texture, latestCompletedProcessedImage->rootImageRawProcessed.dataU8, rect);
			}

			ResetProcessedImage(latestCompletedProcessedImage, canvas);
		}

		RenderAndUploadBadpaintPixelImage(&canvas->badpaintPixelsRootImage);
		RenderAndUploadBadpaintPixelImage(&canvas->badpaintPixelsPNGFiltered);
		RenderAndUploadBadpaintPixelImage(&canvas->badpaintPixelsFinalImage);


		//----------------------------------------------------
		//----------------------RENDER------------------------
		//----------------------------------------------------

		UiBuffer *uiBufferCurrent = &uiState->uiBuffers[uiState->uiBufferIndex];

		UiRaylibSetCursor(uiState->uiInteractionState.currentUiCursorType);
		UiRaylibProcessStrings(uiBufferCurrent);
		UiLayoutBlocks(uiBufferCurrent, uiState->uiInteractionState.uiInteractionFrameInput.windowDim, &gameMemory->temporaryArena);

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


#include "headers.h"

int main(void)
{
    GameMemory gameMemory = {};

    InitializeArena(&gameMemory.permanentArena, Megabytes(100));
    InitializeArena(&gameMemory.temporaryArena, Megabytes(1000));
    InitializeArena(&gameMemory.rootImageArena, Megabytes(50));
    InitializeArena(&gameMemory.canvasArena, Megabytes(50));
    InitializeArena(&gameMemory.circularScratchBuffer, Megabytes(1000), true);

    InitializeArena(&gameMemory.twoFrameArenaModIndex0, Megabytes(100));
    InitializeArena(&gameMemory.twoFrameArenaModIndex1, Megabytes(100));

    GameState *gameState = PushStruct(&gameMemory.permanentArena, GameState);
    G_STRING_TEMP_MEM_ARENA = &gameMemory.temporaryArena;
    _G_TEMPORARY_ARENA_DONT_FUCKING_USE_THIS_EXCEPT_IN_A_MACRO = &gameMemory.temporaryArena;
    G_UI_INPUTS = PushStruct(&gameMemory.permanentArena, UiInputs);
    G_UI_STATE = PushStruct(&gameMemory.permanentArena, UiState);
    G_UI_HASH_TAG_STRING = CreateStringOnArena("##", &gameMemory.permanentArena);

    SetupThreads(&gameMemory);

    SetTargetFPS(60);

    SetTraceLogLevel(RL_LOG_NONE);

    InitWindow(200, 200, "badpaint");

    SetWindowState(FLAG_WINDOW_RESIZABLE);

    V2 screenDim = V2{(float)GetMonitorWidth(0), (float)GetMonitorHeight(0)};

    gameState->windowDim = screenDim * 0.8f;
    V2 windowDim = gameState->windowDim;

    V2 windowPosMiddle = PositionInCenter(screenDim, windowDim);
    SetWindowPosition(windowPosMiddle.x, windowPosMiddle.y);
    SetWindowSize(windowDim.x, windowDim.y);

    Font defaultFont = LoadFontEx("./assets/W95FA.otf", 18, 0, 0);
    Font bigFont = LoadFontEx("./assets/W95FA.otf", 48, 0, 0);

    Canvas canvas = {};

    Color brushColor = BLACK;
    float brushSize = 5;

    BpImage rootBpImage = {};
    Texture loadedTexture = {};

    stbi_write_force_png_filter = 5;

    // BpImage savedImage = {};

    //NOTE: DEVELOPER HACK
    {
        // rootBpImage = LoadDataIntoRawImage("./assets/handmadelogo.png", &gameMemory);
        // UploadAndReplaceTexture(&rootBpImage, &loadedTexture, &gameMemory.temporaryArena);
        // InitializeCanvas(&canvasImage, &canvasTexture, &rootBpImage, &gameMemory.temporaryArena);
    }

    while (!WindowShouldClose())
    {
        G_UI_STATE->twoFrameArenaLastFrame = GetTwoFrameArenaLastFrame(&gameMemory);
        G_UI_STATE->twoFrameArenaThisFrame = GetTwoFrameArenaThisFrame(&gameMemory);
        int uiBoxArrayIndex = GetFrameModIndexLastFrame();

        gameState->windowDim = WidthHeightToV2(GetScreenWidth(), GetScreenHeight());

        V2 mousePixelPos = V2{(float)GetMouseX(), (float)GetMouseY()};

        for (int i = 0;
             i < G_UI_STATE->uiBoxCount;
             i++)
        {
            UiBox *uiBox = &G_UI_STATE->uiBoxes[uiBoxArrayIndex][i];
            if (IsFlag(uiBox, UI_FLAG_INTERACTABLE))
            {
                uiBox->cursorRelativePixelPos = mousePixelPos - uiBox->rect.pos;

                if (IsInRect2D(mousePixelPos, uiBox->rect))
                {
                    uiBox->hovered = true;
                    uiBox->pressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
                    uiBox->down = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
                }
            }
        }

        if (IsKeyPressed(KEY_E))
        {
            brushColor = WHITE;
            brushSize = 20;
        }
        if (IsKeyPressed(KEY_D))
        {
            brushColor = BLACK;
            brushSize = 5;
        }

        String canvasStringKey = CreateString(G_CANVAS_STRING_TAG_CHARS);
        UiBox *canvasUiBox = GetUiBoxLastFrameOfStringKey(canvasStringKey);
        if (canvasUiBox && canvasUiBox->down)
        {
            float scale = Max(1, canvas.image.width / canvasUiBox->rect.dim.x);
            Print(scale);

#if 0
            V2 startPos = scale * (mousePixelPos - RayVectorToV2(GetMouseDelta()) - canvasUiBox->rect.pos);
            V2 endPos = scale * (mousePixelPos - canvasUiBox->rect.pos);

            ImageDrawLine(&canvas.image, startPos.x, startPos.y, endPos.x, endPos.y, BLACK);
            UpdateTexture(&canvas.image, &canvas.texture);
#endif

            V2 pos = scale * (mousePixelPos - canvasUiBox->rect.pos);
            ImageDrawCircle(&canvas.image, pos.x, pos.y, brushSize, brushColor);
            UpdateTexture(&canvas.image, &canvas.texture);
        }

        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();
            char *fileName = droppedFiles.paths[0];

            rootBpImage = LoadDataIntoRawImage(fileName, &gameMemory);
            if (rootBpImage.data)
            {
                UploadAndReplaceTexture(&rootBpImage, &loadedTexture, &gameMemory.temporaryArena);
                InitializeCanvas(&canvas, &rootBpImage, &gameMemory);
            }

            UnloadDroppedFiles(droppedFiles);
        }

        if (IsKeyPressed(KEY_ONE))
            stbi_write_force_png_filter = 1;
        if (IsKeyPressed(KEY_TWO))
            stbi_write_force_png_filter = 2;
        if (IsKeyPressed(KEY_THREE))
            stbi_write_force_png_filter = 3;
        if (IsKeyPressed(KEY_FOUR))
            stbi_write_force_png_filter = 4;
        if (IsKeyPressed(KEY_FIVE))
            stbi_write_force_png_filter = 5;

        if (rootBpImage.data)
        {
            BpImage tempImage = MakeBpImageCopy(&rootBpImage, &gameMemory.temporaryArena);

            ConvertNewBpImage(&tempImage, IMAGE_FORMAT_PNG_FILTERED, &gameMemory.temporaryArena);

            Assert(canvas.image.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

            for (int i = 0;
                 i < tempImage.dataSize;
                 i++)
            {
                Assert(i < canvas.image.height * canvas.image.width * 4);
                Color canvasPixel = ((Color *)canvas.image.data)[i];

                if (canvasPixel == BLACK)
                {
                    ((unsigned char *)tempImage.data)[i] = 0;
                }
            }

#if 0
                for (int i = 0;
                     i < 100;
                     i++)
                {
                    unsigned int pos = RandomInRangeInt(0, tempImage.dataSize);
                    ((unsigned char *)tempImage.data)[pos] += RandomInRangeInt(0, 100000);
                }
#endif

            UploadAndReplaceTexture(&tempImage, &loadedTexture, &gameMemory.temporaryArena);
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

        SetUiAxis({UI_SIZE_KIND_PIXELS, gameState->windowDim.x}, {UI_SIZE_KIND_PIXELS, gameState->windowDim.y});
        CreateUiBox();
        UiParent()
        {
            float titleBarHeight = 20;

            uiSettings->frontColor = BLACK;
            uiSettings->backColor = Color{191, 191, 191, 255};
            uiSettings->borderColor = DARKGRAY;
            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, titleBarHeight});
            CreateUiBox(UI_FLAG_DRAW_BACKGROUND, CreateString("Menu up here eventually"));
            UiParent()
            {
                SetUiAxis({UI_SIZE_KIND_TEXT}, {UI_SIZE_KIND_TEXT});
                String fps = CreateString("FPS: ") + GetFPS();
                CreateUiBox(UI_FLAG_DRAW_TEXT, fps);
            }

            SetUiAxis({UI_SIZE_KIND_PIXELS, gameState->windowDim.x}, {UI_SIZE_KIND_PIXELS, gameState->windowDim.y - titleBarHeight});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
                SetUiAxis({UI_SIZE_KIND_PIXELS, 80}, {UI_SIZE_KIND_PIXELS, 600});
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND);

                SetUiAxis({UI_SIZE_KIND_PIXELS, gameState->windowDim.x - 20}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 1});
                CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
                UiParent()
                {
                    SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 0.5}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 1});
                    CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER);
                    UiParent()
                    {
                        if (loadedTexture.id)
                        {
                            G_UI_INPUTS->texture = loadedTexture;
                            SetUiAxis({UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT}, {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT});
                            CreateUiBox(UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT);
                        }
                        else
                        {
                            String string = CreateString("Drop any file into the window for editing.");
                            SetUiAxis({UI_SIZE_KIND_TEXT, 1}, {UI_SIZE_KIND_TEXT, 1});
                            CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_CENTER_IN_PARENT, string);
                        }
                    }
                    SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 0.5}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 1});
                    CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER);
                    UiParent()
                    {
                        if (canvas.image.data)
                        {
                            G_UI_INPUTS->texture = canvas.texture;
                            SetUiAxis({UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT}, {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT});
                            CreateUiBox(UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT | UI_FLAG_INTERACTABLE, G_UI_HASH_TAG_STRING + G_CANVAS_STRING_TAG_CHARS);
                        }
                    }
                }
            }
        }

        uiSettings->frontColor = GREEN;
        uiSettings->font = bigFont;
        SetUiAxis({UI_SIZE_KIND_TEXT}, {UI_SIZE_KIND_TEXT});
        G_UI_INPUTS->pixelPosition = V2{0, 900};
        String label = CreateString("PNG Filter: ") + G_PNG_FILTER_NAMES[stbi_write_force_png_filter];
        CreateUiBox(UI_FLAG_DRAW_TEXT, label);

        int uiBoxArrayIndexThisFrame = GetFrameModIndexThisFrame();

        if (G_UI_STATE->uiBoxCount)
        {
            for (int i = 1;
                 i < G_UI_STATE->uiBoxCount;
                 i++)
            {
                UiBox *uiBox = &G_UI_STATE->uiBoxes[uiBoxArrayIndexThisFrame][i];

                for (int j = 0;
                     j < ArrayCount(uiBox->uiSettings.uiSizes);
                     j++)
                {
                    UiSize uiSize = uiBox->uiSettings.uiSizes[j];
                    switch (uiSize.kind)
                    {
                    case UI_SIZE_KIND_TEXTURE:
                    {
                        V2 dim = GetTextureDim(uiBox->uiInputs.texture);
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
                    RenderUiEntries(gameState, uiBox);
                }
            }
        }
#if 0
        if (!loadedImage.data)
        {
            DrawText("Drop any file into this window for editing.", 100, 40, 20, DARKGRAY);
        }
        else
        {
            DrawTextureEx(loadedTexture, {0, 0}, 0, 1, WHITE);
        }
#endif

        EndDrawing();

        ResetMemoryArena(&gameMemory.temporaryArena);
        MemoryArena *memoryArenaLastFrame = GetTwoFrameArenaLastFrame(&gameMemory);
        ResetMemoryArena(memoryArenaLastFrame);
    }

    RayCloseWindow();

    return 0;
}

#include "headers.h"

int WinMain(void)
{
    GameMemory gameMemory = {};

    InitializeArena(&gameMemory.permanentArena, Megabytes(100));
    InitializeArena(&gameMemory.temporaryArena, Megabytes(1000));
    InitializeArena(&gameMemory.rootImageArena, Megabytes(50));
    InitializeArena(&gameMemory.canvasArena, Megabytes(500));
    InitializeArena(&gameMemory.mouseClickArena, Megabytes(1));

    InitializeArena(&gameMemory.twoFrameArenaModIndex0, Megabytes(100));
    InitializeArena(&gameMemory.twoFrameArenaModIndex1, Megabytes(100));

    unsigned int threadCount = 8;
    PlatformWorkQueue *threadWorkQueue = SetupThreads(threadCount, &gameMemory);

    BpImage *rootBpImage = PushStruct(&gameMemory.permanentArena, BpImage);
    Canvas *canvas = PushStruct(&gameMemory.permanentArena, Canvas);

    ProcessedImage *processedImages = PushArray(&gameMemory.permanentArena, threadCount, ProcessedImage);
    for (int i = 0;
         i < threadCount;
         i++)
    {
        ProcessedImage *processedImage = processedImages + i;
        processedImage->rootBpImage = rootBpImage;
        processedImage->canvas = canvas;
        processedImage->index = i;
        InitializeArena(&processedImage->workArena, Megabytes(300));
    }

    GameState *gameState = PushStruct(&gameMemory.permanentArena, GameState);
    G_STRING_TEMP_MEM_ARENA = &gameMemory.temporaryArena;
    G_UI_INPUTS = PushStruct(&gameMemory.permanentArena, UiInputs);
    G_UI_STATE = PushStruct(&gameMemory.permanentArena, UiState);
    G_UI_HASH_TAG_STRING = CreateStringOnArena("##", &gameMemory.permanentArena);

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

    Color deleteColor = RED;

    Brush currentBrush = {};
    currentBrush.brushEffect = BRUSH_EFFECT_REMOVE;
    currentBrush.size = 20;

    Texture loadedTexture = {};

    stbi_write_force_png_filter = 5;
    int pngFilterLastFrame = stbi_write_force_png_filter;

    G_COMMAND_STATES[COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE].key[0] = KEY_E;
    G_COMMAND_STATES[COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE].key[0] = KEY_R;

    V2 pressedMousePos = {};
    String draggedUiStringKey = {};

    Slider brushSizeSlider = {};
    brushSizeSlider.sliderAction = SLIDER_ACTION_BRUSH_SIZE;
    brushSizeSlider.unsignedIntToChange = &currentBrush.size;
    brushSizeSlider.min = 1;
    brushSizeSlider.max = 50;

    //NOTE: DEVELOPER HACK
    {
        InitializeNewImage("./assets/handmadelogo.png", &gameMemory, rootBpImage, canvas, &loadedTexture);
    }

    while (!WindowShouldClose())
    {
        G_UI_STATE->twoFrameArenaLastFrame = GetTwoFrameArenaLastFrame(&gameMemory);
        G_UI_STATE->twoFrameArenaThisFrame = GetTwoFrameArenaThisFrame(&gameMemory);
        int uiBoxArrayIndex = GetFrameModIndexLastFrame();
        gameState->windowDim = WidthHeightToV2(GetScreenWidth(), GetScreenHeight());

        V2 mousePixelPos = V2{(float)GetMouseX(), (float)GetMouseY()};
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            ResetMemoryArena(&gameMemory.mouseClickArena);
            draggedUiStringKey = {};

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                pressedMousePos = mousePixelPos;
        }

        for (int i = 0;
             i < COMMAND_COUNT;
             i++)
        {
            CommandState *state = G_COMMAND_STATES + i;
            for (int j = 0;
                 j < MAX_KEYS_FOR_INPUT_BINDING;
                 j++)
            {
                KeyboardKey key = state->key[j];
                if (key)
                {
                    state->down = IsKeyDown(key);
                    state->pressed = IsKeyPressed(key);
                }
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

                if (IsInRect2D(mousePixelPos, uiBox->rect))
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
                        draggedUiStringKey = uiBox->keyString;
                        MoveStringToArena(&draggedUiStringKey, &gameMemory.mouseClickArena);
                    }
                }

                if (uiBox->uiInputs.sliderAction && draggedUiStringKey == uiBox->keyString)
                {
                    float normPressedPosInRect = (pressedMousePos.x - uiBox->rect.pos.x) / uiBox->rect.dim.x;
                    float normDifference = (pressedMousePos.x - mousePixelPos.x) / uiBox->rect.dim.x;
                    float normValue = Clamp(0, normPressedPosInRect - normDifference, 1);

                    //TODO: lookup slider
                    *brushSizeSlider.unsignedIntToChange = Lerp(brushSizeSlider.min, normValue, brushSizeSlider.max);
                }
            }
        }

        if (IsCommandPressed(COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE))
        {
            currentBrush.brushEffect = BRUSH_EFFECT_ERASE_EFFECT;
        }

        if (IsCommandPressed(COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE))
        {
            currentBrush.brushEffect = BRUSH_EFFECT_REMOVE;
        }

        String canvasStringKey = CreateString(G_CANVAS_STRING_TAG_CHARS);
        UiBox *canvasUiBox = GetUiBoxLastFrameOfStringKey(canvasStringKey);
        if (canvasUiBox && canvasUiBox->down)
        {
            ProcessedImage *processedImage = {};
            for (int i = 0;
                 i < threadCount;
                 i++)
            {
                ProcessedImage *processedImageOfIndex = processedImages + i;
                if (!processedImageOfIndex->active)
                {
                    processedImage = processedImageOfIndex;
                    break;
                }
            }

            float scale = Max(1, canvas->rootImageData.width / canvasUiBox->rect.dim.x);
            V2 startPos = scale * (mousePixelPos - RayVectorToV2(GetMouseDelta()) - canvasUiBox->rect.pos);
            V2 endPos = scale * (mousePixelPos - canvasUiBox->rect.pos);
            float distance = Max(1, DistanceV2(startPos, endPos));
            Color colorToPaint = G_BRUSH_EFFECT_COLORS[currentBrush.brushEffect];

            switch (currentBrush.brushEffect)
            {
            case BRUSH_EFFECT_ERASE_EFFECT:
            {
                for (int i = 0;
                     i <= distance;
                     i++)
                {
                    V2 pos = Lerp(startPos, i / distance, endPos);
                    ImageDrawCircle(&canvas->drawnImageData, pos.x, pos.y, currentBrush.size, colorToPaint);
                }

                break;
            }
            case BRUSH_EFFECT_REMOVE:
            {
                unsigned int processedImageIndex = (processedImage)
                                                       ? processedImage->index
                                                       : threadCount + 1;
                colorToPaint.a = processedImageIndex;

                for (int i = 0;
                     i <= distance;
                     i++)
                {
                    V2 pos = Lerp(startPos, i / distance, endPos);
                    ImageDrawCircle(&canvas->drawnImageData, pos.x, pos.y, currentBrush.size, colorToPaint);
                }

                break;
            }
                InvalidDefaultCase
            }

            if (processedImage)
            {
                StartProcessedImageWork(canvas, threadCount, processedImage, threadWorkQueue);
            }
            else
            {
                Print("no thread avaliabe");
                canvas->proccessAsap = true;
            }

            canvas->needsTextureUpload = true;
        }

        if (canvas->proccessAsap)
        {
            ProcessedImage *processedImage = GetFreeProcessedImage(processedImages, threadCount);
            if (processedImage)
                StartProcessedImageWork(canvas, threadCount, processedImage, threadWorkQueue);
        }

        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();
            char *fileName = droppedFiles.paths[0];
            InitializeNewImage(fileName, &gameMemory, rootBpImage, canvas, &loadedTexture);
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

        if (pngFilterLastFrame != stbi_write_force_png_filter)
            canvas->proccessAsap = true;

        ProcessedImage *latestCompletedProcessedImage = {};
        for (int i = 0;
             i < threadCount;
             i++)
        {
            ProcessedImage *processedImageOfIndex = processedImages + i;
            if (processedImageOfIndex->active && processedImageOfIndex->frameFinished > 0)
            {
                if (latestCompletedProcessedImage && (latestCompletedProcessedImage->frameStarted > processedImageOfIndex->frameStarted))
                {
                    Print("Throwing away image");
                    ResetProcessedImage(processedImageOfIndex, canvas, &gameMemory.temporaryArena);
                }
                else
                {
                    latestCompletedProcessedImage = processedImageOfIndex;
                }
            }
        }

        if (latestCompletedProcessedImage)
        {
            UploadAndReplaceTexture(&latestCompletedProcessedImage->finalProcessedImage, &loadedTexture);
            Print("Uploading New Image");
            //TODO: put the latest uploaded image somewhere for safekeeping
            ResetProcessedImage(latestCompletedProcessedImage, canvas, &gameMemory.temporaryArena);
        }

        if (canvas->needsTextureUpload)
        {
            canvas->needsTextureUpload = false;
            Image outputImage = {};
            unsigned int pixelCount = canvas->rootImageData.width * canvas->rootImageData.height;
            Color *pixels = PushArray(&gameMemory.temporaryArena, pixelCount, Color);

            for (int i = 0;
                 i < pixelCount;
                 i++)
            {
                Color rootPixel = ((Color *)canvas->rootImageData.data)[i];
                Color drawnPixel = ((Color *)canvas->drawnImageData.data)[i];

                if (drawnPixel.r || drawnPixel.g || drawnPixel.b || drawnPixel.a)
                {
                    if (drawnPixel.a < 255)
                        drawnPixel.a = 100;

                    *(pixels + i) = drawnPixel;
                }
                else
                    *(pixels + i) = rootPixel;
            }

            UploadTexture(&canvas->texture, pixels, canvas->rootImageData.width, canvas->rootImageData.height);
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

        SetUiAxis({UI_SIZE_KIND_PIXELS, gameState->windowDim.x}, {UI_SIZE_KIND_PIXELS, gameState->windowDim.y});
        CreateUiBox();
        UiParent()
        {
            uiSettings->frontColor = BLACK;
            uiSettings->borderColor = DARKGRAY;
            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, titleBarHeight});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
                CreateUiBox();

                SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 0.5}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 1});
                CreateUiBox();

                SetUiAxis({UI_SIZE_KIND_TEXT}, {UI_SIZE_KIND_TEXT});
                uiSettings->frontColor = GREEN;
                String label = CreateString("PNG Filter: ") + G_PNG_FILTER_NAMES[stbi_write_force_png_filter];
                CreateUiBox(UI_FLAG_DRAW_TEXT, label);
            }

            SetUiAxis({UI_SIZE_KIND_PIXELS, gameState->windowDim.x}, {UI_SIZE_KIND_PIXELS, gameState->windowDim.y - titleBarHeight});
            CreateUiBox();
            UiParent()
            {
                SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 0.5});
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
                SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 0.5});
                CreateUiBox(UI_FLAG_DRAW_BORDER);
                UiParent()
                {
                    if (canvas->texture.id)
                    {
                        G_UI_INPUTS->texture = canvas->texture;
                        SetUiAxis({UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT}, {UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT});
                        CreateUiBox(UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT | UI_FLAG_INTERACTABLE, G_UI_HASH_TAG_STRING + G_CANVAS_STRING_TAG_CHARS);
                    }
                }
            }
        }

        float toolboxWidthAndHeight = 35;
        float toolbarWidth = toolboxWidthAndHeight * 2;

        SetUiAxis({UI_SIZE_KIND_PIXELS, toolbarWidth}, {UI_SIZE_KIND_PIXELS, 400});
        G_UI_INPUTS->pixelPosition = V2{0, (float)titleBarHeight};
        uiSettings->backColor = Color{191, 191, 191, 255};
        CreateUiBox(UI_FLAG_DRAW_BACKGROUND);
        UiParent()
        {
            ReactiveUiColorState uiColorState = {};
            uiColorState.active.disabled = DARKGRAY;
            uiColorState.active.down = Color{100, 100, 100, 255};
            uiColorState.active.hovered = Color{230, 230, 230, 255};
            uiColorState.active.neutral = Color{221, 221, 221, 255};
            uiColorState.nonActive.disabled = DARKGRAY;
            uiColorState.nonActive.down = Color{100, 100, 100, 255};
            uiColorState.nonActive.hovered = Color{221, 221, 221, 255};
            uiColorState.nonActive.neutral = Color{191, 191, 191, 255};

            uiSettings->frontColor = BLACK;
            uiSettings->borderColor = GRAY;

            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
                SetUiAxis({UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
            }
            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
                SetUiAxis({UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
            }
            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
                SetUiAxis({UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
            }
            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
                SetUiAxis({UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
            }
            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
                SetUiAxis({UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
            }

            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, 40});
            CreateUiBox();

            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
                SetUiAxis({UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
                String string = CreateString("E") + G_UI_HASH_TAG_STRING + CreateString(BRUSH_EFFECT_ERASE_EFFECT);
                bool active = currentBrush.brushEffect == BRUSH_EFFECT_ERASE_EFFECT;
                G_UI_INPUTS->command = COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE;
                CreateUiButton(string, uiColorState, active, false);

                string = CreateString("R") + G_UI_HASH_TAG_STRING + CreateString(BRUSH_EFFECT_REMOVE);
                active = currentBrush.brushEffect == BRUSH_EFFECT_REMOVE;
                G_UI_INPUTS->command = COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE;
                CreateUiButton(string, uiColorState, active, false);
            }
            uiSettings->frontColor = BLACK;
            uiSettings->borderColor = GRAY;

            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
            }

            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, 40});
            CreateUiBox();

            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_TEXT});
            CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_CENTERED, IntToString(currentBrush.size));

            ReactiveUiColor uiColorParent = {};
            uiColorParent.down = Color{220, 220, 220, 255};
            uiColorParent.hovered = Color{200, 200, 200, 255};
            uiColorParent.neutral = Color{180, 180, 180, 255};
            String stringKey = CreateString("brushSizeSlider");
            UiBox *sliderUiBox = GetUiBoxLastFrameOfStringKey(stringKey);
            uiSettings->backColor = GetReactiveColor(sliderUiBox, uiColorParent, false);
            G_UI_INPUTS->sliderAction = SLIDER_ACTION_BRUSH_SIZE;

            Slider slider = brushSizeSlider;
            float value = MapNormalize(*brushSizeSlider.unsignedIntToChange, brushSizeSlider.min, brushSizeSlider.max);
            G_UI_INPUTS->value = value;

            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, toolboxWidthAndHeight * 0.5f});
            CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_INTERACTABLE | UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT | UI_FLAG_DRAW_BORDER, G_UI_HASH_TAG_STRING + stringKey);
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
            CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_BORDER);
        }

        SetUiAxis({UI_SIZE_KIND_PIXELS, windowDim.x}, {UI_SIZE_KIND_TEXT});
        String fps = CreateString("FPS: ") + GetFPS();
        // G_UI_INPUTS->pixelPosition = V2{0, 0};
        CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT, fps);

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

        G_CURRENT_FRAME++;
        pngFilterLastFrame = stbi_write_force_png_filter;
    }

    RayCloseWindow();

    return 0;
}
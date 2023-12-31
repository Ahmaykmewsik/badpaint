
#include "headers.h"

void RunApp(PlatformWorkQueue *threadWorkQueue, GameMemory gameMemory, unsigned int threadCount)
{
    BpImage *rootBpImage = PushStruct(&gameMemory.permanentArena, BpImage);
    Canvas *canvas = PushStruct(&gameMemory.permanentArena, Canvas);
    // BpImage latestCompletedBpImage = {};

    bool imageIsBroken = {};

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

    G_STRING_TEMP_MEM_ARENA = &gameMemory.temporaryArena;
    _G_CIRCULAR_ARENA_DONT_FUCKING_USE_THIS_EXCEPT_IN_A_MACRO = &gameMemory.circularScratchBuffer;
    G_UI_INPUTS = PushStruct(&gameMemory.permanentArena, UiInputs);
    G_UI_STATE = PushStruct(&gameMemory.permanentArena, UiState);

    SetTargetFPS(60);

    SetTraceLogLevel(RL_LOG_NONE);

    InitWindow(200, 200, "badpaint");

    SetWindowState(FLAG_WINDOW_RESIZABLE);

    V2 screenDim = V2{(float)GetMonitorWidth(0), (float)GetMonitorHeight(0)};

    V2 windowDim = screenDim * 0.8f;

    V2 windowPosMiddle = PositionInCenter(screenDim, windowDim);
    SetWindowPosition(windowPosMiddle.x, windowPosMiddle.y);
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

    V2 pressedMousePos = {};
    String draggedUiStringKey = {};

    Slider brushSizeSlider = {};
    brushSizeSlider.sliderAction = SLIDER_ACTION_BRUSH_SIZE;
    brushSizeSlider.unsignedIntToChange = &currentBrush.size;
    brushSizeSlider.min = 1;
    brushSizeSlider.max = 50;

    //NOTE: DEVELOPER HACK
    {
        // InitializeNewImage("./assets/handmadelogo.png", &gameMemory, rootBpImage, canvas, &loadedTexture, &currentBrush);
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
        windowDim = WidthHeightToV2(GetScreenWidth(), GetScreenHeight());

        V2 mousePixelPos = V2{(float)GetMouseX(), (float)GetMouseY()};
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            ResetMemoryArena(&gameMemory.mouseClickArena);
            draggedUiStringKey = {};

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                pressedMousePos = mousePixelPos;
        }

        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();
            char *fileName = droppedFiles.paths[0];
            InitializeNewImage(fileName, &gameMemory, rootBpImage, canvas, &loadedTexture, &currentBrush);

            if (rootBpImage->dataSize > 15000000)
            {
                String notification = CreateString("...Are you serious?!? Ok be patient with me, this image is freaking huge. I'm not going to run well at all.");
                InitNotificationMessage(notification, &gameMemory.circularScratchBuffer);
            }
            else if (rootBpImage->dataSize > 8000000)
            {
                String notification = CreateString("Uh...I'm not really ready to edit images this big yet, but I can try. Don't blame me if I'm slow though. You asked for it.");
                InitNotificationMessage(notification, &gameMemory.circularScratchBuffer);
            }
            else if (rootBpImage->dataSize > 5000000)
            {
                String notification = CreateString("Woah, this image is kind of large!. I'll try my best...");
                InitNotificationMessage(notification, &gameMemory.circularScratchBuffer);
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
                String filePath = AllocateString(256, &gameMemory.temporaryArena);
                unsigned int filepathLength;
                bool success = GetPngImageFilePathFromUser(filePath.chars, filePath.length, &filepathLength);
                filePath.length = filepathLength;
                if (success && filePath.length)
                {
                    Image exportImgae = LoadImageFromTexture(loadedTexture);
                    ExportImage(exportImgae, filePath);

                    String notification = CreateString("You have given new life to: ") + filePath;
                    InitNotificationMessage(notification, &gameMemory.circularScratchBuffer);
                }
                else
                {
                    String notification = CreateString("Sorry the save failed. You fail too. You suck. Sorry.");
                    InitNotificationMessage(notification, &gameMemory.circularScratchBuffer);
                }
            }
            else
            {
                String notification = CreateString("Bruh.");
                InitNotificationMessage(notification, &gameMemory.circularScratchBuffer);
            }
        }

        String canvasStringKey = CreateString(G_CANVAS_STRING_TAG_CHARS);
        UiBox *canvasUiBox = GetUiBoxLastFrameOfStringKey(canvasStringKey);
        if (canvasUiBox && canvasUiBox->down && !imageIsBroken)
        {
            if (canvasUiBox->pressed)
            {
                unsigned int canvasSize = GetCanvasDatasize(canvas);
                unsigned char *newRollbackImage = &canvas->rollbackImageData[(canvasSize * canvas->rollbackIndexNext)];
                memcpy(newRollbackImage, canvas->drawnImageData.data, canvasSize);
                ModNext(canvas->rollbackIndexNext, canvas->rollbackSizeCount - 1);
                if (canvas->rollbackIndexNext == canvas->rollbackIndexStart)
                    ModNext(canvas->rollbackIndexStart, canvas->rollbackSizeCount - 1);
            }

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

            float scale = Max(1, canvas->filteredRootImage.width / canvasUiBox->rect.dim.x);
            V2 startPos = scale * (mousePixelPos - RayVectorToV2(GetMouseDelta()) - canvasUiBox->rect.pos);
            V2 endPos = scale * (mousePixelPos - canvasUiBox->rect.pos);
            float distance = Max(1, DistanceV2(startPos, endPos));
            Color colorToPaint = {};

            if (currentBrush.brushEffect != BRUSH_EFFECT_ERASE_EFFECT)
            {
                unsigned int processedImageIndex = (processedImage)
                                                       ? processedImage->index
                                                       : threadCount + 1;
                colorToPaint.r = currentBrush.brushEffect;
                colorToPaint.g = RandomInRangeInt(0, 255);
                colorToPaint.a = processedImageIndex;
            }

            for (int i = 0;
                 i <= distance;
                 i++)
            {
                V2 pos = Lerp(startPos, i / distance, endPos);
                ImageDrawCircle(&canvas->drawnImageData, pos.x, pos.y, currentBrush.size, colorToPaint);
            }

            if (processedImage)
            {
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
                ModBack(canvas->rollbackIndexNext, canvas->rollbackSizeCount - 1);
                unsigned int canvasSize = GetCanvasDatasize(canvas);
                unsigned char *rollbackImage = &canvas->rollbackImageData[(canvasSize * canvas->rollbackIndexNext)];
                memcpy(canvas->drawnImageData.data, rollbackImage, canvasSize);
                canvas->proccessAsap = true;
                canvas->needsTextureUpload = true;
                canvas->oldDataPresent = true;
                imageIsBroken = false;
            }
            else
            {
                String notification = CreateString("The past is no more. You must now live with your mistakes. You've run out of undos!");
                InitNotificationMessage(notification, &gameMemory.circularScratchBuffer);
            }
        }

        if (canvas->texture.id && canvas->proccessAsap)
        {
            ProcessedImage *processedImage = GetFreeProcessedImage(processedImages, threadCount);
            if (processedImage)
                StartProcessedImageWork(canvas, threadCount, processedImage, threadWorkQueue);
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

        if (canvas->texture.id && pngFilterLastFrame != stbi_write_force_png_filter)
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
                    // Print("Throwing away image from thread " + IntToString(latestCompletedProcessedImage->index));
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
            if (latestCompletedProcessedImage->finalProcessedBpImage.data)
            {
                UploadAndReplaceTexture(&latestCompletedProcessedImage->finalProcessedBpImage, &loadedTexture);
                // Print("Uploading New Image from thread " + IntToString(latestCompletedProcessedImage->index));
                //TODO: put the latest uploaded image somewhere for safekeeping?
            }
            else
            {
                imageIsBroken = true;
            }

            ResetMemoryArena(&gameMemory.latestCompletedImageArena);
            // latestCompletedBpImage = CreateDataImage(rootBpImage, latestCompletedProcessedImage->convertedImage, &gameMemory);
            ResetProcessedImage(latestCompletedProcessedImage, canvas, &gameMemory.temporaryArena);
        }

        if (canvas->needsTextureUpload)
        {
            canvas->needsTextureUpload = false;
            Image outputImage = {};
            V2 dim = WidthHeightToV2(canvas->filteredRootImage.width, canvas->filteredRootImage.height);
            unsigned int pixelCount = dim.x * dim.y;
            Color *pixels = PushArray(&gameMemory.temporaryArena, pixelCount, Color);

            for (int i = 0;
                 i < pixelCount;
                 i++)
            {
                Color rootPixel = ((Color *)canvas->filteredRootImage.data)[i];
                Color canvasPixel = ((Color *)canvas->drawnImageData.data)[i];
                Color drawnPixel = G_BRUSH_EFFECT_COLORS[canvasPixel.r];
                Color *outPixel = (pixels + i);

                if (canvasPixel.a < 255)
                    drawnPixel.a = 255 * 0.5;
                *outPixel = drawnPixel;

                if (!canvasPixel.r)
                    *outPixel = rootPixel;
            }

            UploadTexture(&canvas->texture, pixels, dim.x, dim.y);
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

        SetUiAxis({UI_SIZE_KIND_PIXELS, windowDim.x}, {UI_SIZE_KIND_PIXELS, windowDim.y});
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

            SetUiAxis({UI_SIZE_KIND_PIXELS, windowDim.x}, {UI_SIZE_KIND_PIXELS, windowDim.y - titleBarHeight});
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
                            String string = CreateString("Congulations! You broke the image. (undo with Ctrl-Z)");
                            SetUiAxis({UI_SIZE_KIND_TEXT, 1}, {UI_SIZE_KIND_TEXT, 1});
                            CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_CENTER_IN_PARENT, string);
                        }
                    }
                    else
                    {
                        String string = CreateString("Drop any image into the window for editing.");
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
                        CreateUiBox(UI_FLAG_DRAW_TEXTURE | UI_FLAG_CENTER_IN_PARENT | UI_FLAG_INTERACTABLE, CreateString(G_UI_HASH_TAG) + G_CANVAS_STRING_TAG_CHARS);
                    }
                }
            }
        }

        float toolbarWidth = G_TOOLBOX_WIDTH_AND_HEIGHT * 2;

        SetUiAxis({UI_SIZE_KIND_PIXELS, toolbarWidth}, {UI_SIZE_KIND_PIXELS});
        G_UI_INPUTS->relativePixelPosition = V2{0, (float)titleBarHeight};
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
                CreateBrushEffectButton(BRUSH_EFFECT_ERASE_EFFECT, CreateString("Ers"), Color{245, 245, 245, 255}, COMMAND_SWITCH_BRUSH_EFFECT_TO_ERASE, &currentBrush);
                CreateBrushEffectButton(BRUSH_EFFECT_REMOVE, CreateString("Rmv"), G_BRUSH_EFFECT_COLORS[BRUSH_EFFECT_REMOVE], COMMAND_SWITCH_BRUSH_EFFECT_TO_REMOVE, &currentBrush);
            }
            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
                CreateBrushEffectButton(BRUSH_EFFECT_MAX, CreateString("Max"), G_BRUSH_EFFECT_COLORS[BRUSH_EFFECT_MAX], COMMAND_SWITCH_BRUSH_EFFECT_TO_MAX, &currentBrush);
                CreateBrushEffectButton(BRUSH_EFFECT_SHIFT, CreateString("Sft"), G_BRUSH_EFFECT_COLORS[BRUSH_EFFECT_SHIFT], COMMAND_SWITCH_BRUSH_EFFECT_TO_SHIFT, &currentBrush);
            }
            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, G_TOOLBOX_WIDTH_AND_HEIGHT});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
                CreateBrushEffectButton(BRUSH_EFFECT_RANDOM, CreateString("Rnd"), G_BRUSH_EFFECT_COLORS[BRUSH_EFFECT_RANDOM], COMMAND_SWITCH_BRUSH_EFFECT_TO_RANDOM, &currentBrush);
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
            SetUiAxis({UI_SIZE_KIND_PIXELS, windowDim.x}, {UI_SIZE_KIND_TEXT});
            uiSettings->frontColor = Color{255, 255, 255, (unsigned char)(G_NOTIFICATION_ALPHA * 255)};
            uiSettings->backColor = Color{100, 100, 100, (unsigned char)(G_NOTIFICATION_ALPHA * 255)};
            G_UI_INPUTS->relativePixelPosition = V2{0, titleBarHeight};
            CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT, G_NOTIFICATION_MESSAGE);

            G_NOTIFICATION_ALPHA -= 0.001;
            if (G_NOTIFICATION_ALPHA <= 0)
            {
                G_NOTIFICATION_MESSAGE = {};
                G_NOTIFICATION_ALPHA = {};
            }
        }

        SetUiAxis({UI_SIZE_KIND_TEXT}, {UI_SIZE_KIND_TEXT});
        uiSettings->frontColor = BLACK;
        int fps = GetFPS();
        String fpsString = CreateString("FPS: ") + IntToString(fps);
        if (fps < 60)
            uiSettings->frontColor = RED;
        CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_RIGHT, fpsString);

        uiSettings->frontColor = DARKGRAY;
        G_UI_INPUTS->relativePixelPosition = V2{windowDim.x * 0.07f, 2};
        String label = CreateString("Use 1-5 to toggle differnet PNG filter algorythms");
        CreateUiBox(UI_FLAG_DRAW_TEXT, label);

        uiSettings->frontColor = BLACK;
        G_UI_INPUTS->relativePixelPosition = V2{windowDim.x * 0.45f, 2};
        label = CreateString("PNG Filter: ") + G_PNG_FILTER_NAMES[stbi_write_force_png_filter];
        CreateUiBox(UI_FLAG_DRAW_TEXT, label);

        SetUiAxis({UI_SIZE_KIND_PIXELS, 200}, {UI_SIZE_KIND_TEXT});
        G_UI_INPUTS->relativePixelPosition = V2{windowDim.x * 0.8f, 2};
        label = CreateString("EXPORT IMAGE") + G_UI_HASH_TAG + "export";
        G_UI_INPUTS->command = COMMAND_EXPORT_IMAGE;
        ReactiveUiColorState uiColorState = {};
        uiColorState.nonActive.down = DARKGREEN;
        uiColorState.nonActive.hovered = Color{10, 238, 58, 255};
        uiColorState.nonActive.neutral = GREEN;
        CreateUiButton(label, uiColorState, false, false);

        SetUiAxis({UI_SIZE_KIND_PIXELS, windowDim.x}, {UI_SIZE_KIND_TEXT});
        G_UI_INPUTS->relativePixelPosition = V2{-5, windowDim.y - 20};
        uiSettings->frontColor = DARKGRAY;
        label = CreateString(VERSION_NUMBER);
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
                    RenderUiEntries(uiBox, windowDim);
                }
            }
        }

        EndDrawing();

        ResetMemoryArena(&gameMemory.temporaryArena);
        MemoryArena *memoryArenaLastFrame = GetTwoFrameArenaLastFrame(&gameMemory);
        ResetMemoryArena(memoryArenaLastFrame);

        G_CURRENT_FRAME++;
        pngFilterLastFrame = stbi_write_force_png_filter;
    }

    RayCloseWindow();
}
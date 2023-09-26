
#include "headers.h"

#define MAX_FILEPATH_RECORDED 4096
#define MAX_FILEPATH_SIZE 2048

int main(void)
{
    GameMemory gameMemory = {};

    InitializeArena(&gameMemory.permanentArena, Megabytes(100));
    InitializeArena(&gameMemory.temporaryArena, Megabytes(1000));
    InitializeArena(&gameMemory.rootImageArena, Megabytes(50));

    InitializeArena(&gameMemory.twoFrameArenaModIndex0, Megabytes(100));
    InitializeArena(&gameMemory.twoFrameArenaModIndex1, Megabytes(100));

    GameState *gameState = PushStruct(&gameMemory.permanentArena, GameState);
    G_STRING_TEMP_MEM_ARENA = &gameMemory.temporaryArena;
    G_UI_INPUTS = PushStruct(&gameMemory.permanentArena, UiInputs);
    G_UI_STATE = PushStruct(&gameMemory.permanentArena, UiState);
    G_UI_HASH_TAG_STRING = CreateStringOnArena("##", &gameMemory.permanentArena);

    SetTargetFPS(60);

    gameState->windowDim = V2{1000, 1000};
    V2 windowDim = gameState->windowDim;

    InitWindow(windowDim.x, windowDim.y, "badpaint");

    V2 screenDim = V2{(float)GetMonitorWidth(0), (float)GetMonitorHeight(0)};

    V2 windowPosMiddle = (screenDim * 0.5f) - (windowDim * 0.5f);

    Font defaultFont = LoadFontEx("./assets/verdana.ttf", 18, 0, 0);

    SetWindowPosition(windowPosMiddle.x, windowPosMiddle.y);

    BpImage loadedBpImage = {};
    Texture loadedTexture = {};

    //NOTE: DEVELOPER HACK
    {
        // loadedImage = LoadDataIntoRawImage("./assets/handmadelogo.png");
        // loadedTexture = LoadTextureFromImage(loadedImage);
    }

    while (!WindowShouldClose())
    {
        //----------------------------------------------------
        //---------------------INPUT--------------------------
        //----------------------------------------------------

        G_UI_STATE->twoFrameArenaLastFrame = GetTwoFrameArenaLastFrame(&gameMemory);
        G_UI_STATE->twoFrameArenaThisFrame = GetTwoFrameArenaThisFrame(&gameMemory);
        int uiBoxArrayIndex = GetFrameModIndexLastFrame();

        for (int i = 0;
             i < G_UI_STATE->uiBoxCount;
             i++)
        {
            UiBox *uiBox = &G_UI_STATE->uiBoxes[uiBoxArrayIndex][i];
            if (IsFlag(uiBox, UI_FLAG_INTERACTABLE))
            {
                V2 mousePixelPos = V2{(float)GetMouseX(), (float)GetMouseY()};
                uiBox->cursorRelativePixelPos = mousePixelPos - uiBox->rect.pos;

                if (IsInRect2D(mousePixelPos, uiBox->rect))
                {
                    uiBox->hovered = true;
                    uiBox->pressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
                    uiBox->down = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
                }
            }
        }

        //----------------------------------------------------
        //----------------------GAME--------------------------
        //----------------------------------------------------

        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();
            char *fileName = droppedFiles.paths[0];

            loadedBpImage = LoadDataIntoRawImage(fileName, &gameMemory);
            UploadAndReplaceTexture(&loadedBpImage, &loadedTexture);
            UnloadDroppedFiles(droppedFiles);
        }

        if (loadedBpImage.data)
        {
            if (IsKeyDown(KEY_F))
            {
                // Assert(loadedImage.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

                for (int i = 0;
                     i < 100;
                     i++)
                {
                    unsigned int startChunk = RandomInRangeInt(0, loadedBpImage.dataSize);
                    unsigned int randomSize = RandomInRangeInt(1, 1000);
                    unsigned int endChunk = startChunk + Clamp(0, randomSize, loadedBpImage.dataSize - startChunk);

                    for (int j = startChunk;
                         j < endChunk;
                         j++)
                    {
                        ((unsigned char *)loadedBpImage.data)[j] = ((unsigned char *)loadedBpImage.data)[j + 21];
                    }
                }

                UploadAndReplaceTexture(&loadedBpImage, &loadedTexture);
            }
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

        ClearBackground(WHITE);

        uiSettings->font = defaultFont;
        uiSettings->fontSize = defaultFont.baseSize;

        SetUiAxis({UI_SIZE_KIND_PIXELS, gameState->windowDim.x}, {UI_SIZE_KIND_PIXELS, gameState->windowDim.y});
        CreateUiBox();
        UiParent()
        {
            float titleBarHeight = 20;

            uiSettings->backColor = Color{100, 100, 100, 230};
            uiSettings->frontColor = BLACK;
            uiSettings->backColor = Color{100, 100, 100, 230};
            uiSettings->borderColor = DARKBLUE;
            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, titleBarHeight});
            CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_TEXT, CreateString("Menu up here eventually"));

            SetUiAxis({UI_SIZE_KIND_PIXELS, gameState->windowDim.x}, {UI_SIZE_KIND_PIXELS, gameState->windowDim.y - titleBarHeight});
            CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
            UiParent()
            {
                SetUiAxis({UI_SIZE_KIND_PIXELS, 80}, {UI_SIZE_KIND_PIXELS, 600});
                uiSettings->backColor = Color{200, 200, 200, 230};
                CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_ALIGN_TEXTURE_CENTERED | UI_FLAG_DRAW_TEXT, CreateString(("maybe\nsome\ntools\nhere\nidk")));

                SetUiAxis({UI_SIZE_KIND_PIXELS, gameState->windowDim.x - 20}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 1});
                CreateUiBox(UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);
                UiParent()
                {
                    if (loadedTexture.id)
                    {
                        G_UI_INPUTS->texture = loadedTexture;
                        SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PERCENT_OF_PARENT, 1});
                        CreateUiBox(UI_FLAG_DRAW_TEXTURE | UI_FLAG_ALIGN_TEXTURE_CENTERED);
                    }
                    else
                    {
                        G_UI_INPUTS->texture = loadedTexture;
                        String string = CreateString("Drop any file into this window for editing.");
                        CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_CENTERED, string);
                    }
                }
            }
        }

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
                    case UI_SIZE_KIND_TEXT_NO_WRAPPING:
                    {
                        uiBox->rect.dim.elements[j] = uiBox->textDim.elements[j];
                    }
                    case UI_SIZE_KIND_PERCENT_OF_PARENT:
                    case UI_SIZE_KIND_TEXT_WRAP_TO_PARENT:
                    case UI_SIZE_KIND_CHILDREN_OF_SUM:
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
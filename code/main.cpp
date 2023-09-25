
#include "headers.h"

#define MAX_FILEPATH_RECORDED 4096
#define MAX_FILEPATH_SIZE 2048

Image LoadDataIntoRawImage(char *filePath)
{
    Image result = {};

    unsigned int fileSize = {};
    unsigned char *fileData = LoadFileData(filePath, &fileSize);
    const char *getFileExtension = GetFileExtension(filePath);

    if (fileData != NULL)
    {
        // NOTE: Using stb_image to load images (Supports multiple image formats)

        if (fileData != NULL)
        {
            int comp = 0;
            result.data = stbi_load_from_memory(fileData, fileSize, &result.width, &result.height, &comp, 0);

            if (result.data != NULL)
            {
                result.mipmaps = 1;

                if (comp == 1)
                    result.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
                else if (comp == 2)
                    result.format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA;
                else if (comp == 3)
                    result.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
                else if (comp == 4)
                    result.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            }
            else
            {
                //TODO: load invalid data anyway
            }
        }
    }
    else
    {
        //TODO: Logging
    }

    return result;
}

int main(void)
{
    GameMemory gameMemory = {};

    InitializeArena(&gameMemory.permanentArena, Megabytes(100));
    InitializeArena(&gameMemory.temporaryArena, Megabytes(1000));
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

    Image loadedImage = {};
    Texture loadedTexture = {};

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

#if 0
                if (uiBox->uiInputs.inputCommand)
                {
                    //TODO: we don't need to store these like this. Why not just access commandState directly when needed?
                    CommandState *commandState = &inputState->commandStates[uiBox->uiInputs.inputCommand];

                    uiBox->down |= commandState->down;
                    uiBox->pressed |= commandState->pressed;

                    commandState->down |= uiBox->down;
                    commandState->pressed |= uiBox->pressed;
                }
#endif
            }
        }

        //----------------------------------------------------
        //----------------------GAME--------------------------
        //----------------------------------------------------

        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();
            char *fileName = droppedFiles.paths[0];

            loadedImage = LoadDataIntoRawImage(fileName);
            loadedTexture = LoadTextureFromImage(loadedImage);
            UnloadDroppedFiles(droppedFiles);
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
            uiSettings->backColor = Color{100, 100, 100, 230};
            uiSettings->frontColor = BLACK;
            uiSettings->backColor = Color{100, 100, 100, 230};
            SetUiAxis({UI_SIZE_KIND_PERCENT_OF_PARENT, 1}, {UI_SIZE_KIND_PIXELS, 30});
            CreateUiBox(UI_FLAG_DRAW_BACKGROUND | UI_FLAG_DRAW_TEXT, CreateString("Menu up here eventually"));

            SetUiAxis({UI_SIZE_KIND_PIXELS, gameState->windowDim.x}, {UI_SIZE_KIND_PIXELS, 50});
            CreateUiBox();

            if (loadedTexture.id)
            {
                G_UI_INPUTS->texture = loadedTexture;
                CreateUiBox(UI_FLAG_DRAW_TEXTURE);
            }
            else
            {
                G_UI_INPUTS->texture = loadedTexture;
                String string = CreateString("Drop any file into this window for editing.");
                CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_ALIGN_TEXT_CENTERED, string);
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
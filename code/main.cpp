
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
                // result.data = stbi__load
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
    V2 windowDim = V2{1000, 1000};

    InitWindow(windowDim.x, windowDim.y, "badpaint");

    V2 screenDim = V2{(float)GetMonitorWidth(0), (float)GetMonitorHeight(0)};

    V2 windowPosMiddle = (screenDim * 0.5f) - (windowDim * 0.5f);

    SetWindowPosition(windowPosMiddle.x, windowPosMiddle.y);

    GameMemory gameMemory = {};

    InitializeArena(&gameMemory.permanentArena, Megabytes(100));
    InitializeArena(&gameMemory.temporaryArena, Megabytes(1000));

    G_STRING_TEMP_MEM_ARENA = &gameMemory.temporaryArena;

    SetTargetFPS(60);

    Image loadedImage = {};
    Texture loadedTexture = {};

    while (!WindowShouldClose())
    {
        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();
            char *fileName = droppedFiles.paths[0];

            loadedImage = LoadDataIntoRawImage(fileName);
            loadedTexture = LoadTextureFromImage(loadedImage);
            UnloadDroppedFiles(droppedFiles);
        }

        //----------------------------------------------------------------------------------

        BeginDrawing();

        ClearBackground(WHITE);

        if (!loadedImage.data)
            DrawText("Drop any file into this window for editing.", 100, 40, 20, DARKGRAY);
        else
        {
            DrawTextureEx(loadedTexture, {0, 0}, 0, 1, WHITE);
        }

        EndDrawing();

        ResetMemoryArena(&gameMemory.temporaryArena);
    }

    RayCloseWindow();

    return 0;
}
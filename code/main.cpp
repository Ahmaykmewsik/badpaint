
#include "headers.h"

#define MAX_FILEPATH_RECORDED 4096
#define MAX_FILEPATH_SIZE 2048

int main(void)
{
    V2 windowDim = V2{1000, 1000};

    InitWindow(windowDim.x, windowDim.y, "badpaint");

    GameMemory gameMemory = {};

    InitializeArena(&gameMemory.permanentArena, Megabytes(100));
    InitializeArena(&gameMemory.temporaryArena, Megabytes(1000));

    G_STRING_TEMP_MEM_ARENA = &gameMemory.temporaryArena;

    int filePathCounter = 0;
    String filePaths[MAX_FILEPATH_RECORDED] = {};

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();

            for (int i = 0, offset = filePathCounter;
                 i < droppedFiles.count;
                 i++)
            {
                if (filePathCounter < (MAX_FILEPATH_RECORDED - 1))
                {
                    filePaths[offset + i] = CreateString(droppedFiles.paths[i]);
                    filePathCounter++;
                }
            }

            UnloadDroppedFiles(droppedFiles); 
        }

        //----------------------------------------------------------------------------------

        BeginDrawing();

        ClearBackground(RAYWHITE);

        if (filePathCounter == 0)
            DrawText("Drop your files to this window!", 100, 40, 20, DARKGRAY);
        else
        {
            DrawText("Dropped files:", 100, 40, 20, DARKGRAY);

            for (unsigned int i = 0; i < filePathCounter; i++)
            {
                if (i % 2 == 0)
                    DrawRectangle(0, 85 + 40 * i, windowDim.x, 40, Fade(LIGHTGRAY, 0.5f));
                else
                    DrawRectangle(0, 85 + 40 * i, windowDim.y, 40, Fade(LIGHTGRAY, 0.3f));

                DrawText(filePaths[i].chars, 120, 100 + 40 * i, 10, GRAY);
            }

            DrawText("Drop new files...", 100, 110 + 40 * filePathCounter, 20, DARKGRAY);
        }

        EndDrawing();

        ResetMemoryArena(&gameMemory.temporaryArena);
    }

    RayCloseWindow(); 

    return 0;
}
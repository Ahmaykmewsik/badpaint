#pragma once

#include "headers.h"

unsigned char *LoadDataFromDisk(const char *fileName, unsigned int *bytesRead, MemoryArena *temporaryArena)
{
    unsigned char *data = NULL;
    *bytesRead = 0;

    FILE *file = fopen(fileName, "rb");

    if (file != NULL)
    {
        // WARNING: On binary streams SEEK_END could not be found,
        // using fseek() and ftell() could not work in some (rare) cases
        fseek(file, 0, SEEK_END);
        int size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (size > 0)
        {
            data = PushArray(temporaryArena, size, unsigned char);

            // NOTE: fread() returns number of read elements instead of bytes, so we read [1 byte, size elements]
            unsigned int count = (unsigned int)fread(data, sizeof(unsigned char), size, file);
            *bytesRead = count;

            if (count != size)
                TRACELOG(LOG_WARNING, "FILEIO: [%s] File partially loaded", fileName);
            else
                TRACELOG(LOG_INFO, "FILEIO: [%s] File loaded successfully", fileName);
        }
        else
            TRACELOG(LOG_WARNING, "FILEIO: [%s] Failed to read file", fileName);

        fclose(file);
    }
    else
        TRACELOG(LOG_WARNING, "FILEIO: [%s] Failed to open file", fileName);

    return data;
}

BpImage LoadDataIntoRawImage(const char *filePath, GameMemory *gameMemory)
{
    BpImage result = {};

    unsigned int fileSize = {};
    unsigned char *fileData = LoadDataFromDisk(filePath, &fileSize, &gameMemory->temporaryArena);
    const char *getFileExtension = GetFileExtension(filePath);

    if (fileData != NULL)
    {
        // NOTE: Using stb_image to load images (Supports multiple image formats)

        if (fileData != NULL)
        {
            int comp = 0;
            int width = 0;
            int height = 0;
            void *outputData = stbi_load_from_memory(fileData, fileSize, &width, &height, &comp, 0);

            if (outputData != NULL)
            {
                result.imageFormat = IMAGE_FORMAT_R8G8B8A8;
                result.dim.x = width;
                result.dim.y = height;
                result.dataSize = width * height * 4 * sizeof(unsigned char);
                result.data = PushSize(&gameMemory->rootImageArena, result.dataSize); 

                if (comp != 4)
                {
                    V4 *pixels = PushArray(&gameMemory->temporaryArena, width * height, V4);
                    for (int i = 0, k = 0;
                         i < result.dim.x * result.dim.y;
                         i++)
                    {
                        switch (comp)
                        {
                        case 1:
                        {
                            //NOTE: PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
                            pixels[i].x = (float)((unsigned char *)outputData)[i] / 255.0f;
                            pixels[i].y = (float)((unsigned char *)outputData)[i] / 255.0f;
                            pixels[i].z = (float)((unsigned char *)outputData)[i] / 255.0f;
                            pixels[i].w = 1.0f;
                            break;
                        }
                        case 2:
                        {
                            //NOTE: PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA
                            pixels[i].x = (float)((unsigned char *)outputData)[k] / 255.0f;
                            pixels[i].y = (float)((unsigned char *)outputData)[k] / 255.0f;
                            pixels[i].z = (float)((unsigned char *)outputData)[k] / 255.0f;
                            pixels[i].w = (float)((unsigned char *)outputData)[k + 1] / 255.0f;
                            k += 2;
                            break;
                        }
                        case 3:
                        {
                            //NOTE: PIXELFORMAT_UNCOMPRESSED_R8G8B8
                            pixels[i].x = (float)((unsigned char *)outputData)[k] / 255.0f;
                            pixels[i].y = (float)((unsigned char *)outputData)[k + 1] / 255.0f;
                            pixels[i].z = (float)((unsigned char *)outputData)[k + 2] / 255.0f;
                            pixels[i].w = 1.0f;
                            k += 3;
                            break;
                        }
                            InvalidDefaultCase
                        }
                    }

                    for (int i = 0, k = 0;
                         i < result.dataSize;
                         i += 4, k++)
                    {
                        ((unsigned char *)result.data)[i] = (unsigned char)(pixels[k].x * 255.0f);
                        ((unsigned char *)result.data)[i + 1] = (unsigned char)(pixels[k].y * 255.0f);
                        ((unsigned char *)result.data)[i + 2] = (unsigned char)(pixels[k].z * 255.0f);
                        ((unsigned char *)result.data)[i + 3] = (unsigned char)(pixels[k].w * 255.0f);
                    }
                }
                else
                {
                    memcpy(result.data, outputData, result.dataSize);
                }
            }
            else
            {
                //TODO: load invalid data anyway somehow
            }
        }
    }
    else
    {
        //TODO: Logging
    }

    return result;
}

void UploadAndReplaceTexture(BpImage *bpImage, Texture *texture)
{
    Assert(!IsZero(bpImage->dim));
    Assert(bpImage->imageFormat == IMAGE_FORMAT_R8G8B8A8);

    if (texture->id)
        rlUnloadTexture(texture->id);

    texture->id = rlLoadTexture(bpImage->data, bpImage->dim.x, bpImage->dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);

    texture->width = bpImage->dim.x;
    texture->height = bpImage->dim.y;
    texture->mipmaps = 1;
    texture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
}
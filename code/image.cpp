#pragma once

#include "headers.h"
#include <vcruntime_string.h>

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

unsigned int GetSizeOfRawRGBA32(V2 dim)
{
    unsigned int result = dim.x * dim.y * 4 * sizeof(unsigned char);
    return result;
}

unsigned int GetSizeOfRawRGBA32(unsigned int width, unsigned int height)
{
    unsigned int result = width * height * 4 * sizeof(unsigned char);
    return result;
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
                result.imageFormat = IMAGE_FORMAT_RAW_RGBA32;
                result.dim.x = width;
                result.dim.y = height;
                result.dataSize = GetSizeOfRawRGBA32(result.dim);
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

                stbi_image_free(outputData);
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

void WriteDataToBpImage(BpImage *bpImage, unsigned char *outData, unsigned int dataSize, IMAGE_FORMAT imageFormat, unsigned int width, unsigned int height, MemoryArena *arena)
{
    *bpImage = {};
    if (outData)
    {
        bpImage->dim = WidthHeightToV2(width, height);
        bpImage->dataSize = dataSize;
        bpImage->imageFormat = imageFormat;
        bpImage->data = PushSize(arena, bpImage->dataSize);
        memcpy(bpImage->data, outData, bpImage->dataSize);
    }
}

BpImage ConvertNewBpImage(BpImage *bpImage, IMAGE_FORMAT imageFormat, MemoryArena *arena)
{
    BpImage result = {};

    unsigned char *outData = {};
    unsigned int width = bpImage->dim.x;
    unsigned int height = bpImage->dim.y;
    int dataSize = {};

    switch (imageFormat)
    {
    case IMAGE_FORMAT_RAW_RGBA32:
    {
        dataSize = GetSizeOfRawRGBA32(bpImage->dim);
        switch (bpImage->imageFormat)
        {
        case IMAGE_FORMAT_RAW_RGBA32:
            WriteDataToBpImage(&result, (unsigned char *)bpImage->data, bpImage->dataSize, imageFormat, bpImage->dim.x, bpImage->dim.y, arena);
            break;
        case IMAGE_FORMAT_PNG_FINAL:
        {
            LodePNGColorType colorType = LCT_RGBA;
            unsigned int bitdepth = 8;

            LodePNGState state;
            lodepng_state_init(&state);
            state.info_raw.colortype = colorType;
            state.info_raw.bitdepth = bitdepth;
            state.info_png.color.colortype = colorType;
            state.info_png.color.bitdepth = bitdepth;
            state.decoder.ignore_crc = true;
            state.decoder.zlibsettings.ignore_adler32 = true;
            state.decoder.zlibsettings.ignore_nlen= true;
            
            lodepng_decode(&outData, &width, &height, &state, (const unsigned char *)bpImage->data, (size_t)dataSize);

            if (!state.error)
            {
                WriteDataToBpImage(&result, outData, dataSize, imageFormat, width, height, arena);
            }
            else
            {
                Print(lodepng_error_text(state.error));
            }

            lodepng_state_cleanup(&state);

            break;
        }
            InvalidDefaultCase
        }

        break;
    }
    case IMAGE_FORMAT_PNG_FINAL:
    {
        switch (bpImage->imageFormat)
        {
        case IMAGE_FORMAT_RAW_RGBA32:
        {
            int strideBytes = bpImage->dim.x * 4;
            outData = stbi_write_png_to_mem((unsigned char *)bpImage->data, strideBytes, bpImage->dim.x, bpImage->dim.y, 4, &dataSize);
            WriteDataToBpImage(&result, outData, dataSize, imageFormat, width, height, arena);
            stbi_image_free(outData);
            break;
        }
        case IMAGE_FORMAT_PNG_FINAL:
            WriteDataToBpImage(&result, (unsigned char *)bpImage->data, bpImage->dataSize, imageFormat, bpImage->dim.x, bpImage->dim.y, arena);
            break;
            InvalidDefaultCase
        }

        break;
    }
        InvalidDefaultCase
    }

    return result;
}

void UploadAndReplaceTexture(BpImage *bpImage, Texture *texture, MemoryArena *temporaryArena)
{
    Assert(!IsZero(bpImage->dim));

    BpImage bpImageToUpload = *bpImage;
    if (bpImage->imageFormat != IMAGE_FORMAT_RAW_RGBA32)
        bpImageToUpload = ConvertNewBpImage(bpImage, IMAGE_FORMAT_RAW_RGBA32, temporaryArena);

    if (bpImageToUpload.data)
    {
        if (texture->id)
            rlUnloadTexture(texture->id);

        *texture = {};

        if (bpImage->data)
        {
            texture->id = rlLoadTexture(bpImageToUpload.data, bpImageToUpload.dim.x, bpImageToUpload.dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
            texture->width = bpImageToUpload.dim.x;
            texture->height = bpImageToUpload.dim.y;
            texture->mipmaps = 1;
            texture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        }
    }
    else
    {
        //TODO: Log error
        // Print("Empty bpImage passed into texture uploader!");
    }
}

#pragma once

#if __clang__
#include "headers.h"
#endif

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

BpImage CreateEmptyBpImage(V2 dim, IMAGE_FORMAT imageFormat)
{
    BpImage result = {};
    result.dim = dim;
    result.imageFormat = imageFormat;
    return result;
}

BpImage MakeBpImageCopy(BpImage *bpImage, MemoryArena *arena)
{
    BpImage result = *bpImage;
    result.data = PushSize(arena, bpImage->dataSize);
    memcpy(result.data, bpImage->data, bpImage->dataSize);
    return result;
}

BpImage LoadDataIntoRawBpImage(const char *filePath, GameMemory *gameMemory)
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

void ConvertToRawRGBA32IfNot(BpImage *bpImage, MemoryArena *arena);

void PiratedSTB_EncodePngFilters(BpImage *bpImage, MemoryArena *arena)
{
    ConvertToRawRGBA32IfNot(bpImage, arena);

    int force_filter = stbi_write_force_png_filter;
    if (force_filter >= 5)
        force_filter = -1;

    int x = bpImage->dim.x;
    int y = bpImage->dim.y;
    int n = 4;
    int stride_bytes = x * 4;

    unsigned char *pixels = (unsigned char *)bpImage->data;
    unsigned int dataSize = (x * n + 1) * y;
    unsigned char *filters = (unsigned char *)PushSize(arena, dataSize);
    signed char *line_buffer = (signed char *)PushSize(arena, x * n);

    for (int j = 0; j < y; ++j)
    {
        int filter_type;
        if (force_filter > -1)
        {
            filter_type = force_filter;
            stbiw__encode_png_line((unsigned char *)(pixels), stride_bytes, x, y, j, n, force_filter, line_buffer);
        }
        else
        { // Estimate the best filter by running through all of them:
            int best_filter = 0, best_filter_val = 0x7fffffff, est, i;
            for (filter_type = 0; filter_type < 5; filter_type++)
            {
                stbiw__encode_png_line((unsigned char *)(pixels), stride_bytes, x, y, j, n, filter_type, line_buffer);

                // Estimate the entropy of the line using this filter; the less, the better.
                est = 0;
                for (i = 0; i < x * n; ++i)
                {
                    est += abs((signed char)line_buffer[i]);
                }
                if (est < best_filter_val)
                {
                    best_filter_val = est;
                    best_filter = filter_type;
                }
            }
            if (filter_type != best_filter)
            { // If the last iteration already got us the best filter, don't redo it
                stbiw__encode_png_line((unsigned char *)(pixels), stride_bytes, x, y, j, n, best_filter, line_buffer);
                filter_type = best_filter;
            }
        }
        // when we get here, filter_type contains the filter type, and line_buffer contains the data
        filters[j * (x * n + 1)] = (unsigned char)filter_type;
        STBIW_MEMMOVE(filters + j * (x * n + 1) + 1, line_buffer, x * n);
    }

    bpImage->data = filters;
    bpImage->dataSize = dataSize;
    bpImage->imageFormat = IMAGE_FORMAT_PNG_FILTERED;
}

void PiratedSTB_EncodePngCompression(BpImage *bpImage, MemoryArena *arena)
{
    if (bpImage->imageFormat != IMAGE_FORMAT_PNG_FILTERED)
        PiratedSTB_EncodePngFilters(bpImage, arena);

    int x = bpImage->dim.x;
    int y = bpImage->dim.y;
    int n = 4;
    unsigned char *filt = (unsigned char *)bpImage->data;

    int dataSize = {};

    //TODO: break apart so we don't have another copy here
    unsigned char *out = stbi_zlib_compress(filt, y * (x * n + 1), &dataSize, stbi_write_png_compression_level);

    bpImage->data = PushSize(arena, dataSize);
    memcpy(bpImage->data, out, dataSize);

    bpImage->dataSize = dataSize;
    bpImage->imageFormat = IMAGE_FORMAT_PNG_COMPRESSED;
}

void PiratedSTB_EncodePngCRC(BpImage *bpImage, MemoryArena *arena)
{
    if (bpImage->imageFormat != IMAGE_FORMAT_PNG_COMPRESSED)
        PiratedSTB_EncodePngCompression(bpImage, arena);

    unsigned int zlen = bpImage->dataSize;
    int x = bpImage->dim.x;
    int y = bpImage->dim.y;

    // each tag requires 12 bytes of overhead
    unsigned int outLength = 8 + 12 + 13 + 12 + zlen + 12;
    unsigned char *out = (unsigned char *)PushSize(arena, outLength);

    int ctype[5] = {-1, 0, 4, 2, 6};
    unsigned char sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    unsigned char *o = out;
    STBIW_MEMMOVE(o, sig, 8);
    o += 8;
    stbiw__wp32(o, 13); // header length
    stbiw__wptag(o, "IHDR");
    stbiw__wp32(o, x);
    stbiw__wp32(o, y);
    *o++ = 8;
    *o++ = STBIW_UCHAR(ctype[4]);
    *o++ = 0;
    *o++ = 0;
    *o++ = 0;
    stbiw__wpcrc(&o, 13);

    stbiw__wp32(o, zlen);
    stbiw__wptag(o, "IDAT");
    STBIW_MEMMOVE(o, bpImage->data, zlen);
    o += zlen;
    stbiw__wpcrc(&o, zlen);

    stbiw__wp32(o, 0);
    stbiw__wptag(o, "IEND");
    stbiw__wpcrc(&o, 0);

    STBIW_ASSERT(o == out + outLength);

    bpImage->data = out;
    bpImage->dataSize = outLength;
    bpImage->imageFormat = IMAGE_FORMAT_PNG_FINAL;
}

void PiratedSTB_EncodePng(BpImage *bpImage, MemoryArena *arena)
{
    PiratedSTB_EncodePngFilters(bpImage, arena);
    PiratedSTB_EncodePngCompression(bpImage, arena);
    PiratedSTB_EncodePngCRC(bpImage, arena);
}

void DecodePng(BpImage *bpImage, MemoryArena *arena)
{
    Assert(bpImage->imageFormat == IMAGE_FORMAT_PNG_FINAL);
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
    state.decoder.zlibsettings.ignore_nlen = true;

    unsigned char *outData = {};
    unsigned int width = bpImage->dim.x;
    unsigned int height = bpImage->dim.y;
    lodepng_decode(&outData, &width, &height, &state, (const unsigned char *)bpImage->data, (size_t)bpImage->dataSize);

    if (!state.error)
    {
        unsigned int rawRGBA32DataSize = GetSizeOfRawRGBA32(bpImage->dim);
        if (outData)
        {
            bpImage->dataSize = rawRGBA32DataSize;
            bpImage->imageFormat = IMAGE_FORMAT_RAW_RGBA32;
            bpImage->data = PushSize(arena, bpImage->dataSize);
            memcpy(bpImage->data, outData, bpImage->dataSize);
        }
    }
    else
    {
        Print(lodepng_error_text(state.error));
    }

    lodepng_state_cleanup(&state);
}

void ConvertToRawRGBA32IfNot(BpImage *bpImage, MemoryArena *arena)
{
    if (bpImage->imageFormat != IMAGE_FORMAT_RAW_RGBA32)
    {
        switch (bpImage->imageFormat)
        {
        case IMAGE_FORMAT_PNG_FILTERED:
        {
            PiratedSTB_EncodePngCompression(bpImage, arena);
            PiratedSTB_EncodePngCRC(bpImage, arena);
            DecodePng(bpImage, arena);
            break;
        }
        case IMAGE_FORMAT_PNG_COMPRESSED:
        {
            PiratedSTB_EncodePngCRC(bpImage, arena);
            DecodePng(bpImage, arena);
            break;
        }
        case IMAGE_FORMAT_PNG_FINAL:
        {
            DecodePng(bpImage, arena);
            break;
        }
            InvalidDefaultCase
        }
    }
}

void ConvertNewBpImage(BpImage *bpImage, IMAGE_FORMAT imageFormat, MemoryArena *temporaryArena)
{
    if (bpImage->imageFormat != imageFormat)
    {
        switch (imageFormat)
        {
        case IMAGE_FORMAT_PNG_FILTERED:
            PiratedSTB_EncodePngFilters(bpImage, temporaryArena);
            break;
        case IMAGE_FORMAT_PNG_COMPRESSED:
            PiratedSTB_EncodePngCompression(bpImage, temporaryArena);
            break;
        case IMAGE_FORMAT_PNG_FINAL:
            PiratedSTB_EncodePng(bpImage, temporaryArena);
            break;
        case IMAGE_FORMAT_RAW_RGBA32:
            //No conversion needed
            break;
            InvalidDefaultCase
        }
    }
}

void UploadAndReplaceTexture(BpImage *bpImage, Texture *texture)
{
    Assert(bpImage->data);
    Assert(bpImage->imageFormat == IMAGE_FORMAT_RAW_RGBA32);

    if (texture->id)
        rlUnloadTexture(texture->id);
    *texture = {};

    texture->id = rlLoadTexture(bpImage->data, bpImage->dim.x, bpImage->dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    texture->width = bpImage->dim.x;
    texture->height = bpImage->dim.y;
    texture->mipmaps = 1;
    texture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    // GenTextureMipmaps(texture);
}

void UploadTexture(Texture *texture, void *data, int width, int height)
{
    if (data)
    {
        if (texture->id)
        {
            rlUpdateTexture(texture->id, 0, 0, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, data);
        }
        else
        {
            texture->id = rlLoadTexture(data, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
            texture->width = width;
            texture->height = height;
            texture->mipmaps = 1;
            texture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        }
        GenTextureMipmaps(texture);
    }
    else
    {
        //TODO: Log error
        Print("Empty image passed into texture uploader!");
    }
}

void InitializeCanvas(Canvas *canvas, BpImage *rootBpImage, GameMemory *gameMemory)
{
    if (canvas->texture.id)
        UnloadTexture(canvas->texture);

    float closestSquare = {};

    V2 canvsDim = {};

    BpImage tempImage = MakeBpImageCopy(rootBpImage, &gameMemory->temporaryArena);
    ConvertNewBpImage(&tempImage, IMAGE_FORMAT_PNG_FILTERED, &gameMemory->temporaryArena);
    // closestSquare = Round(SqrtFloat(tempImage.dataSize));
    // closestSquare = Round(SqrtFloat(tempImage.dataSize));

    // float dataRatio = (float)tempImage.dataSize / (float)rootBpImage->dataSize;

    // V2 canvasDim = rootBpImage->dim * dataRatio;
    V2 canvasDim = V2{rootBpImage->dim.x * 4, rootBpImage->dim.y} + 1;
    // V2 canvasDim = V2{tempImage.dim.x, tempImage.dim.y};

    // float pixelCount = tempImage.dataSize;
    float pixelCount = rootBpImage->dataSize;

    ResetMemoryArena(&gameMemory->canvasArena);
    Color *pixelsRootImage = PushArray(&gameMemory->canvasArena, pixelCount, Color);
    Color *pixelsDrawn = PushArray(&gameMemory->canvasArena, pixelCount, Color);

    for (int i = 0;
         i < pixelCount;
         i++)
    {
        // unsigned char value = ((unsigned char *)rootBpImage->data)[i];
        unsigned char value = ((unsigned char *)tempImage.data)[i];
        // value = 255 - value;
        pixelsRootImage[i] = Color{value, value, value, 255};
    }

    ZeroArrayType(pixelsDrawn, pixelCount, Color);

    canvas->rootImageData.data = pixelsRootImage;
    canvas->rootImageData.width = canvasDim.x;
    canvas->rootImageData.height = canvasDim.y;
    canvas->rootImageData.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    canvas->rootImageData.mipmaps = 1;

    canvas->drawnImageData.data = pixelsDrawn;
    canvas->drawnImageData.width = canvasDim.x;
    canvas->drawnImageData.height = canvasDim.y;
    canvas->drawnImageData.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    canvas->drawnImageData.mipmaps = 1;

    if (canvas->texture.id)
        rlUnloadTexture(canvas->texture.id);
    canvas->texture = {};

    canvas->needsTextureUpload = true;
}

void InitializeNewImage(const char *fileName, GameMemory *gameMemory, BpImage *rootBpImage, Canvas *canvas, Texture *loadedTexture)
{
    *rootBpImage = LoadDataIntoRawBpImage(fileName, gameMemory);
    if (rootBpImage->data)
    {
        UploadAndReplaceTexture(rootBpImage, loadedTexture);
        InitializeCanvas(canvas, rootBpImage, gameMemory);
    }
}

void UpdateBpImage(ProcessedImage *processedImage)
{
    MemoryArena *arena = &processedImage->workArena;
    BpImage tempImage = MakeBpImageCopy(processedImage->rootBpImage, arena);

    ConvertNewBpImage(&tempImage, IMAGE_FORMAT_PNG_FILTERED, arena);

    Canvas *canvas = processedImage->canvas;

    Assert(processedImage->canvas->rootImageData.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Assert(processedImage->canvas->drawnImageData.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

#if 1
    for (int i = 0;
         i < tempImage.dataSize;
         i += 1)
    {
        // Assert(i < canvas->image.height * canvas->image.width * 4);
        Color canvasPixel = ((Color *)canvas->drawnImageData.data)[i];

        if (canvasPixel.a)
        {
            ((unsigned char *)tempImage.data)[i] = 0;
            // ((unsigned char *)tempImage.data)[i + 1] = 0;
            // ((unsigned char *)tempImage.data)[i + 2] = 0;
            // ((unsigned char *)tempImage.data)[i + 3] = 0;
        }
    }
#endif

    ConvertToRawRGBA32IfNot(&tempImage, arena);

    processedImage->finalProcessedImage = tempImage;
}

void ResetProcessedImage(ProcessedImage *processedImage, Canvas *canvas, MemoryArena *temporaryArena)
{
    ResetMemoryArena(&processedImage->workArena);
    processedImage->active = false;
    processedImage->frameStarted = 0;
    processedImage->frameFinished = 0;
    processedImage->finalProcessedImage = {};

    unsigned int pixelCount = canvas->drawnImageData.width * canvas->drawnImageData.height;
    for (int i = 0;
         i <= pixelCount;
         i++)
    {
        Color *drawnPixel = &((Color *)canvas->drawnImageData.data)[i];
        if ((drawnPixel->r || drawnPixel->g || drawnPixel->b) && drawnPixel->a == processedImage->index)
            drawnPixel->a = 255;
    }

    canvas->needsTextureUpload = true;
}

ProcessedImage *GetFreeProcessedImage(ProcessedImage *processedImages, unsigned int threadCount)
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
    return processedImage;
}

PLATFORM_WORK_QUEUE_CALLBACK(ProcessImageOnThread)
{
    ProcessedImage *processedImage = (ProcessedImage *)data;
    Assert(processedImage);

    UpdateBpImage(processedImage);
    processedImage->frameFinished = G_CURRENT_FRAME;
    Print("Finished");
}

void StartProcessedImageWork(Canvas *canvas, unsigned int threadCount, ProcessedImage *processedImage, PlatformWorkQueue *threadWorkQueue)
{
    unsigned int pixelCount = canvas->drawnImageData.width * canvas->drawnImageData.height;
    for (int i = 0;
         i <= pixelCount;
         i++)
    {
        Color *drawnPixel = &((Color *)canvas->drawnImageData.data)[i];
        if (drawnPixel->a == threadCount + 1)
            drawnPixel->a = processedImage->index;
    }

    canvas->proccessAsap = false;

    Print("Starting Work");
    processedImage->frameStarted = G_CURRENT_FRAME;
    processedImage->active = true;
    PlatformAddThreadWorkEntry(threadWorkQueue, ProcessImageOnThread, (void *)processedImage);
}

bool ExportImage(Image image, String filepath)
{
    bool result = false;
    int channels = 4;

    //HACK: append png if there's no file path lol
    //TODO: the platform layer should handle this not the game
    String fileExtention = CreateString(GetFileExtension(filepath.chars));
    if (!fileExtention.length)
        filepath += ".png";

    fileExtention = CreateString(GetFileExtension(filepath.chars));

    if (fileExtention == ".png")
    {
        int dataSize = 0;
        unsigned char *fileData = stbi_write_png_to_mem((const unsigned char *)image.data, image.width * channels, image.width, image.height, channels, &dataSize);
        result = SaveFileData(filepath.chars, fileData, dataSize);
        RL_FREE(fileData);
    }
    else if (fileExtention == ".bmp")
    {
        result = stbi_write_bmp(filepath.chars, image.width, image.height, channels, image.data);
    }
    else if (fileExtention == ".jpg" || fileExtention == ".jpeg")
    {
        result = stbi_write_jpg(filepath.chars, image.width, image.height, channels, image.data, 100); // JPG quality: between 1 and 100
    }
    else 
    {
        // Export raw pixel data (without header)
        // NOTE: It's up to the user to track image parameters
        result = SaveFileData(filepath.chars, image.data, GetPixelDataSize(image.width, image.height, image.format));
    }

    return result;
}
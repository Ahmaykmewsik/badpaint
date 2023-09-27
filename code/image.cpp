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
    bpImage->data = stbi_zlib_compress(filt, y * (x * n + 1), &dataSize, stbi_write_png_compression_level);
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

void UploadAndReplaceTexture(BpImage *bpImage, Texture *texture, MemoryArena *temporaryArena)
{
    Assert(!IsZero(bpImage->dim));

    ConvertToRawRGBA32IfNot(bpImage, temporaryArena);

    if (bpImage->data)
    {
        if (texture->id)
            rlUnloadTexture(texture->id);

        *texture = {};

        if (bpImage->data)
        {
            texture->id = rlLoadTexture(bpImage->data, bpImage->dim.x, bpImage->dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
            texture->width = bpImage->dim.x;
            texture->height = bpImage->dim.y;
            texture->mipmaps = 1;
            texture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            // GenTextureMipmaps(texture);
        }
    }
    else
    {
        //TODO: Log error
        Print("Empty bpImage passed into texture uploader!");
    }
}

void UpdateTexture(Image *image, Texture *texture)
{
    if (image->data)
    {
        if (texture->id)
        {
            rlUpdateTexture(texture->id, 0, 0, image->width, image->height, image->format, image->data);
        }
        else
        {
            texture->id = rlLoadTexture(image->data, image->width, image->height, image->format, 1);
            texture->width = image->width;
            texture->height = image->height;
            texture->mipmaps = 1;
            texture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        }
        // GenTextureMipmaps(texture);
    }
    else
    {
        //TODO: Log error
        Print("Empty image passed into texture uploader!");
    }
}

void InitializeCanvas(Image *canvasImage, Texture *canvasTexture, BpImage *rootBpImage)
{
    if (canvasImage->data)
        UnloadImage(*canvasImage);

    *canvasImage = GenImageColor(rootBpImage->dim.x, rootBpImage->dim.y, WHITE);
    if (canvasTexture->id)
    {
        rlUnloadTexture(canvasTexture->id);
        *canvasTexture = {};
    }

    UpdateTexture(canvasImage, canvasTexture);
}

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
                int dataSize = GetSizeOfRawRGBA32(WidthHeightToV2(width, height));
                if (dataSize < 30000000)
                {
                    ResetMemoryArena(&gameMemory->rootImageArena);
                    result.dim.x = width;
                    result.dim.y = height;
                    result.dataSize = dataSize;
                    result.imageFormat = IMAGE_FORMAT_RAW_RGBA32;
                    result.imageFormat = IMAGE_FORMAT_RAW_RGBA32;
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
                    String notification = CreateString("Geez!!! I am NOT touching that! That's WAY too big! I would crash!!!");
                    InitNotificationMessage(notification, &gameMemory->circularScratchBuffer);
                }
            }
            else
            {
                String notification = CreateString("I don't recognize that as an image. In the future you'll be able to load arbitrary data, but not yet.");
                InitNotificationMessage(notification, &gameMemory->circularScratchBuffer);
            }

            stbi_image_free(outputData);
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
    // int n = 4;
    unsigned char *filt = (unsigned char *)bpImage->data;

    int dataSize = {};

    unsigned char *data = filt;
    int data_len = y * (x * 4 + 1);
    int *out_len = &dataSize;
    int quality = stbi_write_png_compression_level;

    static unsigned short lengthc[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 259};
    static unsigned char lengtheb[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static unsigned short distc[] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 32768};
    static unsigned char disteb[] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    unsigned int bitbuf = 0;
    int i, j, bitcount = 0;
    unsigned char *out = NULL;
    unsigned char ***hash_table = (unsigned char ***)PushSize(arena, stbiw__ZHASH * sizeof(unsigned char **));

    if (quality < 5)
        quality = 5;

    stbiw__sbpush(out, 0x78); // DEFLATE 32K window
    stbiw__sbpush(out, 0x5e); // FLEVEL = 1
    stbiw__zlib_add(1, 1);    // BFINAL = 1
    stbiw__zlib_add(1, 2);    // BTYPE = 1 -- fixed huffman

    for (i = 0; i < stbiw__ZHASH; ++i)
        hash_table[i] = NULL;

    //  Print("STB COMPRESSION -- START WHILE LOOP");
    i = 0;
    while (i < data_len - 3)
    {
        // hash next 3 bytes of data to be compressed
        int h = stbiw__zhash(data + i) & (stbiw__ZHASH - 1), best = 3;
        unsigned char *bestloc = 0;
        unsigned char **hlist = hash_table[h];
        int n = stbiw__sbcount(hlist);
        for (j = 0; j < n; ++j)
        {
            if (hlist[j] - data > i - 32768)
            { // if entry lies within window
                int d = stbiw__zlib_countm(hlist[j], data + i, data_len - i);
                if (d >= best)
                {
                    best = d;
                    bestloc = hlist[j];
                }
            }
        }
        // when hash table entry is too long, delete half the entries
        if (hash_table[h] && stbiw__sbn(hash_table[h]) == 2 * quality)
        {
            STBIW_MEMMOVE(hash_table[h], hash_table[h] + quality, sizeof(hash_table[h][0]) * quality);
            stbiw__sbn(hash_table[h]) = quality;
        }
        stbiw__sbpush(hash_table[h], data + i);

        if (bestloc)
        {
            // "lazy matching" - check match at *next* byte, and if it's better, do cur byte as literal
            h = stbiw__zhash(data + i + 1) & (stbiw__ZHASH - 1);
            hlist = hash_table[h];
            n = stbiw__sbcount(hlist);
            for (j = 0; j < n; ++j)
            {
                if (hlist[j] - data > i - 32767)
                {
                    int e = stbiw__zlib_countm(hlist[j], data + i + 1, data_len - i - 1);
                    if (e > best)
                    { // if next match is better, bail on current match
                        bestloc = NULL;
                        break;
                    }
                }
            }
        }

        if (bestloc)
        {
            int d = (int)(data + i - bestloc); // distance back
            STBIW_ASSERT(d <= 32767 && best <= 258);
            for (j = 0; best > lengthc[j + 1] - 1; ++j)
                ;
            stbiw__zlib_huff(j + 257);
            if (lengtheb[j])
                stbiw__zlib_add(best - lengthc[j], lengtheb[j]);
            for (j = 0; d > distc[j + 1] - 1; ++j)
                ;
            stbiw__zlib_add(stbiw__zlib_bitrev(j, 5), 5);
            if (disteb[j])
                stbiw__zlib_add(d - distc[j], disteb[j]);
            i += best;
        }
        else
        {
            stbiw__zlib_huffb(data[i]);
            ++i;
        }
    }

    //  Print("STB COMPRESSION -- WRITE FINAL BYTES");
    // write out final bytes
    for (; i < data_len; ++i)
        stbiw__zlib_huffb(data[i]);
    stbiw__zlib_huff(256); // end of block
    // pad with 0 bits to byte boundary
    while (bitcount)
        stbiw__zlib_add(0, 1);

#if 0
    for (i = 0; i < stbiw__ZHASH; ++i)
        (void)stbiw__sbfree(hash_table[i]);
    STBIW_FREE(hash_table);
#endif

    // store uncompressed instead if compression was worse
    if (stbiw__sbn(out) > data_len + 2 + ((data_len + 32766) / 32767) * 5)
    {
        stbiw__sbn(out) = 2; // truncate to DEFLATE 32K window and FLEVEL = 1
        for (j = 0; j < data_len;)
        {
            int blocklen = data_len - j;
            if (blocklen > 32767)
                blocklen = 32767;
            stbiw__sbpush(out, data_len - j == blocklen); // BFINAL = ?, BTYPE = 0 -- no compression
            stbiw__sbpush(out, STBIW_UCHAR(blocklen));    // LEN
            stbiw__sbpush(out, STBIW_UCHAR(blocklen >> 8));
            stbiw__sbpush(out, STBIW_UCHAR(~blocklen)); // NLEN
            stbiw__sbpush(out, STBIW_UCHAR(~blocklen >> 8));
            memcpy(out + stbiw__sbn(out), data + j, blocklen);
            stbiw__sbn(out) += blocklen;
            j += blocklen;
        }
    }

    {
        // compute adler32 on input
        unsigned int s1 = 1, s2 = 0;
        int blocklen = (int)(data_len % 5552);
        j = 0;
        while (j < data_len)
        {
            for (i = 0; i < blocklen; ++i)
            {
                s1 += data[j + i];
                s2 += s1;
            }
            s1 %= 65521;
            s2 %= 65521;
            j += blocklen;
            blocklen = 5552;
        }
        stbiw__sbpush(out, STBIW_UCHAR(s2 >> 8));
        stbiw__sbpush(out, STBIW_UCHAR(s2));
        stbiw__sbpush(out, STBIW_UCHAR(s1 >> 8));
        stbiw__sbpush(out, STBIW_UCHAR(s1));
    }
    *out_len = stbiw__sbn(out);
    // make returned pointer freeable
    STBIW_MEMMOVE(stbiw__sbraw(out), out, *out_len);

    unsigned char *dataOut = (unsigned char *)stbiw__sbraw(out);

    // Print("FINISHED STB COMPRESSION");

    // int dataSize = out_len;
    bpImage->data = PushSize(arena, dataSize);
    memcpy(bpImage->data, dataOut, dataSize);

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
            bpImage->data = PushSize(arena, bpImage->dataSize);
            memcpy(bpImage->data, outData, bpImage->dataSize);
            lodepng_free(outData);
        }
    }
    else
    {
        bpImage->dataSize = 0;
        bpImage->data = {};
        bpImage->dim = {};
        Print(lodepng_error_text(state.error));
    }

    bpImage->imageFormat = IMAGE_FORMAT_RAW_RGBA32;

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

    Assert(bpImage->imageFormat == IMAGE_FORMAT_RAW_RGBA32);
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
        // Print("Empty image passed into texture uploader!");
    }
}

unsigned int GetCanvasDatasize(Canvas *canvas)
{
    V2 dim = WidthHeightToV2(canvas->drawnImageData.width, canvas->drawnImageData.height);
    unsigned int result = GetSizeOfRawRGBA32(dim);
    return result;
}

BpImage CreateDataImage(BpImage *rootBpImage, BpImage finalImage, GameMemory *gameMemory)
{
    BpImage result = {};

    if (!finalImage.data)
    {
        finalImage = MakeBpImageCopy(rootBpImage, &gameMemory->temporaryArena);
        ConvertNewBpImage(&finalImage, IMAGE_FORMAT_PNG_FILTERED, &gameMemory->temporaryArena);
    }

    float pixelCount = rootBpImage->dataSize;
    ResetMemoryArena(&gameMemory->latestCompletedImageArena);
    Color *pixelsRootImage = PushArray(&gameMemory->latestCompletedImageArena, pixelCount, Color);
    for (int i = 0;
         i < pixelCount;
         i++)
    {
        unsigned char value = ((unsigned char *)finalImage.data)[i];
        pixelsRootImage[i] = Color{value, value, value, 255};
    }

    result.data = pixelsRootImage;
    result.dim = V2{rootBpImage->dim.x * 4, rootBpImage->dim.y} + 1;
    result.imageFormat = IMAGE_FORMAT_RAW_RGBA32;
    result.dataSize = pixelCount * sizeof(Color); //NOTE: sizeof(unsigned char) * 4

    return result;
}

void InitializeCanvas(Canvas *canvas, BpImage *rootBpImage, Brush *brush, GameMemory *gameMemory)
{
    if (canvas->texture.id)
        UnloadTexture(canvas->texture);

    float closestSquare = {};

    V2 canvsDim = {};

    BpImage tempImage = MakeBpImageCopy(rootBpImage, &gameMemory->temporaryArena);
    ConvertNewBpImage(&tempImage, IMAGE_FORMAT_PNG_FILTERED, &gameMemory->temporaryArena);
    V2 canvasDim = V2{rootBpImage->dim.x * 4, rootBpImage->dim.y} + 1;
    float pixelCount = rootBpImage->dataSize;

    ResetMemoryArena(&gameMemory->canvasArena);
    Color *pixelsDrawn = PushArray(&gameMemory->canvasArena, pixelCount, Color);

    ZeroArrayType(pixelsDrawn, pixelCount, Color);

    canvas->drawnImageData.data = pixelsDrawn;
    canvas->drawnImageData.width = canvasDim.x;
    canvas->drawnImageData.height = canvasDim.y;
    canvas->drawnImageData.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    canvas->drawnImageData.mipmaps = 1;

    if (canvas->texture.id)
        rlUnloadTexture(canvas->texture.id);
    canvas->texture = {};

    canvas->needsTextureUpload = true;

    ResetMemoryArena(&gameMemory->canvasRollbackArena);
    unsigned int canvasSize = GetCanvasDatasize(canvas);
    canvas->rollbackSizeCount = Floor((float)gameMemory->canvasRollbackArena.size / canvasSize) - 1;
    canvas->rollbackImageData = PushArray(&gameMemory->canvasRollbackArena, canvas->rollbackSizeCount * canvasSize, unsigned char);
    canvas->rollbackIndexStart = 0;
    canvas->rollbackIndexNext = 0;

    canvas->brush = brush;
}

void InitializeNewImage(const char *fileName, GameMemory *gameMemory, BpImage *rootBpImage, BpImage *latestCompletedBpImage, Canvas *canvas, Texture *loadedTexture, Brush *currentBrush)
{
    *rootBpImage = LoadDataIntoRawBpImage(fileName, gameMemory);
    if (rootBpImage->data)
    {
        UploadAndReplaceTexture(rootBpImage, loadedTexture);
        InitializeCanvas(canvas, rootBpImage, currentBrush, gameMemory);
        *latestCompletedBpImage = CreateDataImage(rootBpImage, {}, gameMemory);
    }
}

void UpdateBpImageOnThread(ProcessedImage *processedImage)
{
    // Print("Staring Work on thread " + IntToString(processedImage->index));

    MemoryArena *arena = &processedImage->workArena;

    BpImage *convertedImage = &processedImage->convertedImage;
    *convertedImage = MakeBpImageCopy(processedImage->rootBpImage, arena);

    ConvertNewBpImage(&processedImage->convertedImage, IMAGE_FORMAT_PNG_FILTERED, arena);

    Canvas *canvas = processedImage->canvas;

    Assert(processedImage->canvas->drawnImageData.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    for (int i = 0;
         i < convertedImage->dataSize;
         i += 1)
    {
        Color canvasPixel = ((Color *)canvas->drawnImageData.data)[i];

        if (canvasPixel.r)
        {
            switch (canvasPixel.r)
            {
            case BRUSH_EFFECT_REMOVE:
            {
                ((unsigned char *)convertedImage->data)[i] = 0;
                break;
            }
            case BRUSH_EFFECT_MAX:
            {
                ((unsigned char *)convertedImage->data)[i] = 255;
                break;
            }
            case BRUSH_EFFECT_SHIFT:
            {
                int shiftAmount = 36;
                if (i < convertedImage->dataSize - shiftAmount)
                    ((unsigned char *)convertedImage->data)[i] = ((unsigned char *)convertedImage->data)[i + shiftAmount];
                break;
            }
            case BRUSH_EFFECT_RANDOM:
            {
                ((unsigned char *)convertedImage->data)[i] = canvasPixel.g;
                break;
            }
            case BRUSH_EFFECT_ERASE_EFFECT:
                break;
                InvalidDefaultCase
            }
        }
    }

    processedImage->finalProcessedBpImage = MakeBpImageCopy(convertedImage, arena);
    // Print("Made copy of canvas edit on thead " + IntToString(processedImage->index));

    // ConvertToRawRGBA32IfNot(&processedImage->finalProcessedBpImage, arena);
    PiratedSTB_EncodePngCompression(&processedImage->finalProcessedBpImage, arena);

    // Print("Encoded PNG COmpression on thread " + IntToString(processedImage->index));
    PiratedSTB_EncodePngCRC(&processedImage->finalProcessedBpImage, arena);
    // Print("Encoded PNG CRC on thread " + IntToString(processedImage->index));
    DecodePng(&processedImage->finalProcessedBpImage, arena);
    // Print("Decoded PNG on thread " + IntToString(processedImage->index));

    // Print("Converted to final PNG on thead " + IntToString(processedImage->index));
}

void ResetProcessedImage(ProcessedImage *processedImage, Canvas *canvas, MemoryArena *temporaryArena)
{
    ResetMemoryArena(&processedImage->workArena);
    processedImage->active = false;
    processedImage->frameStarted = 0;
    processedImage->frameFinished = 0;
    processedImage->finalProcessedBpImage = {};

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

    UpdateBpImageOnThread(processedImage);
    processedImage->frameFinished = G_CURRENT_FRAME;
    // Print("Finished on thread " + IntToString(processedImage->index));
}

void StartProcessedImageWork(Canvas *canvas, unsigned int threadCount, ProcessedImage *processedImage, PlatformWorkQueue *threadWorkQueue)
{
    unsigned int pixelCount = canvas->drawnImageData.width * canvas->drawnImageData.height;
    for (int i = 0;
         i <= pixelCount;
         i++)
    {
        Color *drawnPixel = &((Color *)canvas->drawnImageData.data)[i];
        if (drawnPixel->a == threadCount + 1 || ((drawnPixel->r || drawnPixel->g || drawnPixel->b) && drawnPixel->a != 255 && canvas->oldDataPresent))
            drawnPixel->a = processedImage->index;
    }

    canvas->proccessAsap = false;
    canvas->oldDataPresent = false;

    // Print("Queueing Work on thread " + IntToString(processedImage->index));
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
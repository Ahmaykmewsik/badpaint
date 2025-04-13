#pragma once

#if __clang__
#include "headers.h"
#endif

unsigned char *LoadDataFromDisk(const char *fileName, unsigned int *bytesRead, Arena *arena)
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
			data = ARENA_PUSH_ARRAY(arena, size, unsigned char);

			// NOTE: fread() returns number of read elements instead of bytes, so we read [1 byte, size elements]
			unsigned int count = (unsigned int)fread(data, sizeof(unsigned char), size, file);
			*bytesRead = count;

			//TODO: (Ahmayk) Log this stuff ourselves
			if (count != (u32) size)
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

unsigned int GetSizeOfRawRGBA32(iv2 dim)
{
	unsigned int result = dim.x * dim.y * 4 * sizeof(unsigned char);
	return result;
}

ImageRawRGBA32 LoadDataIntoRawImage(const char *filePath, GameMemory *gameMemory)
{
	ImageRawRGBA32 result = {};

	ArenaMarker loadMarker = ArenaPushMarker(&gameMemory->temporaryArena);
	unsigned int fileSize = {};
	unsigned char *fileData = LoadDataFromDisk(filePath, &fileSize, &gameMemory->temporaryArena);
	const char *getFileExtension = GetFileExtension(filePath);

	int comp = 0;
	int width = 0;
	int height = 0;
	void *outputData = {};

	if (fileData != NULL)
	{
		outputData = stbi_load_from_memory(fileData, fileSize, &width, &height, &comp, 0);
	}
	else
	{
		String notification = STRING("Oops! I failed to load that! Sorry! Guess you're out of luck pal. No badpaint for you today.");
		InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
	}

	ArenaPopMarker(loadMarker);

	if (outputData != NULL)
	{
		int dataSize = GetSizeOfRawRGBA32(iv2{width, height});
		if (dataSize < MegaByte * 50)
		{
			ArenaFree(&gameMemory->rootImageArena);
			gameMemory->rootImageArena = ArenaInit(dataSize);

			result.dim.x = width;
			result.dim.y = height;
			result.dataSize = dataSize;
			result.dataU8 = (u8*) ArenaPushSize(&gameMemory->rootImageArena, result.dataSize, {});

			if (comp != 4)
			{
				ArenaMarker marker = {};
				v4 *pixels = ARENA_PUSH_ARRAY_MARKER(&gameMemory->temporaryArena, width * height, v4, &marker);
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

				for (u32 i = 0, k = 0; i < result.dataSize; i += 4, k++)
				{
					result.dataU8[i] = (unsigned char)(pixels[k].x * 255.0f);
					result.dataU8[i + 1] = (unsigned char)(pixels[k].y * 255.0f);
					result.dataU8[i + 2] = (unsigned char)(pixels[k].z * 255.0f);
					result.dataU8[i + 3] = (unsigned char)(pixels[k].w * 255.0f);
				}

				ArenaPopMarker(marker);
			}
			else
			{
				memcpy(result.dataU8, outputData, result.dataSize);
			}
		}
		else
		{
			String notification = STRING("Geez!!! I am NOT touching that! That's WAY too big! I would crash!!!");
			InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
		}

		stbi_image_free(outputData);
	}
	else
	{
		String notification = STRING("I don't recognize that as an image. In the future you'll be able to load arbitrary data, but not yet.");
		InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
	}

	return result;
}

ImagePNGFiltered PiratedSTB_EncodePngFilters(ImageRawRGBA32 *imageRaw, Arena *arena, PNG_FILTER_TYPE pngFilterType)
{
	ImagePNGFiltered result = {};

	int x = imageRaw->dim.x;
	int y = imageRaw->dim.y;
	int n = 4;
	int stride_bytes = x * 4;

	result.dataSize = (x * n + 1) * y;
	result.dim = imageRaw->dim;
	u8 *filters = (u8 *)ArenaPushSize(arena, result.dataSize, {});
	i8 *line_buffer = (signed char *)ArenaPushSize(arena, x * n, {});

	i32 force_filter = (i32) pngFilterType;
	if (force_filter >= 5)
	{
		force_filter = -1;
	}

	for (int j = 0; j < y; ++j)
	{
		int filter_type;
		if (force_filter > -1)
		{
			filter_type = force_filter;
			stbiw__encode_png_line(imageRaw->dataU8, stride_bytes, x, y, j, n, force_filter, line_buffer);
		}
		else
		{ // Estimate the best filter by running through all of them:
			int best_filter = 0, best_filter_val = 0x7fffffff, est, i;
			for (filter_type = 0; filter_type < 5; filter_type++)
			{
				stbiw__encode_png_line(imageRaw->dataU8, stride_bytes, x, y, j, n, filter_type, line_buffer);

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
				stbiw__encode_png_line(imageRaw->dataU8, stride_bytes, x, y, j, n, best_filter, line_buffer);
				filter_type = best_filter;
			}
		}
		// when we get here, filter_type contains the filter type, and line_buffer contains the data
		filters[j * (x * n + 1)] = (u8) filter_type;
		STBIW_MEMMOVE(filters + j * (x * n + 1) + 1, line_buffer, x * n);
	}
	result.dataU8 = filters;
	result.pngFilterType = pngFilterType;

	return result;
}

ImageRawRGBA32 PiratedLoadPNG_Defilter(ImagePNGFiltered *imagePNGFiltered, Arena *arena)
{
	ImageRawRGBA32 result = {};;

	u32 rawRGBA32DataSize = GetSizeOfRawRGBA32(imagePNGFiltered->dim);
	u8 *out = (u8*) ArenaPushSize(arena, rawRGBA32DataSize, {});

	u8 bpp = 32;
	u32 w = imagePNGFiltered->dim.x;
	u32 h = imagePNGFiltered->dim.y;
	u8 *in = imagePNGFiltered->dataU8;

	unsigned y;
	unsigned char* prevline = 0;

	/*bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise*/
	size_t bytewidth = (bpp + 7u) / 8u;
	/*the width of a scanline in bytes, not including the filter type*/
	size_t linebytes = lodepng_get_raw_size_idat(w, 1, bpp) - 1u;

	u32 unfilterResult = 0;
	for(y = 0; y < h; ++y)
	{
		size_t outindex = linebytes * y;
		size_t inindex = (1 + linebytes) * y; /*the extra filterbyte added to each row*/
		unsigned char filterType = in[inindex];

		unfilterResult = unfilterScanline(&out[outindex], &in[inindex + 1], prevline, bytewidth, filterType, linebytes);
		if (unfilterResult != 0)
		{
			break;
		}

		prevline = &out[outindex];
	}

	if (unfilterResult == 0)
	{
		result.dim = imagePNGFiltered->dim;
		result.dataSize = rawRGBA32DataSize; 
		result.dataU8 = out; 
	}

	return result;
}

ImagePNGChecksumed PiratedSTB_EncodePngCRC(ImagePNGCompressed *imagePNGCompressed, Arena *arena)
{
	ImagePNGChecksumed result = {};

	unsigned int zlen = imagePNGCompressed->dataSize;
	int x = imagePNGCompressed->dim.x;
	int y = imagePNGCompressed->dim.y;

	// each tag requires 12 bytes of overhead
	unsigned int outLength = 8 + 12 + 13 + 12 + zlen + 12;
	unsigned char *out = (unsigned char *)ArenaPushSize(arena, outLength, {});

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
	STBIW_MEMMOVE(o, imagePNGCompressed->dataU8, zlen);
	o += zlen;
	stbiw__wpcrc(&o, zlen);

	stbiw__wp32(o, 0);
	stbiw__wptag(o, "IEND");
	stbiw__wpcrc(&o, 0);

	STBIW_ASSERT(o == out + outLength);

	result.dataU8 = out;
	result.dataSize = outLength;
	result.dim = imagePNGCompressed->dim;

	return result;
}

ImagePNGChecksumed PiratedFPNG_EncodePngCRC(ImagePNGCompressed *imagePNGCompressed, Arena *arena)
{
	ImagePNGChecksumed result = {};

	u8 FPNG_FDEC_VERSION = 0;
	u32 PNG_HEADER_SIZE = 58;
	u32 zlen = imagePNGCompressed->dataSize;
	u32 outLength = PNG_HEADER_SIZE + zlen + 16;
	u8 *out = (u8 *)ArenaPushSize(arena, outLength, {});
	u32 w = imagePNGCompressed->dim.x;
	u32 h = imagePNGCompressed->dim.y;
	u32 num_chans = 4;
	u32 i;

	u32 idat_len = imagePNGCompressed->dataSize;

	// Write real PNG header, fdEC chunk, and the beginning of the IDAT chunk
	{
		static const uint8_t s_color_type[] = { 0x00, 0x00, 0x04, 0x02, 0x06 };

		uint8_t pnghdr[58] = { 
			0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,   // PNG sig
			0x00,0x00,0x00,0x0d, 'I','H','D','R',  // IHDR chunk len, type
			0,0,(uint8_t)(w >> 8),(uint8_t)w, // width
			0,0,(uint8_t)(h >> 8),(uint8_t)h, // height
			8,   //bit_depth
			s_color_type[num_chans], // color_type
			0, // compression
			0, // filter
			0, // interlace
			0, 0, 0, 0, // IHDR crc32
			0, 0, 0, 5, 'f', 'd', 'E', 'C', 82, 36, 147, 227, FPNG_FDEC_VERSION,   0xE5, 0xAB, 0x62, 0x99, // our custom private, ancillary, do not copy, fdEC chunk // NOTE: (Ahmayk) Sorry imma copy it
			(uint8_t)(idat_len >> 24),(uint8_t)(idat_len >> 16),(uint8_t)(idat_len >> 8),(uint8_t)idat_len, 'I','D','A','T' // IDATA chunk len, type
		}; 

		// Compute IHDR CRC32
		u32 c = (u32)fpng::fpng_crc32(pnghdr + 12, 17, fpng::FPNG_CRC32_INIT);
		for (i = 0; i < 4; ++i, c <<= 8)
			((uint8_t*)(pnghdr + 29))[i] = (uint8_t)(c >> 24);

		memcpy(out, pnghdr, PNG_HEADER_SIZE);
	}

	memcpy(out + PNG_HEADER_SIZE, imagePNGCompressed->dataU8, imagePNGCompressed->dataSize);

	// Write IDAT chunk's CRC32 and a 0 length IEND chunk
	//vector_append(out_buf, "\0\0\0\0\0\0\0\0\x49\x45\x4e\x44\xae\x42\x60\x82", 16); // IDAT CRC32, followed by the IEND chunk
	memcpy(out + PNG_HEADER_SIZE + zlen, "\0\0\0\0\0\0\0\0\x49\x45\x4e\x44\xae\x42\x60\x82", 16);

	// Compute IDAT crc32
	u32 c = (u32)fpng::fpng_crc32(out + PNG_HEADER_SIZE - 4, idat_len + 4, fpng::FPNG_CRC32_INIT);

	for (i = 0; i < 4; ++i, c <<= 8)
		(out + outLength - 16)[i] = (uint8_t)(c >> 24);

	result.dataU8 = out;
	result.dataSize = outLength;
	result.dim = imagePNGCompressed->dim;
	return result;
}

ImageRawRGBA32 DecodePng(ImagePNGChecksumed *imagePNGChecksumed, Arena *arena)
{
	ImageRawRGBA32 result = {};

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
	unsigned int width = imagePNGChecksumed->dim.x;
	unsigned int height = imagePNGChecksumed->dim.y;
	lodepng_decode(&outData, &width, &height, &state, imagePNGChecksumed->dataU8, (size_t)imagePNGChecksumed->dataSize);

	const char *error = "";
	if (!state.error)
	{
		unsigned int rawRGBA32DataSize = GetSizeOfRawRGBA32(imagePNGChecksumed->dim);
		if (outData)
		{
			result.dim = imagePNGChecksumed->dim;
			result.dataSize = rawRGBA32DataSize;
			result.dataU8 = (u8*) ArenaPushSize(arena, result.dataSize, {});
			//TOOD: (Ahmayk) Use our own memory so there's no copying or freeing here
			memcpy(result.dataU8, outData, result.dataSize);
			lodepng_free(outData);
		}
	}
	else
	{
		// Print(lodepng_error_text(state.error));
		error = lodepng_error_text(state.error);
	}

	lodepng_state_cleanup(&state);

	return result;
}


ImageRawRGBA32 DecodePNG_FPNG(ImagePNGChecksumed *imagePNGChecksumed, Arena *arena)
{
	ImageRawRGBA32 result = {};

	u32 height = 0;
	u32 width = 0;
	u32 idat_ofs = 0;
	u32 idat_len = 0;
	u32 channels_in_file = 0;
	int status = fpng::fpng_get_info_internal(imagePNGChecksumed->dataU8, imagePNGChecksumed->dataSize, width, height, channels_in_file, idat_ofs, idat_len);
	const u8 *idatData = imagePNGChecksumed->dataU8 + idat_ofs + sizeof(u32) * 2;
	u32 src_len = imagePNGChecksumed->dataSize - (idat_ofs + sizeof(u32) * 2);
	u8 *out = ArenaPushSize(arena, arena->size, {});
	b32 decompressResult = fpng::fpng_pixel_zlib_decompress_4<4>(idatData, src_len, idat_len, out, width, height);
	if (decompressResult)
	{
		result.dataU8 = out;
		result.dataSize = width * height * 4;  //NOTE: (Ahmayk) don't assume what this should be, perhaps it has changed
		result.dim = imagePNGChecksumed->dim;
	}

	return result;
}


void UploadAndReplaceTexture(ImageRawRGBA32 *imageRaw, Texture *texture)
{
	if (ASSERT(imageRaw->dataU8))
	{
		if (texture->id)
		{
			rlUnloadTexture(texture->id);
		}
		*texture = {};

		texture->id = rlLoadTexture(imageRaw->dataU8, imageRaw->dim.x, imageRaw->dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
		texture->width = imageRaw->dim.x;
		texture->height = imageRaw->dim.y;
		texture->mipmaps = 1;
		texture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	}
	// GenTextureMipmaps(texture);
}

void UpdateRectInTexture(Texture *texture, void *data, RectIV2 rect)
{
	if (data)
	{
		rlUpdateTexture(texture->id, rect.pos.x, rect.pos.y, rect.dim.x, rect.dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, data);
	}
	else
	{
		//TODO: Log error
		// Print("Empty image passed into texture uploader!");
	}
}

void SetPNGFilterType(Canvas *canvas, ImageRawRGBA32 *rootImageRaw, GameMemory *gameMemory)
{
	LockOnBool(&canvas->filterLock);
	ArenaReset(&canvas->arenaFilteredPNG);
	canvas->imagePNGFiltered = PiratedSTB_EncodePngFilters(rootImageRaw, &canvas->arenaFilteredPNG, canvas->currentPNGFilterType);

#if DEBUG_MODE
	MemoryProtectWrite(canvas->arenaFilteredPNG.memory, canvas->arenaFilteredPNG.used);
#endif

	UnlockOnBool(&canvas->filterLock);

	iv2 canvasDim = iv2{(rootImageRaw->dim.x * 4) + 1, rootImageRaw->dim.y};
	u32 visualizedCanvasDataSize = canvasDim.x * canvasDim.y * sizeof(Color);

	ArenaMarker marker = {};
	ImageRawRGBA32 visualizedFilteredRootImage = {};
	visualizedFilteredRootImage.dim = canvasDim;
	visualizedFilteredRootImage.dataSize = visualizedCanvasDataSize;
	visualizedFilteredRootImage.dataU8 = (u8*) ArenaPushSize(&gameMemory->temporaryArena, visualizedCanvasDataSize, &marker);
	for (int i = 0; i < canvasDim.x * canvasDim.y; i++)
	{
		u8 value = canvas->imagePNGFiltered.dataU8[i];
		((Color *)(visualizedFilteredRootImage.dataU8))[i] = Color{value, value, value, 255};
	}
	UploadAndReplaceTexture(&visualizedFilteredRootImage, &canvas->textureVisualizedFilteredRootImage);
	ArenaPopMarker(marker);
}

RectIV2 GetDrawingRectFromIndex(iv2 imageDim, iv2 rectDim, u32 i)
{
	RectIV2 result;
	result.dim = rectDim; 

	u32 numX = (u32) CeilF32(imageDim.x / (f32) rectDim.x);
	u32 numY = (u32) CeilF32(imageDim.y / (f32) rectDim.y);
	u32 indexX = i % numX;
	u32 indexY = ((u32) FloorF32(i / (f32) numX)) % numX;
	result.pos.x = result.dim.x * indexX;
	result.pos.y = result.dim.y * indexY;

	if (result.pos.x + result.dim.x > imageDim.x)
	{
		result.dim.x = imageDim.x - result.pos.x;
	}
	if (result.pos.y + result.dim.y > imageDim.y)
	{
		result.dim.y = imageDim.y - result.pos.y;
	}
	ASSERT(result.dim.y > 0);

	return result;
}

u32 GetDrawingRectCount(iv2 imageDim, iv2 rectDim)
{
	u32 numX = (u32) CeilF32(imageDim.x / (f32) rectDim.x);
	u32 numY = (u32) CeilF32(imageDim.y / (f32) rectDim.y);
	u32 result = numX * numY;
	return result;
}

void CanvasSetDirtyRect(Canvas *canvas, RectIV2 updateArea)
{
	u32 numX = (u32) CeilF32(canvas->drawnImageData.dim.x / (f32) canvas->drawingRectDim.x);
	u32 numY = (u32) CeilF32(canvas->drawnImageData.dim.y / (f32) canvas->drawingRectDim.y);
	u32 indexX = MaxI32(0, updateArea.pos.x) / canvas->drawingRectDim.x;
	u32 indexY = MaxI32(0, updateArea.pos.y) / canvas->drawingRectDim.y;

	RectIV2 drawRect;
	drawRect.dim = canvas->drawingRectDim;
	for (u32 y = indexY; y < numY; y++)
	{
		drawRect.pos.y = y * canvas->drawingRectDim.x;
		b32 interceptsThisRow = false;
		for (u32 x = indexX; x < numX; x++)
		{
			drawRect.pos.x = x * canvas->drawingRectDim.x;
			if (!IsInterceptRectIV2(updateArea, drawRect))
			{
				break;
			}
			interceptsThisRow = true;
			u32 dirtyIndexX = drawRect.pos.x / canvas->drawingRectDim.x;
			u32 dirtyIndexY = drawRect.pos.y / canvas->drawingRectDim.y;
			u32 index = dirtyIndexX + (dirtyIndexY * numX);
			if (ASSERT(index < canvas->drawingRectCount))
			{
				canvas->drawingRectDirtyList[index] = true;
				canvas->drawingRectDirtyListProcess[index] = true;
			}
		}
		if (!interceptsThisRow)
		{
			break;
		}
	}
}

Image ImageRawToRayImage(ImageRawRGBA32 *imageRaw)
{
	Image result;
	result.data = imageRaw->dataU8;
	result.width = imageRaw->dim.x;
	result.height = imageRaw->dim.y;
	result.mipmaps = 1;
	result.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	return result;
}

void ProcessEdge(iv2 a, iv2 b, i32 y, i32 *xmin, i32 *xmax)
{
	if (a.y == b.y)
	{
		if (y == a.y)
		{
			i32 left = a.x < b.x ? a.x : b.x;
			i32 right = a.x > b.x ? a.x : b.x;
			if (left < *xmin) *xmin = left;
			if (right > *xmax) *xmax = right;
		}
	}
	else if ((a.y <= y && y <= b.y) || (b.y <= y && y <= a.y))
	{
		f32 t = (f32) (y - a.y) / (b.y - a.y);
		//TODO: (Ahmayk) Worth it?
		i32 x = a.x + (i32) RoundF32((t * (b.x - a.x)));
		if (x < *xmin) *xmin = x;
		if (x > *xmax) *xmax = x;
	}
}

b32 CanvasImageDrawPixel(Image *dst, int x, int y, Color color)
{
	b32 result = false;
	u8 r = ((unsigned char *)dst->data)[(y*dst->width + x)*4];
	if (r != color.r) 
	{
		((unsigned char *)dst->data)[(y*dst->width + x)*4] = color.r;
		((unsigned char *)dst->data)[(y*dst->width + x)*4 + 1] = color.g;
		((unsigned char *)dst->data)[(y*dst->width + x)*4 + 2] = color.b;
		((unsigned char *)dst->data)[(y*dst->width + x)*4 + 3] = color.a;
		result = true;
	}
	return result;
}

b32 CanvasFillConvexQuad(Canvas *canvas, iv2 p0, iv2 p1, iv2 p2, iv2 p3, Color color)
{
	b32 result = false;
	iv2 minIV2 = p0;
	iv2 maxIV2 = p0;
    iv2 points[] = {p0, p1, p2, p3};
    for (u32 i = 1; i < 4; i++)
	{
        if (points[i].x < minIV2.x)
		{
			minIV2.x = points[i].x;
		}
        if (points[i].x > maxIV2.x)
		{
			maxIV2.x = points[i].x;
		}
        if (points[i].y < minIV2.y)
		{
			minIV2.y = points[i].y;
		}
        if (points[i].y > maxIV2.y)
		{
			maxIV2.y = points[i].y;
		}
    }

	minIV2.y = ClampI32(0, minIV2.y, canvas->drawnImageData.dim.y - 1);
	maxIV2.y = ClampI32(0, maxIV2.y, canvas->drawnImageData.dim.y - 1);

	Image tempImage = ImageRawToRayImage(&canvas->drawnImageData);
    for (i32 y = minIV2.y; y <= maxIV2.y; y++)
	{
        i32 xmin = INT_MAX;
        i32 xmax = INT_MIN;
        ProcessEdge(p0, p1, y, &xmin, &xmax);
        ProcessEdge(p1, p2, y, &xmin, &xmax);
        ProcessEdge(p2, p3, y, &xmin, &xmax);
        ProcessEdge(p3, p0, y, &xmin, &xmax);
		xmin = ClampI32(0, xmin, canvas->drawnImageData.dim.x - 1);
		xmax = ClampI32(0, xmax, canvas->drawnImageData.dim.x - 1);

        if (xmin <= xmax)
		{
            for (i32 x = xmin; x <= xmax; x++)
			{
				result |= CanvasImageDrawPixel(&tempImage, x, y, color);
            }
        }
    }

	if (result)
	{
		RectIV2 updateArea;
		updateArea.pos = minIV2;
		updateArea.dim = maxIV2 - minIV2 + 1;
		CanvasSetDirtyRect(canvas, updateArea);
	}
	return result;
}

b32 CanvasImageDrawRectangle(Image *dst, int posX, int posY, int width, int height, Color color)
{
	b32 result = false;
    // Security check to avoid drawing out of bounds in case of bad user data
    if (posX < 0) { width += posX; posX = 0; }
    if (posY < 0) { height += posY; posY = 0; }
    if (width < 0) width = 0;
    if (height < 0) height = 0;

    // Clamp the size the the image bounds
    if ((posX + width) >= dst->width) width = dst->width - posX;
    if ((posY + height) >= dst->height) height = dst->height - posY;

    // Check if the  is even inside the image
    if ((posX >= dst->width) || (posY >= dst->height)) return result;
    if (((posX + width) <= 0) || (posY + height <= 0)) return result;

	u32 startY = posY;
	u32 endY = startY + height;
	for (u32 y = startY; y < endY; y++)
	{
		u32 startIndex = (dst->width * y) + posX;
		u32 endIndex = startIndex + width;
		for (u32 i = startIndex; i < endIndex; i++)
		{
			Color *pixelDest = &((Color*)dst->data)[i];
			if (pixelDest->r != color.r)
			{
				*pixelDest = color;
				result |= true;
			}
		}
	}

   return result;
}

b32 CanvasDrawCircle(Canvas *canvas, iv2 pos, u32 radius, Color color)
{
	b32 result = false;

	Image dst = ImageRawToRayImage(&canvas->drawnImageData);
	u32 centerX = pos.x;
	u32 centerY = pos.y;
    int x = 0;
    int y = radius;
    int decesionParameter = 3 - 2*radius;

    while (y >= x)
    {
        result |= CanvasImageDrawRectangle(&dst, centerX - x, centerY + y, x*2, 1, color);
        result |= CanvasImageDrawRectangle(&dst, centerX - x, centerY - y, x*2, 1, color);
        result |= CanvasImageDrawRectangle(&dst, centerX - y, centerY + x, y*2, 1, color);
        result |= CanvasImageDrawRectangle(&dst, centerX - y, centerY - x, y*2, 1, color);
        x++;

        if (decesionParameter > 0)
        {
            y--;
            decesionParameter = decesionParameter + 4*(x - y) + 10;
        } 
        else decesionParameter = decesionParameter + 4*x + 6;
    }

	if (result)
	{
		RectIV2 updateArea;
		updateArea.dim.x = radius * 2 + 1;
		updateArea.dim.y = radius * 2 + 1;
		updateArea.pos.x = pos.x - radius;
		updateArea.pos.y = pos.y - radius;
		CanvasSetDirtyRect(canvas, updateArea);
	}
	return result;
}

b32 CanvasDrawCircleStroke(Canvas *canvas, iv2 startPos, iv2 endPos, u32 radius, Color color)
{
	b32 result = false;

	result |= CanvasDrawCircle(canvas, startPos, radius, color);

	if (startPos != endPos)
	{
		result |= CanvasDrawCircle(canvas, endPos, radius, color);

		i32 dx = endPos.x - startPos.x;
		i32 dy = endPos.y - startPos.y;
		f32 length = SqrtI32(dx*dx + dy*dy);
		if (length > 2)
		{
			f32 px = (-dy * (i32) radius) / length;
			f32 py = (dx * (i32) radius) / length;
			iv2 points[4] =
			{
				{ (i32)RoundF32(startPos.x + px), (i32)RoundF32(startPos.y + py) },
				{ (i32)RoundF32(startPos.x - px), (i32)RoundF32(startPos.y - py) },
				{ (i32)RoundF32(endPos.x   - px), (i32)RoundF32(endPos.y   - py) },
				{ (i32)RoundF32(endPos.x   + px), (i32)RoundF32(endPos.y   + py) }
			};
			result |= CanvasFillConvexQuad(canvas, points[0], points[1], points[2], points[3], color);
		}
	}
	return result;
}

void HashImageRects(ImageRawRGBA32 *imageRaw, iv2 rectDim, u32 **outHashes)
{
	u32 rectCount = GetDrawingRectCount(imageRaw->dim, rectDim);
	for (u32 rectIndex = 0; rectIndex < rectCount; rectIndex++)
	{
		u32 hash = 5381;
		RectIV2 rect = GetDrawingRectFromIndex(imageRaw->dim, rectDim, rectIndex);
		u32 startY = rect.pos.y;
		u32 endY = startY + rect.dim.y;
		for (u32 y = startY; y < endY; y++)
		{
			u32 startIndex = ((imageRaw->dim.x * y) + rect.pos.x) * 4;
			u8 *pixel = imageRaw->dataU8 + startIndex;
			hash = DJB33HashU32((u32*)pixel, rect.dim.x, hash);
		}
		(*outHashes)[rectIndex] = hash;
	}
}

void InitializeCanvas(Canvas *canvas, ImageRawRGBA32 *rootImageRaw, Brush *brush, GameMemory *gameMemory)
{
	//NOTE: (Ahmayk) free and reallocate temporary arena to reduce memory size
	//to uncommit the memory there, will reduce our memory footprint
	if (ASSERT(gameMemory->temporaryArena.used == 0))
	{
		u64 size = gameMemory->temporaryArena.size;
		ArenaFree(&gameMemory->temporaryArena);
		gameMemory->temporaryArena = ArenaInit(size);
	}

	ArenaFree(&gameMemory->canvasArena);

	iv2 canvasDim = iv2{(rootImageRaw->dim.x * 4) + 1, rootImageRaw->dim.y};
	u32 visualizedCanvasDataSize = canvasDim.x * canvasDim.y * sizeof(Color);

	//NOTE: (Ahmayk) drawn image, visualized image, drawingRects, dirtyRectsForEachProcesssImage, finalImageRectHashes
	u32 canvasArneaSize = (u32) MaxU32(MegaByte * 1, (u32) (visualizedCanvasDataSize * 2.1f));
	gameMemory->canvasArena = ArenaInit(canvasArneaSize);

	canvas->drawnImageData.dataU8 = ArenaPushSize(&gameMemory->canvasArena, visualizedCanvasDataSize, {});
	canvas->drawnImageData.dim = canvasDim;
	canvas->drawnImageData.dataSize = visualizedCanvasDataSize;
	memset(canvas->drawnImageData.dataU8, 0, visualizedCanvasDataSize);

	u32 conversionArenaSize = (canvasDim.x * canvasDim.y) + (rootImageRaw->dim.x * 4);
	conversionArenaSize = (u32) (conversionArenaSize * 1.2f);
	AlignPow2U32(&conversionArenaSize, 256);

	canvas->arenaFilteredPNG = ArenaInitFromArena(&gameMemory->canvasArena, conversionArenaSize);

	ArenaGroupFree(&gameMemory->conversionArenaGroup);
	gameMemory->conversionArenaGroup = ArenaGroupInit(MegaByte * 400);
	ArenaGroupResetAndFill(&gameMemory->conversionArenaGroup, conversionArenaSize);

	SetPNGFilterType(canvas, rootImageRaw, gameMemory);

	if (canvas->textureDrawing.id)
	{
		rlUnloadTexture(canvas->textureDrawing.id);
		canvas->textureDrawing = {};
	}

	canvas->textureDrawing.id = rlLoadTexture(canvas->drawnImageData.dataU8, canvasDim.x, canvasDim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
	canvas->textureDrawing.width = canvasDim.x;
	canvas->textureDrawing.height = canvasDim.y;
	canvas->textureDrawing.mipmaps = 1;
	canvas->textureDrawing.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

	if (canvas->initialized)
	{
		glDeleteBuffers(ARRAY_COUNT(canvas->drawingPboIDs), canvas->drawingPboIDs);
	}
	glGenBuffers(ARRAY_COUNT(canvas->drawingPboIDs), canvas->drawingPboIDs);
	for(u32 i = 0; i < ARRAY_COUNT(canvas->drawingPboIDs); i++) 
	{
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, canvas->drawingPboIDs[i]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, visualizedCanvasDataSize, NULL, GL_STREAM_DRAW);
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	if (canvas->initialized)
	{
		glDeleteBuffers(ARRAY_COUNT(canvas->finalImagePboIDs), canvas->finalImagePboIDs);
	}
	glGenBuffers(ARRAY_COUNT(canvas->finalImagePboIDs), canvas->finalImagePboIDs);
	for(u32 i = 0; i < ARRAY_COUNT(canvas->finalImagePboIDs); i++) 
	{
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, canvas->finalImagePboIDs[i]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, rootImageRaw->dim.x * rootImageRaw->dim.y * 4, NULL, GL_STREAM_DRAW);
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	canvas->finalImageRectDim = iv2{512, 512};
	canvas->finalImageRectCount = GetDrawingRectCount(rootImageRaw->dim, canvas->finalImageRectDim);
	canvas->cachedFinalImageRectHashes = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->finalImageRectCount, u32);
	HashImageRects(rootImageRaw, canvas->finalImageRectDim, &canvas->cachedFinalImageRectHashes);

	ArenaFree(&gameMemory->canvasRollbackArena);
	gameMemory->canvasRollbackArena = ArenaInit(MegaByte * 800);
	canvas->rollbackSizeCount = (u32) FloorF32((f32)gameMemory->canvasRollbackArena.size / visualizedCanvasDataSize) - 1;
	canvas->rollbackImageData = ARENA_PUSH_ARRAY(&gameMemory->canvasRollbackArena, canvas->rollbackSizeCount * visualizedCanvasDataSize, u8);
	canvas->rollbackIndexStart = 0;
	canvas->rollbackIndexNext = 0;
	canvas->brush = brush;

	canvas->drawingRectDim = iv2{32, 32};
	canvas->drawingRectCount = GetDrawingRectCount(canvasDim, canvas->drawingRectDim);
	canvas->drawingRectDirtyList = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->drawingRectCount, b32);
	canvas->drawingRectDirtyListProcess = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->drawingRectCount, b32);
	memset(canvas->drawingRectDirtyList, 0, canvas->drawingRectCount * sizeof(b32));
	memset(canvas->drawingRectDirtyListProcess, 0, canvas->drawingRectCount * sizeof(b32));

	canvas->initialized = true;
}

void InitializeNewImage(const char *fileName, GameMemory *gameMemory, ImageRawRGBA32 *rootImageRaw, Canvas *canvas, Texture *loadedTexture, Brush *currentBrush, ProcessedImage *processedImages, u32 threadCount)
{
	*rootImageRaw = LoadDataIntoRawImage(fileName, gameMemory);
	if (rootImageRaw->dataU8)
	{
		UploadAndReplaceTexture(rootImageRaw, loadedTexture);
		InitializeCanvas(canvas, rootImageRaw, currentBrush, gameMemory);
		for (u32 i = 0; i < threadCount; i++)
		{
			ProcessedImage *processedImage = processedImages + i;
			processedImage->dirtyRectsInProcess = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->drawingRectCount, b32);
			processedImage->finalImageRectHashes = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->finalImageRectCount, u32);
		}
	}
}

void UpdateBpImageOnThread(ProcessedImage *processedImage)
{
	// Print("Staring Work on thread " + IntToString(processedImage->index));

	Canvas *canvas = processedImage->canvas;

	Arena *arenaFiltered = ArenaPairPushOldest(&processedImage->arenaPair, {});

	LockOnBool(&canvas->filterLock);
	ImagePNGFiltered imagePNGFiltered = canvas->imagePNGFiltered;
	imagePNGFiltered.dataU8 = ArenaPushSize(arenaFiltered, imagePNGFiltered.dataSize, {});
	memcpy(imagePNGFiltered.dataU8, canvas->imagePNGFiltered.dataU8, imagePNGFiltered.dataSize);
	UnlockOnBool(&canvas->filterLock);

#if 1
	for (u32 i = 0; i < imagePNGFiltered.dataSize; i += 1)
	{
		Color canvasPixel = ((Color *)canvas->drawnImageData.dataU8)[i];

		if (canvasPixel.r)
		{
			switch (canvasPixel.r)
			{
				case BRUSH_EFFECT_ERASE:
					break;
				case BRUSH_EFFECT_REMOVE:
					{
						imagePNGFiltered.dataU8[i] = 0;
						break;
					}
				case BRUSH_EFFECT_MAX:
					{
						imagePNGFiltered.dataU8[i] = 255;
						break;
					}
				case BRUSH_EFFECT_SHIFT:
					{
						int shiftAmount = 36;
						if (i < imagePNGFiltered.dataSize - shiftAmount)
							imagePNGFiltered.dataU8[i] = imagePNGFiltered.dataU8[i + shiftAmount];
						break;
					}
				case BRUSH_EFFECT_RANDOM:
					{
						imagePNGFiltered.dataU8[i] = canvasPixel.g;
						break;
					}
					InvalidDefaultCase
			}
		}
	}
#endif

#if 0
	ImagePNGCompressed imagePNGCompressed = {};
	imagePNGCompressed.dim = imagePNGFiltered.dim;
	imagePNGCompressed.dataSize = imagePNGFiltered.dataSize;

	Arena *arenaCompressed = ArenaPairPushOldest(&processedImage->arenaPair, {});

	fpng::fpng_init(); //NOTE: (Ahmayk) enables SSE in fpng
	imagePNGCompressed.dataU8 = (u8 *)ArenaPushSize(arenaCompressed, arenaCompressed->size, {});
	imagePNGCompressed.dataSize = fpng::pixel_deflate_dyn_4_rle_one_pass(imagePNGFiltered.dataU8, imagePNGFiltered.dim.x, imagePNGFiltered.dim.y, imagePNGCompressed.dataU8, (u32) arenaCompressed->size);
	Arena *arenaChecksumed = ArenaPairPushOldest(&processedImage->arenaPair, arenaFiltered);

	// Print("Encoded PNG Compression on thread " + IntToString(processedImage->index));
	//ImagePNGChecksumed imagePNGChecksumed = PiratedSTB_EncodePngCRC(&imagePNGCompressed, arenaChecksumed);
	ImagePNGChecksumed imagePNGChecksumed = PiratedFPNG_EncodePngCRC(&imagePNGCompressed, arenaChecksumed);

	Arena *arenaFinalRaw = ArenaPairPushOldest(&processedImage->arenaPair, arenaCompressed);
	// Print("Encoded PNG CRC on thread " + IntToString(processedImage->index));

	//NOTE: (Ahmayk) FPNG seems to be optimized on filter type up only. So if we're in that mode
	//we can take advantage of its optimization! But otherwise we need a slower generic decoder
	if (canvas->currentPNGFilterType == PNG_FILTER_TYPE_UP)
	{
		processedImage->finalProcessedImageRaw = DecodePNG_FPNG(&imagePNGChecksumed, arenaFinalRaw);
	}
	else
	{
		processedImage->finalProcessedImageRaw = DecodePng(&imagePNGChecksumed, arenaFinalRaw);
	}
#else
	Arena *arenaFinalRaw = ArenaPairPushOldest(&processedImage->arenaPair, {});
	processedImage->finalProcessedImageRaw = PiratedLoadPNG_Defilter(&imagePNGFiltered, arenaFinalRaw);
#endif

	ArenaPairFreeOldest(&processedImage->arenaPair);

	//f64 start = GetTime();
	HashImageRects(&processedImage->finalProcessedImageRaw, canvas->finalImageRectDim, &processedImage->finalImageRectHashes);
	//f64 end = GetTime();
	//printf("process time: %f\n", (end - start) * 1000);
	// Print("Decoded PNG on thread " + IntToString(processedImage->index));

	// Print("Converted to final PNG on thead " + IntToString(processedImage->index));
}

void ResetProcessedImage(ProcessedImage *processedImage, Canvas *canvas)
{
	processedImage->active = false;
	processedImage->frameStarted = 0;
	processedImage->frameFinished = 0;
	processedImage->finalProcessedImageRaw = {};
	ArenaPairFreeAll(&processedImage->arenaPair);
}

ProcessedImage *GetFreeProcessedImage(ProcessedImage *processedImages, unsigned int threadCount)
{
	ProcessedImage *result = {};
	for (u32 i = 0; i < threadCount; i++)
	{
		ProcessedImage *processedImageOfIndex = processedImages + i;
		if (!processedImageOfIndex->active)
		{
			result = processedImageOfIndex;
			break;
		}
	}

	return result;
}

PLATFORM_WORK_QUEUE_CALLBACK(ProcessImageOnThread)
{
	ProcessedImage *processedImage = (ProcessedImage *)data;
	ASSERT(processedImage);

	UpdateBpImageOnThread(processedImage);
	processedImage->frameFinished = G_CURRENT_FRAME;
	// Print("Finished on thread " + IntToString(processedImage->index));
}

bool ExportImage(Image image, String filepath)
{
	bool result = false;
	int channels = 4;

	//HACK: append png if there's no file path lol
	//TODO: the platform layer should handle this not the game
	//TODO: Do this ourselves so we don't have to play hot potato with string null termination
	String fileExtention = STRING(GetFileExtension(C_STRING_NULL_TERMINATED(filepath)));
	if (!fileExtention.length)
	{
		filepath += ".png";
	}

	fileExtention = STRING(GetFileExtension(C_STRING_NULL_TERMINATED(filepath)));

	char *filepathChars = C_STRING_NULL_TERMINATED(filepath);

	if (fileExtention == ".png")
	{
		int dataSize = 0;
		unsigned char *fileData = stbi_write_png_to_mem((const unsigned char *)image.data, image.width * channels, image.width, image.height, channels, &dataSize);
		result = SaveFileData(filepathChars, fileData, dataSize);
		RL_FREE(fileData);
	}
	else if (fileExtention == ".bmp")
	{
		result = stbi_write_bmp(filepathChars, image.width, image.height, channels, image.data);
	}
	else if (fileExtention == ".jpg" || fileExtention == ".jpeg")
	{
		result = stbi_write_jpg(filepathChars, image.width, image.height, channels, image.data, 100); // JPG quality: between 1 and 100
	}
	else
	{
		// Export raw pixel data (without header)
		// NOTE: It's up to the user to track image parameters
		result = SaveFileData(filepathChars, image.data, GetPixelDataSize(image.width, image.height, image.format));
	}

	return result;
}

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

ImageRaw LoadDataIntoRawImage(const char *filePath, GameMemory *gameMemory)
{
	ImageRaw result = {};

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
		int dataSize = GetSizeOfRawRGBA32(WidthHeightToV2(width, height));
		if (dataSize < 30000000)
		{
			ArenaReset(&gameMemory->rootImageArena);
			result.dim.x = width;
			result.dim.y = height;
			result.dataSize = dataSize;
			result.dataU8 = (u8*) ArenaPushSize(&gameMemory->rootImageArena, result.dataSize, {});

			if (comp != 4)
			{
				ArenaMarker marker = {};
				V4 *pixels = ARENA_PUSH_ARRAY_MARKER(&gameMemory->temporaryArena, width * height, V4, &marker);
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

ImagePNGFiltered PiratedSTB_EncodePngFilters(ImageRaw *imageRaw, Arena *arena, PNG_FILTER_TYPE pngFilterType)
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

ImageRaw DecodePng(ImagePNGChecksumed *imagePNGChecksumed, Arena *arena)
{
	ImageRaw result = {};

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
		const char *error = lodepng_error_text(state.error);
	}

	lodepng_state_cleanup(&state);

	return result;
}

void UploadAndReplaceTexture(ImageRaw *imageRaw, Texture *texture)
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

void SetPNGFilterType(Canvas *canvas, ImageRaw *rootImageRaw, GameMemory *gameMemory)
{
	LockOnBool(&canvas->filterLock);
	ArenaReset(&canvas->arenaFilteredPNG);
	canvas->imagePNGFiltered = PiratedSTB_EncodePngFilters(rootImageRaw, &canvas->arenaFilteredPNG, canvas->currentPNGFilterType);

#if DEBUG_MODE
	MemoryProtectWrite(canvas->arenaFilteredPNG.memory, canvas->arenaFilteredPNG.used);
#endif

	UnlockOnBool(&canvas->filterLock);

	V2 canvasDim = V2{(rootImageRaw->dim.x * 4) + 1, rootImageRaw->dim.y};
	u32 visualizedCanvasDataSize = canvasDim.x * canvasDim.y * sizeof(Color);

	ArenaMarker marker = {};
	ImageRaw visualizedFilteredRootImage = {};
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
	canvas->needsTextureUpload = true;
}

void InitializeCanvas(Canvas *canvas, ImageRaw *rootImageRaw, Brush *brush, GameMemory *gameMemory)
{
	ArenaReset(&gameMemory->canvasArena);

	V2 canvasDim = V2{(rootImageRaw->dim.x * 4) + 1, rootImageRaw->dim.y};
	u32 visualizedCanvasDataSize = canvasDim.x * canvasDim.y * sizeof(Color);

	canvas->drawnImageData.data = ArenaPushSize(&gameMemory->canvasArena, visualizedCanvasDataSize, {});
	canvas->drawnImageData.width = canvasDim.x;
	canvas->drawnImageData.height = canvasDim.y;
	canvas->drawnImageData.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	canvas->drawnImageData.mipmaps = 1;
	memset(canvas->drawnImageData.data, 0, visualizedCanvasDataSize);

	u32 conversionArenaSize = (canvasDim.x * canvasDim.y) + (rootImageRaw->dim.x * 4);
	conversionArenaSize *= 1.2;
	ALIGN_POW2(&conversionArenaSize, 256);

	canvas->arenaFilteredPNG = ArenaInitFromArena(&gameMemory->canvasArena, conversionArenaSize);

	ArenaGroupFill(&gameMemory->conversionArenaGroup, conversionArenaSize);

	SetPNGFilterType(canvas, rootImageRaw, gameMemory);

	if (canvas->texture.id)
	{
		rlUnloadTexture(canvas->texture.id);
		canvas->texture = {};
	}

	ArenaReset(&gameMemory->canvasRollbackArena);
	unsigned int canvasSize = GetCanvasDatasize(canvas);
	canvas->rollbackSizeCount = Floor((float)gameMemory->canvasRollbackArena.size / canvasSize) - 1;
	canvas->rollbackImageData = ARENA_PUSH_ARRAY(&gameMemory->canvasRollbackArena, canvas->rollbackSizeCount * canvasSize, unsigned char);
	canvas->rollbackIndexStart = 0;
	canvas->rollbackIndexNext = 0;
	canvas->brush = brush;
}

void InitializeNewImage(const char *fileName, GameMemory *gameMemory, ImageRaw *rootImageRaw, Canvas *canvas, Texture *loadedTexture, Brush *currentBrush)
{
	*rootImageRaw = LoadDataIntoRawImage(fileName, gameMemory);
	if (rootImageRaw->dataU8)
	{
		UploadAndReplaceTexture(rootImageRaw, loadedTexture);
		InitializeCanvas(canvas, rootImageRaw, currentBrush, gameMemory);
		// *latestCompletedBpImage = CreateDataImage(rootImageRaw, {}, gameMemory);
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

	ASSERT(processedImage->canvas->drawnImageData.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

	for (int i = 0;
			i < imagePNGFiltered.dataSize;
			i += 1)
	{
		Color canvasPixel = ((Color *)canvas->drawnImageData.data)[i];

		if (canvasPixel.r)
		{
			switch (canvasPixel.r)
			{
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
				case BRUSH_EFFECT_ERASE_EFFECT:
					break;
					InvalidDefaultCase
			}
		}
	}

	ImagePNGCompressed imagePNGCompressed = {};
	imagePNGCompressed.dim = imagePNGFiltered.dim;
	imagePNGCompressed.dataSize = imagePNGFiltered.dataSize;

	Arena *arenaCompressed = ArenaPairPushOldest(&processedImage->arenaPair, {});

	imagePNGCompressed.dataU8 = (u8 *)ArenaPushSize(arenaCompressed, arenaCompressed->size, {});
	imagePNGCompressed.dataSize = fpng::pixel_deflate_dyn_4_rle_one_pass(imagePNGFiltered.dataU8, imagePNGFiltered.dim.x, imagePNGFiltered.dim.y, imagePNGCompressed.dataU8, arenaCompressed->size);
	Arena *arenaChecksumed = ArenaPairPushOldest(&processedImage->arenaPair, arenaFiltered);

	// Print("Encoded PNG Compression on thread " + IntToString(processedImage->index));
	ImagePNGChecksumed imagePNGChecksumed = PiratedSTB_EncodePngCRC(&imagePNGCompressed, arenaChecksumed);

	Arena *arenaFinalRaw = ArenaPairPushOldest(&processedImage->arenaPair, arenaCompressed);
	// Print("Encoded PNG CRC on thread " + IntToString(processedImage->index));
	processedImage->finalProcessedImageRaw = DecodePng(&imagePNGChecksumed, arenaFinalRaw);

	ArenaPairFreeOldest(&processedImage->arenaPair);
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

	unsigned int pixelCount = canvas->drawnImageData.width * canvas->drawnImageData.height;

#if 1
	for (int i = 0;
			i < pixelCount;
			i++)
	{
		Color *drawnPixel = &((Color *)canvas->drawnImageData.data)[i];
		if ((drawnPixel->r || drawnPixel->g || drawnPixel->b) && drawnPixel->a == processedImage->index)
			drawnPixel->a = 255;
	}
#endif

	canvas->needsTextureUpload = true;
}

ProcessedImage *GetFreeProcessedImage(ProcessedImage *processedImages, unsigned int threadCount)
{
	ProcessedImage *result = {};
	for (int i = 0;
			i < threadCount;
			i++)
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

void StartProcessedImageWork(Canvas *canvas, unsigned int threadCount, ProcessedImage *processedImage, PlatformWorkQueue *threadWorkQueue)
{
	unsigned int pixelCount = canvas->drawnImageData.width * canvas->drawnImageData.height;
#if 1
	for (int i = 0;
			i < pixelCount;
			i++)
	{
		Color *drawnPixel = &((Color *)canvas->drawnImageData.data)[i];
		if (drawnPixel->a == threadCount + 1 || (canvas->oldDataPresent && (drawnPixel->r || drawnPixel->g || drawnPixel->b) && drawnPixel->a != 255))
			drawnPixel->a = processedImage->index;
	}
#endif

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

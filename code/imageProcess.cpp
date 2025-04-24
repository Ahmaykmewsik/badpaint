
#include "image.h"
#include "imageProcess.h"
#include "platform_win32.h"

#include "../includes/lodepng.h"
#include "../includes/lodepng.c"

#include "../includes/raylib/src/external/stb_image.h"
#include "../includes/raylib/src/external/stb_image_write.h"

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
		memmove(filters + j * (x * n + 1) + 1, line_buffer, x * n);
	}
	result.dataU8 = filters;
	result.pngFilterType = pngFilterType;

	return result;
}

ImageRawRGBA32 PiratedLoadPNG_Defilter(ImagePNGFiltered *imagePNGFiltered, Arena *arena)
{
	ImageRawRGBA32 result = {};;

	u32 rawRGBA32DataSize = imagePNGFiltered->dim.x * imagePNGFiltered->dim.y * 4 * sizeof(u8);
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

	for (u32 y = 0; y < (u32) canvas->imagePNGFiltered.dim.y; y++)
	{
		u32 filterByteOffset = y + 1;
		u32 startIndex = imagePNGFiltered.dim.x * y;
		u32 endIndex = startIndex + imagePNGFiltered.dim.x;

		for (u32 i = startIndex; i < endIndex; i++)
		{
			u8 *filteredPixel = imagePNGFiltered.dataU8 + (i * 4) + filterByteOffset;
			u8 *canvasPixel = canvas->drawnImageData.dataU8 + (i * 4);

			if (canvasPixel[0])
			{
				switch (canvasPixel[0])
				{
					case BADPAINT_BRUSH_EFFECT_REMOVE:
					{
						filteredPixel[0] = 0;
						filteredPixel[1] = 0;
						filteredPixel[2] = 0;
						filteredPixel[3] = 0;
					} break;
					case BADPAINT_BRUSH_EFFECT_MAX:
					{
						filteredPixel[0] = 255;
						filteredPixel[1] = 255;
						filteredPixel[2] = 255;
						filteredPixel[3] = 255;
					} break;
					case BADPAINT_BRUSH_EFFECT_SHIFT:
					{
						int shiftAmount = 36;
						if (i < imagePNGFiltered.dataSize - shiftAmount)
						{
							filteredPixel[0] = filteredPixel[0 + shiftAmount];
							filteredPixel[1] = filteredPixel[1 + shiftAmount];
							filteredPixel[2] = filteredPixel[2 + shiftAmount];
							filteredPixel[3] = filteredPixel[3 + shiftAmount];
						}
					} break;
					case BADPAINT_BRUSH_EFFECT_RANDOM:
					{
						filteredPixel[0] = canvasPixel[1];
						filteredPixel[1] = canvasPixel[1];
						filteredPixel[2] = canvasPixel[1];
						filteredPixel[3] = canvasPixel[1];
					} break;
					InvalidDefaultCase
				}
			}
		}
	}

	Arena *arenaFinalRaw = ArenaPairPushOldest(&processedImage->arenaPair, {});
	processedImage->finalProcessedImageRaw = PiratedLoadPNG_Defilter(&imagePNGFiltered, arenaFinalRaw);

	ArenaPairFreeOldest(&processedImage->arenaPair);
	HashImageRects(&processedImage->finalProcessedImageRaw, canvas->finalImageRectDim, &processedImage->finalImageRectHashes);
	// Print("Converted to final PNG on thead " + IntToString(processedImage->index));
	processedImage->processingComplete = true;
}

void ResetProcessedImage(ProcessedImage *processedImage, Canvas *canvas)
{
	processedImage->active = false;
	processedImage->frameStarted = 0;
	processedImage->processingComplete = false;
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
	// Print("Finished on thread " + IntToString(processedImage->index));
}

#pragma once

#include "image.h"
#include "main.h"
#include "platform_win32.h"

#include "../includes/raylib/src/external/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../includes/raylib/src/external/stb_image_write.h"

#include "../includes/lodepng.h"
#include "../includes/lodepng.c"

#include "../includes/raylib/src/rlgl.h"

#include "../includes/raylib/src/external/glad.h" // GLAD extensions loading library, includes OpenGL headers

u8 *LoadDataFromDisk(const char *fileName, unsigned int *bytesRead, Arena *arena)
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

void LoadDataSetPixel(u8 *dst, u32 i, v4 pixel)
{
	pixel = pixel * 255.0f;
	u32 index = i * 4; 
	dst[index + 0] = (u8)(pixel.x);
	dst[index + 1] = (u8)(pixel.y);
	dst[index + 2] = (u8)(pixel.z);
	dst[index + 3] = (u8)(pixel.w);
}

ImageRawRGBA32 LoadDataIntoRawImage(u8 *fileData, u32 fileSize, GameMemory *gameMemory)
{
	ImageRawRGBA32 result = {};

	int comp = 0;
	int width = 0;
	int height = 0;
	void *outputData = stbi_load_from_memory(fileData, fileSize, &width, &height, &comp, 0);

	if (outputData != NULL)
	{
		int dataSize = GetSizeOfRawRGBA32(iv2{width, height});
		ArenaFree(&gameMemory->rootImageArena);
		gameMemory->rootImageArena = ArenaInit(dataSize);

		if (gameMemory->rootImageArena.memory)
		{
			result.dim.x = width;
			result.dim.y = height;
			result.dataSize = dataSize;
			result.dataU8 = (u8*) ArenaPushSize(&gameMemory->rootImageArena, result.dataSize, {});

			if (comp != 4)
			{
				for (int i = 0, k = 0; i < result.dim.x * result.dim.y; i++)
				{
					v4 pixel;
					switch (comp)
					{
						case 1:
						{
							//NOTE: PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
							pixel.x = (f32)((u8*)outputData)[i] / 255.0f;
							pixel.y = (f32)((u8*)outputData)[i] / 255.0f;
							pixel.z = (f32)((u8*)outputData)[i] / 255.0f;
							pixel.w = 1.0f;
							LoadDataSetPixel(result.dataU8, i, pixel);
						} break;
						case 2:
						{
							//NOTE: PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA
							pixel.x = (f32)((u8*)outputData)[k] / 255.0f;
							pixel.y = (f32)((u8*)outputData)[k] / 255.0f;
							pixel.z = (f32)((u8*)outputData)[k] / 255.0f;
							pixel.w = (f32)((u8*)outputData)[k + 1] / 255.0f;
							k += 2;
							LoadDataSetPixel(result.dataU8, i, pixel);
						} break;
						case 3:
						{
							//NOTE: PIXELFORMAT_UNCOMPRESSED_R8G8B8
							pixel.x = (f32)((u8*)outputData)[k] / 255.0f;
							pixel.y = (f32)((u8*)outputData)[k + 1] / 255.0f;
							pixel.z = (f32)((u8*)outputData)[k + 2] / 255.0f;
							pixel.w = 1.0f;
							k += 3;
							LoadDataSetPixel(result.dataU8, i, pixel);
						} break;
						InvalidDefaultCase
					}
				}
			}
			else
			{
				memcpy(result.dataU8, outputData, result.dataSize);
			}
		}
		else
		{
			String notification = STRING("Failed to allocate memory for the image! Too bad!");
			//TODO: (Ahmayk) Handle error reporting outside this function!
			//InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
		}
		stbi_image_free(outputData);
	}
	else
	{
		String notification = STRING("I don't recognize that as an image. In the future you'll be able to load arbitrary data, but not yet.");
		//TODO: (Ahmayk) Handle error reporting outside this function!
		//InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
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

#if 0
#if DEBUG_MODE
	MemoryProtectWrite(canvas->arenaFilteredPNG.memory, canvas->arenaFilteredPNG.used);
#endif
#endif

	UnlockOnBool(&canvas->filterLock);

	iv2 canvasDim = rootImageRaw->dim; 
	u32 visualizedCanvasDataSize = canvasDim.x * canvasDim.y * sizeof(Color);

	ArenaMarker marker = {};
	ImageRawRGBA32 visualizedFilteredRootImage = {};
	visualizedFilteredRootImage.dim = canvasDim;
	visualizedFilteredRootImage.dataSize = visualizedCanvasDataSize;
	visualizedFilteredRootImage.dataU8 = (u8*) ArenaPushSize(&gameMemory->temporaryArena, visualizedCanvasDataSize, &marker);
	for (u32 y = 0; y < (u32) canvasDim.y; y++)
	{
		u32 filterByteOffset = y + 1;
		u32 startIndex = canvasDim.x * y;
		u32 endIndex = startIndex + canvasDim.x;

		u8 *scanlineFiltered = canvas->imagePNGFiltered.dataU8 + (startIndex * 4) + filterByteOffset;
		u8 *scanlineVisualized = visualizedFilteredRootImage.dataU8 + (startIndex * 4);
		memcpy(scanlineVisualized, scanlineFiltered, canvasDim.x * 4);

		for (u32 i = startIndex; i < endIndex; i++)
		{
			u8 *filteredPixel = canvas->imagePNGFiltered.dataU8 + (i * 4) + filterByteOffset;
			u8 *visualizedPixel = visualizedFilteredRootImage.dataU8 + (i * 4);
			visualizedPixel[3] = 255;
		}
	}

	if (!canvas->textureVisualizedFilteredRootImage.id)
	{
		canvas->textureVisualizedFilteredRootImage.id = rlLoadTexture(visualizedFilteredRootImage.dataU8, visualizedFilteredRootImage.dim.x, visualizedFilteredRootImage.dim.y, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
		canvas->textureVisualizedFilteredRootImage.width = canvasDim.x;
		canvas->textureVisualizedFilteredRootImage.height = canvasDim.y; 
		canvas->textureVisualizedFilteredRootImage.mipmaps = 1;
		canvas->textureVisualizedFilteredRootImage.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	}
	RectIV2 rect = {};
	rect.dim = canvasDim;
	UpdateRectInTexture(&canvas->textureVisualizedFilteredRootImage, visualizedFilteredRootImage.dataU8, rect);
	ArenaPopMarker(marker);
}

RectIV2 GetDrawingRectFromIndex(iv2 imageDim, iv2 rectDim, u32 i)
{
	RectIV2 result;
	result.dim = rectDim; 

	u32 numX = (u32) CeilF32(imageDim.x / (f32) rectDim.x);
	u32 indexX = i % numX;
	u32 indexY = ((u32) FloorF32(i / (f32) numX));
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
				canvas->drawingRectDirtyListFrame[index] = true;
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

u8 *GetPNGFilteredPixel(ImagePNGFiltered *imagePNGFiltered, iv2 pos)
{
	u8 *result = {};
	if (ASSERT(pos.x <= imagePNGFiltered->dim.x && pos.x >= 0 && pos.y <= imagePNGFiltered->dim.y && pos.y >= 0))
	{
		u32 filterByteOffset = pos.y + 1;
		result = imagePNGFiltered->dataU8 + (4 * ((imagePNGFiltered->dim.x * pos.y) + pos.x)) + filterByteOffset;
	}
	else
	{
		result = imagePNGFiltered->dataU8;
	}

	return result;
}

b32 CanvasSwapPoints(Canvas *canvas, iv2 pos1, iv2 pos2)
{
	b32 result = false;

	pos1.x = ClampI32(0, pos1.x, canvas->imagePNGFiltered.dim.x);
	pos1.y = ClampI32(0, pos1.y, canvas->imagePNGFiltered.dim.y);
	pos2.x = ClampI32(0, pos2.x, canvas->imagePNGFiltered.dim.x);
	pos2.y = ClampI32(0, pos2.y, canvas->imagePNGFiltered.dim.y);

#if 1
	LockOnBool(&canvas->filterLock);
	
	u8 *pixel1 = GetPNGFilteredPixel(&canvas->imagePNGFiltered, pos1);
	u8 *pixel2 = GetPNGFilteredPixel(&canvas->imagePNGFiltered, pos2);

	u32 tempPixel;
	memcpy(&tempPixel, pixel1, sizeof(u32));
	memcpy(pixel1, pixel2, sizeof(u32));
	memcpy(pixel2, &tempPixel, sizeof(u32));

	UnlockOnBool(&canvas->filterLock);
#endif

	//Image dst = ImageRawToRayImage(&canvas->drawnImageData);
	//CanvasImageDrawPixel(&dst, pos1.x, pos1.y, Color{1, 0, 0, 0});
	//CanvasImageDrawPixel(&dst, pos2.x, pos2.y, Color{1, 0, 0, 0});

	result = true;

	if (result)
	{
		RectIV2 updateArea;
		updateArea.dim.x = 1; 
		updateArea.dim.y = 1;
		updateArea.pos = pos1;
		CanvasSetDirtyRect(canvas, updateArea);
		updateArea.pos = pos2;
		CanvasSetDirtyRect(canvas, updateArea);
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

//TODO: (Ahmayk) This takes a good deal of time now, the exspensive stuff should be offloaded to another thread
//And we have a fun little loading animation or something
void InitializeCanvas(Canvas *canvas, ImageRawRGBA32 *rootImageRaw, GameMemory *gameMemory)
{
	//NOTE: (Ahmayk) free and reallocate temporary arena to reduce memory size
	//to uncommit the memory there, will reduce our memory footprint
	if (ASSERT(gameMemory->temporaryArena.used == 0))
	{
		u64 size = gameMemory->temporaryArena.size;
		ArenaFree(&gameMemory->temporaryArena);
		gameMemory->temporaryArena = ArenaInit(size);
		gameMemory->temporaryArena.memory;
	}

	ArenaFree(&gameMemory->canvasArena);

	iv2 canvasDim = rootImageRaw->dim;
	u32 visualizedCanvasDataSize = canvasDim.x * canvasDim.y * sizeof(Color);
	u32 conversionArenaSize = visualizedCanvasDataSize + (rootImageRaw->dim.x * 4);
	conversionArenaSize = (u32) (conversionArenaSize * 1.2f);
	AlignPow2U32(&conversionArenaSize, 256);

	//NOTE: (Ahmayk) drawn image, visualized image, drawingRects, dirtyRectsForEachProcesssImage, finalImageRectHashes
	u32 canvasArneaSize = (u32) MaxU32(MegaByte * 1, (u32) (conversionArenaSize * 2.1f));
	gameMemory->canvasArena = ArenaInit(canvasArneaSize);
	//TODO: (Ahmayk) have a better undo plan!
	ArenaFree(&gameMemory->canvasRollbackArena);
	gameMemory->canvasRollbackArena = ArenaInit(MegaByte * 800);

	ArenaGroupFree(&gameMemory->conversionArenaGroup);
	u32 conversionArenaGroupSize = MaxU32(MegaByte * 400, conversionArenaSize * 2 + (KiloByte * 16));
	AlignPow2U32(&conversionArenaSize, 4096);
	gameMemory->conversionArenaGroup = ArenaGroupInit(conversionArenaGroupSize);

	if (gameMemory->temporaryArena.memory && 
		gameMemory->canvasArena.memory &&
		gameMemory->canvasRollbackArena.memory &&
		gameMemory->conversionArenaGroup.masterArena.memory)
	{
		canvas->drawnImageData.dataU8 = ArenaPushSize(&gameMemory->canvasArena, visualizedCanvasDataSize, {});
		canvas->drawnImageData.dim = canvasDim;
		canvas->drawnImageData.dataSize = visualizedCanvasDataSize;
		memset(canvas->drawnImageData.dataU8, 0, visualizedCanvasDataSize);

		canvas->arenaFilteredPNG = ArenaInitFromArena(&gameMemory->canvasArena, conversionArenaSize);

		ArenaGroupResetAndFill(&gameMemory->conversionArenaGroup, conversionArenaSize);

		UploadAndReplaceTexture(&canvas->drawnImageData, &canvas->textureDrawing);

		if (canvas->textureVisualizedFilteredRootImage.id)
		{
			rlUnloadTexture(canvas->textureVisualizedFilteredRootImage.id);
			canvas->textureVisualizedFilteredRootImage = {};
		}
		SetPNGFilterType(canvas, rootImageRaw, gameMemory);

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

		canvas->rollbackSizeCount = (u32) FloorF32((f32)gameMemory->canvasRollbackArena.size / visualizedCanvasDataSize) - 1;
		canvas->rollbackImageData = ARENA_PUSH_ARRAY(&gameMemory->canvasRollbackArena, canvas->rollbackSizeCount * visualizedCanvasDataSize, u8);
		canvas->rollbackIndexStart = 0;
		canvas->rollbackIndexNext = 0;
		canvas->rollbackStartHasProgressed = {};
		canvas->rollbackHasRolledBackOnce = {};
		canvas->saveRollbackOnNextPress = {};
		canvas->dataOnCanvas = {};

		canvas->drawingRectDim = iv2{32, 32};
		canvas->drawingRectCount = GetDrawingRectCount(canvasDim, canvas->drawingRectDim);
		canvas->drawingRectDirtyListFrame = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->drawingRectCount, b32);
		canvas->drawingRectDirtyListProcess = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->drawingRectCount, b32);
		memset(canvas->drawingRectDirtyListFrame, 0, canvas->drawingRectCount * sizeof(b32));
		memset(canvas->drawingRectDirtyListProcess, 0, canvas->drawingRectCount * sizeof(b32));

		canvas->initialized = true;
	}
	else
	{
		ArenaFree(&gameMemory->canvasArena);
		ArenaFree(&gameMemory->canvasRollbackArena);
		ArenaGroupFree(&gameMemory->conversionArenaGroup);
	}
}

b32 InitializeNewImage(GameMemory *gameMemory, ImageRawRGBA32 *rootImageRaw, Canvas *canvas, Texture *loadedTexture, ProcessedImage *processedImages, u32 threadCount)
{
	b32 result = false;
	if (rootImageRaw->dataU8)
	{
		InitializeCanvas(canvas, rootImageRaw, gameMemory);
		if (canvas->initialized)
		{
			UploadAndReplaceTexture(rootImageRaw, loadedTexture);
			for (u32 i = 0; i < threadCount; i++)
			{
				ProcessedImage *processedImage = processedImages + i;
				processedImage->dirtyRectsInProcess = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->drawingRectCount, b32);
				processedImage->finalImageRectHashes = ARENA_PUSH_ARRAY(&gameMemory->canvasArena, canvas->finalImageRectCount, u32);
			}
		}
		else
		{
			//NOTE: (Ahmayk) The only way canvas init can fail is for this reason.
			//Not sure how error handling should work yet...will need to change this once there's more than 1 way for this to fail
			if (rootImageRaw->dataSize > MegaByte * 500)
			{
				String notification = STRING("Yikes, this thing is huge! Sorry, but I failed to allocate the memory I needed to handle that. I'll need to be more memory efficent to handle that monster!");
				//TODO: (Ahmayk) Handle error reporting outside this function!
				//InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
			}
			else
			{
				String notification = STRING("Failed to allocate the memory I needed to handle the new image! Man, not your day, huh?");
				//TODO: (Ahmayk) Handle error reporting outside this function!
				//InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
			}
			*rootImageRaw = {};
		}
	}
	return result;
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

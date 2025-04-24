#pragma once

#include "image.h"
#include "platform_win32.h"

struct ProcessedImage
{
	bool active;
	unsigned int index;
	u8 processBatchIndex;
	ImageRawRGBA32 *rootImageRaw;
	Canvas *canvas;
	ArenaPair arenaPair;
	ImageRawRGBA32 finalProcessedImageRaw;
	unsigned int frameStarted;
	b32 processingComplete;
	b32 *dirtyRectsInProcess;
	u32 *finalImageRectHashes;
};

ProcessedImage *GetFreeProcessedImage(ProcessedImage *processedImages, unsigned int threadCount);
void ResetProcessedImage(ProcessedImage *processedImage, Canvas *canvas);

PLATFORM_WORK_QUEUE_CALLBACK(ProcessImageOnThread);

//TODO: (Ahmayk) Our API probably should not allow calling these functions directly, but instead
//having only a system for them to be put onto a thread. they take time!
ImagePNGFiltered PiratedSTB_EncodePngFilters(ImageRawRGBA32 *imageRaw, Arena *arena, PNG_FILTER_TYPE pngFilterType);
void HashImageRects(ImageRawRGBA32 *imageRaw, iv2 rectDim, u32 **outHashes);

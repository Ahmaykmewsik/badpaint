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
	ImageRawRGBA32 rootImageRawProcessed;
	ImageRawRGBA32 finalProcessedImageRaw;
	ImageRawRGBA32 imageFilteredVisualized;
	unsigned int frameStarted;
	b32 processingComplete;
	b32 *dirtyRectsInProcess;
	u32 *finalImageRectHashes;

	Arena *arenaRoot;
	Arena *arenaFiltered;
	Arena *arenaVisualized;
	Arena *arenaFinal;
};

ProcessedImage *GetFreeProcessedImage(ProcessedImage *processedImages, unsigned int threadCount);
void ResetProcessedImage(ProcessedImage *processedImage, Canvas *canvas);

PLATFORM_WORK_QUEUE_CALLBACK(ProcessImageOnThread);

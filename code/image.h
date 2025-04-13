
#if __clang__
#include "headersNondependent.h"
#include "input.h"
#endif

enum PNG_FILTER_TYPE : i32
{
	PNG_FILTER_TYPE_NONE = 0,
	PNG_FILTER_TYPE_SUB = 1,
	PNG_FILTER_TYPE_UP = 2,
	PNG_FILTER_TYPE_AVERAGE = 3,
	PNG_FILTER_TYPE_PAETH = 4,
	PNG_FILTER_TYPE_OPTIMAL = 5,
};

static const char *G_PNG_FILTER_NAMES[] = {"None", "Sub", "Up", "Average", "Paeth", "Optimal"};

struct ImageRawRGBA32
{
	u8 *dataU8;
	u32 dataSize;
	iv2 dim;
};

struct ImagePNGFiltered
{
	u8 *dataU8;
	u32 dataSize;
	iv2 dim;
	PNG_FILTER_TYPE pngFilterType;
};

struct ImagePNGCompressed
{
	u8 *dataU8;
	u32 dataSize;
	iv2 dim;
};

struct ImagePNGChecksumed
{
	u8 *dataU8;
	u32 dataSize;
	iv2 dim;
};

static String G_CANVAS_STRING_TAG_CHARS = STRING("canvas");

struct Canvas
{
	b32 initialized;
	ImagePNGFiltered imagePNGFiltered;
	Arena arenaFilteredPNG;
	b32 filterLock;

	Texture textureVisualizedFilteredRootImage;

	//Data in drawnImageData:
	//R - brush type
	//G - random value
	//A - processBatchIndex if processing, otherwise 0 (for displaying processes state per pixel)
	ImageRawRGBA32 drawnImageData;
	iv2 drawingRectDim;
	b32 *drawingRectDirtyListFrame;
	b32 *drawingRectDirtyListProcess;
	u32 drawingRectCount;
	Texture textureDrawing;
	u8 processBatchIndex;
	u32 drawingPboIDs[2]; 
	u32 currentDrawingPboID;

	Brush *brush;
	bool proccessAsap;
	PNG_FILTER_TYPE currentPNGFilterType;

	u32 finalImagePboIDs[2]; 
	u32 currentFinalImagePboID;
	iv2 finalImageRectDim;
	u32 finalImageRectCount;
	u32 *cachedFinalImageRectHashes;

	u8 *rollbackImageData;
	u32 rollbackSizeCount;
	u32 rollbackIndexNext;
	u32 rollbackIndexStart;
	b32 rollbackHasRolledBackOnce; 
	b32 rollbackStartHasProgressed; 
};

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
	unsigned int frameFinished;
	b32 *dirtyRectsInProcess;
	u32 *finalImageRectHashes;
};

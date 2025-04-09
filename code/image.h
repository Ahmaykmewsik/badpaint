
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

// NOTE: (Ahmayk) RGBA32
struct ImageRaw
{
	u8 *dataU8;
	u32 dataSize;
	V2 dim;
};

struct ImagePNGFiltered
{
	u8 *dataU8;
	u32 dataSize;
	V2 dim;
	PNG_FILTER_TYPE pngFilterType;
};

struct ImagePNGCompressed
{
	u8 *dataU8;
	u32 dataSize;
	V2 dim;
};

struct ImagePNGChecksumed
{
	u8 *dataU8;
	u32 dataSize;
	V2 dim;
};


static String G_CANVAS_STRING_TAG_CHARS = STRING("canvas");

struct Canvas
{
	b32 initialized;
	ImagePNGFiltered imagePNGFiltered;
	Arena arenaFilteredPNG;
	b32 filterLock;

	Texture textureVisualizedFilteredRootImage;

	ImageRaw drawnImageData;
	Texture textureDrawing;
	Brush *brush;
	bool proccessAsap;
	bool needsTextureUpload;
	bool oldDataPresent;
	PNG_FILTER_TYPE currentPNGFilterType;

	unsigned char *rollbackImageData;
	unsigned int rollbackSizeCount;
	unsigned int rollbackIndexNext;
	unsigned int rollbackIndexStart;
};

struct ProcessedImage
{
	bool active;
	unsigned int index;
	ImageRaw *rootImageRaw;
	Canvas *canvas;
	ArenaPair arenaPair;
	ImageRaw finalProcessedImageRaw;
	unsigned int frameStarted;
	unsigned int frameFinished;
};

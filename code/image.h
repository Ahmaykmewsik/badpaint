
#if __clang__
#include "headersNondependent.h"
#include "input.h"
#endif

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

static const char *G_PNG_FILTER_NAMES[] = {"", "Sub", "Up", "Average", "Paeth", "Optimal"};

static String G_CANVAS_STRING_TAG_CHARS = STRING("canvas");

struct Canvas
{
    Image filteredRootImage;
    Image drawnImageData;
    Texture texture;
    Brush *brush;
    bool proccessAsap;
    bool needsTextureUpload;
    bool oldDataPresent;

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

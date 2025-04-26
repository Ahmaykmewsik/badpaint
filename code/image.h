#pragma once
#include <base.h>
#include "platform_main.h"
#include "../includes/raylib/src/raylib.h"
#include "ui/ui_core.h"

enum PNG_FILTER_TYPE : i32
{
	PNG_FILTER_TYPE_NONE = 0,
	PNG_FILTER_TYPE_SUB = 1,
	PNG_FILTER_TYPE_UP = 2,
	PNG_FILTER_TYPE_AVERAGE = 3,
	PNG_FILTER_TYPE_PAETH = 4,
	PNG_FILTER_TYPE_OPTIMAL = 5,
};

static const char *PNG_FILTER_NAMES[] = {"None", "Sub", "Up", "Average", "Paeth", "Optimal"};

enum BADPAINT_TOOL_TYPE
{
	BADPAINT_TOOL_PENCIL,
	BADPAINT_TOOL_ERASER,
	BADPAINT_TOOL_TEST,
	BADPAINT_TOOL_COUNT,
};

enum BADPAINT_PIXEL_TYPE : u8
{
	BADPAINT_PIXEL_TYPE_NONE = 0,
	BADPAINT_PIXEL_TYPE_REMOVE = 1,
	BADPAINT_PIXEL_TYPE_MAX = 2,
	BADPAINT_PIXEL_TYPE_SHIFT = 3,
	BADPAINT_PIXEL_TYPE_RANDOM= 4,
};

static ColorU32 BADPAINT_PIXEL_TYPE_COLORS_PRIMARY[] =
{
	{},
	{ 230, 41, 55, 227 }, // RED
	{ 253, 249, 0, 227 }, // YELLOW
	{ 0, 121, 241, 227 }, //BLUE
	{ 200, 122, 255, 227 }, //PURPLE
};
static ColorU32 BADPAINT_PIXEL_TYPE_COLORS_PROCESSING[] =
{
	{},
	{ 230, 41, 55, 127 }, // RED
	{ 253, 249, 0, 127 }, // YELLOW
	{ 0, 121, 241, 127 }, //BLUE
	{ 200, 122, 255, 127 }, //PURPLE
};

struct BadpaintPixel
{
	BADPAINT_PIXEL_TYPE badpaintPixelType;
	union 
	{
		u8 r1Null;
		u8 r1RandomValue;
	};
	union 
	{
		u8 r2Null;
	};
	union 
	{
		u8 r3Null;
	};
	u8 processBatchIndex;
};

struct TextureGPU
{
	Texture texture;
	iv2 dim;
	u32 pboIDs[2]; 
	u32 currentPboID;
};

struct ImageBadpaintPixels
{
	BadpaintPixel *dataBadpaintPixel;
	u32 dataSize;
	iv2 dim;
	iv2 drawingRectDim;
	b32 *drawingRectDirtyListFrame;
	b32 *drawingRectDirtyListProcess;
	u32 drawingRectCount;
	TextureGPU textureGPU;
};

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

struct Canvas
{
	b32 initialized;
	u8 processBatchIndex;

	Arena *areaFiltered;
	Arena *areaVisualized;
	Arena *areaFinal;

	ImageBadpaintPixels badpaintPixelsRootImage;
	ImageBadpaintPixels badpaintPixelsPNGFiltered;
	ImageBadpaintPixels badpaintPixelsFinalImage;

	TextureGPU textureGPURoot;
	TextureGPU textureGPUPNGFiltered;
	TextureGPU textureGPUFinal;
	iv2 finalImageRectDim;
	u32 finalImageRectCount;
	u32 *cachedFinalImageRectHashes;

	bool proccessAsap;
	PNG_FILTER_TYPE currentPNGFilterType;

	u8 *rollbackImageData;
	u32 rollbackSizeCount;
	u32 rollbackIndexNext;
	u32 rollbackIndexStart;
	b32 rollbackHasRolledBackOnce; 
	b32 rollbackStartHasProgressed; 
	b32 saveRollbackOnNextPress;
	b32 dataOnCanvas;
};

struct ProcessedImage;
struct AppState;

b32 InitializeNewImage(GameMemory *gameMemory, AppState *appState);
RectIV2 GetDrawingRectFromIndex(iv2 imageDim, iv2 rectDim, u32 i);
u32 GetDrawingRectCount(iv2 imageDim, iv2 rectDim);
void UpdateRectInTexture(Texture *texture, void *data, RectIV2 rect);
void RenderAndUploadBadpaintPixelImage(ImageBadpaintPixels *imageBadpaintPixels);

struct Tool
{
	UiTextureView uiTextureViews[7]; //INTERACTION_STATE_COUNT
};


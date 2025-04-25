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

	Arena *areaFiltered;
	Arena *areaVisualized;
	Arena *areaFinal;
	Texture textureVisualizedFilteredRootImage;

	//Data in drawnImageData:
	//R - brush type
	//G - random value
	//A - processBatchIndex if processing, otherwise 0 (for displaying processes state per pixel)
	ImageRawRGBA32 drawnImageData;
	ImageRawRGBA32 drawnImageDataRoot;
	iv2 drawingRectDim;
	b32 *drawingRectDirtyListFrame;
	b32 *drawingRectDirtyListProcess;
	u32 drawingRectCount;
	Texture textureDrawing;
	u8 processBatchIndex;
	u32 drawingPboIDs[2]; 
	u32 currentDrawingPboID;

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
	b32 saveRollbackOnNextPress;
	b32 dataOnCanvas;
};

struct ProcessedImage;

b32 InitializeNewImage(GameMemory *gameMemory, ImageRawRGBA32 *rootImageRaw, Canvas *canvas, Texture *loadedTexture, ProcessedImage *processedImages, u32 threadCount);
RectIV2 GetDrawingRectFromIndex(iv2 imageDim, iv2 rectDim, u32 i);
u32 GetDrawingRectCount(iv2 imageDim, iv2 rectDim);
void UpdateRectInTexture(Texture *texture, void *data, RectIV2 rect);

enum BADPAINT_BRUSH_EFFECT : u32
{
    BADPAINT_BRUSH_EFFECT_REMOVE = 1,
    BADPAINT_BRUSH_EFFECT_MAX = 2,
    BADPAINT_BRUSH_EFFECT_SHIFT = 3,
    BADPAINT_BRUSH_EFFECT_RANDOM = 4,
};

static ColorU32 BRUSH_EFFECT_COLORS_PRIMARY[] =
{
	{},
	{ 230, 41, 55, 187 }, // RED
	{ 230, 41, 55, 187 }, // RED
	{ 253, 249, 0, 187 }, // YELLOW
	{ 0, 121, 241, 187 }, //BLUE
	{ 200, 122, 255, 187 }, //PURPLE
};
static ColorU32 BRUSH_EFFECT_COLORS_PROCESSING[] =
{
	{},
	{ 230, 41, 55, 127 }, // RED
	{ 253, 249, 0, 127 }, // YELLOW
	{ 0, 121, 241, 127 }, //BLUE
	{ 200, 122, 255, 127 }, //PURPLE
};

enum BADPAINT_TOOL_TYPE
{
	BADPAINT_TOOL_PENCIL,
	BADPAINT_TOOL_ERASER,
	BADPAINT_TOOL_TEST,
	BADPAINT_TOOL_COUNT,
};

struct Tool
{
	UiTextureView uiTextureViews[7]; //INTERACTION_STATE_COUNT
};


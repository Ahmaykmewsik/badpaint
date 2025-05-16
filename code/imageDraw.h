#pragma once

#include "image.h"

b32 CanvasDrawSpray(ImageBadpaintPixels *imageBadpaintPixels, iv2 posIV2, u32 toolSize, BadpaintPixel *badpaintPixel);
BadpaintPixel CreateBadpaintPixel(BADPAINT_PIXEL_TYPE badpaintPixelType, ImageBadpaintPixels *imageBadpaintPixels, iv2 endPosIV2, u32 toolSize, u8 processBatchIndex);
b32 CanvasDrawCircleStroke(ImageBadpaintPixels *imageBadpaintPixels, iv2 startPos, iv2 endPos, u32 radius, BadpaintPixel *badpaintPixel);
b32 ImageSwapPoints(ImageRawRGBA32 *imageRaw, iv2 pos1, iv2 pos2);

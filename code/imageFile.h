#pragma once

#include <base.h>
#include "image.h"

u8 *LoadDataFromDisk(String fileName, unsigned int *bytesRead, Arena *arena);
ImageRawRGBA32 LoadDataIntoRawImage(u8 *fileData, u32 fileSize, GameMemory *gameMemory);
bool ExportImage(Image image, String filepath);

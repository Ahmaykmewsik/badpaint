#pragma once

#include <base.h>
#include <ui/ui_core.h>
#include "../includes/raylib/src/raylib.h"
#include "image.h"
#include "platform_main.h"
#include "widgets.h"

struct AppState
{
	Font defaultFont;
	UiFont defaultUiFont;
	Texture toolbrushSpriteSheet;

	ImageRawRGBA32 rootImageRaw;
	ProcessedImage *processedImages;
	u32 processedImageCount;

	Tool tools[BADPAINT_TOOL_COUNT];
    BADPAINT_PIXEL_TYPE currentBadpaintPixelType;
	BADPAINT_TOOL_TYPE currentTool;
	u32 toolSize;
	Canvas canvas;
	UiPanel rootUiPanel;

	b32 imageIsBroken;

	u32 hashOpenUiMenu;
};

b32 IsCommandKeyBindingDown(u32 command);
b32 IsCommandKeyBindingPressed(u32 command);

#pragma once

#include <base.h>
#include <ui/ui_core.h>
#include "../includes/raylib/src/raylib.h"
#include "image.h"
#include "platform_main.h"
#include "widgets.h"

struct AppState
{
	UiFont defaultUiFont;

	Tool tools[BADPAINT_TOOL_COUNT];
    BADPAINT_BRUSH_EFFECT currentBrushEffect;
	BADPAINT_TOOL_TYPE currentTool;
	u32 toolSize;
	Texture loadedTexture;
	Canvas canvas;
	UiPanel rootUiPanel;
};


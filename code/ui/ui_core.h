#pragma once

#include <base.h>

enum UI_SIZE_KIND
{
	UI_SIZE_KIND_NULL,
	UI_SIZE_KIND_PIXELS,
	UI_SIZE_KIND_TEXTURE,
	UI_SIZE_KIND_TEXT,
	UI_SIZE_KIND_PERCENT_OF_PARENT,
	UI_SIZE_KIND_PERCENT_OF_OTHER_AXIS,
	UI_SIZE_KIND_SUM_OF_CHILDREN,
};

struct UiSize
{
	UI_SIZE_KIND kind;
	f32 value;
};

enum UI_AXIS
{
	UI_AXIS_X,
	UI_AXIS_Y,
	UI_AXIS_COUNT,
};

enum UI_FLAGS
{
	UI_FLAG_NULL = (0 << 0),
	UI_FLAG_DRAW_TEXT = (1 << 1),
	UI_FLAG_DRAW_BACKGROUND = (1 << 2),
	UI_FLAG_DRAW_BORDER = (1 << 4),
	UI_FLAG_DRAW_TEXTURE = (1 << 5),

	UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT = (1 << 9),
	UI_FLAG_CHILDREN_MANUAL_POSITION = (1 << 10),
	UI_FLAG_MANUAL_POSITION = (1 << 11),
	UI_FLAG_ALIGN_TEXT_CENTERED = (1 << 13),
	UI_FLAG_ALIGN_TEXT_RIGHT = (1 << 14),
	UI_FLAG_ALIGN_TEXTURE_CENTERED = (1 << 15),
	UI_FLAG_INTERACTABLE = (1 << 17),
	UI_FLAG_CENTER_IN_PARENT = (1 << 19),
};

struct UiBlockColors
{
	//TODO: Think of different names for these colors I'm always forgetting which goes to what
	ColorU32 backColor;
	ColorU32 frontColor;
	ColorU32 borderColor;
};

struct UiFont
{
	u32 id;
	void *data;
};

struct UiTexture
{
	u32 id;
	iv2 dim;
	void *data;
};

struct UiBlock
{
	u32 hash;

	UiBlock *firstChild;
	UiBlock *lastChild;
	UiBlock *next;
	UiBlock *prev;
	UiBlock *parent;

	u64 flags;
	String string;
	v2 textDim;
	UiFont uiFont;

	UiSize uiSizes[UI_AXIS_COUNT];
	v2 relativePixelPosition;
	UiTexture uiTexture;
	v2 manualDim;

	UiBlockColors uiBlockColors;

	v2 cursorRelativePixelPos;

	v2 computedRelativePixelPos;
	RectV2 rect;
};

#define MAX_UI_BLOCKS 1000
struct UiBuffer
{
	UiBlock uiBlocks[MAX_UI_BLOCKS];
	u32 uiBlockCount;
	Arena arena;
};

struct UiState
{
	b32 initialized;
	UiBuffer uiBuffers[2];
	u32 uiBufferIndex;

	UiBlock *parentStack[64];
	int parentStackCount;
};

UiState *UiInit(Arena *arena);
UiBlock *UiGetBlockOfHashLastFrame(UiState *uiState, u32 hash);
UiBlock *UiCreateBlock(UiState *uiState);
void UiPushParent(UiState *uiState, UiBlock *uiBlock);
void UiPopParent(UiState *uiState, UiBlock *uiBlock);
void UiLayoutBlocks(UiBuffer *uiBuffer);
void UiEndFrame(UiState *uiState);

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define DEFER_LOOP(begin, end) for (int CONCAT(_i_, __LINE__) = ((begin), 0); !CONCAT(_i_, __LINE__); CONCAT(_i_, __LINE__) += 1, (end))
#define UI_PARENT_SCOPE(uiState, uiBlock) DEFER_LOOP(UiPushParent(uiState, uiBlock), UiPopParent(uiState, uiBlock))

#pragma once

#include <base.h>

enum UI_POSITION_TYPE
{
	UI_POSITION_AUTO,
	UI_POSITION_ABSOLUTE,
	UI_POSITION_RELATIVE,
	UI_POSITION_PERCENT_OF_PARENT,
};
struct UiPosition
{
	UI_POSITION_TYPE type;
	f32 value;
};

enum UI_SIZE_TYPE
{
	UI_SIZE_NULL,
	UI_SIZE_PIXELS,
	UI_SIZE_TEXTURE,
	UI_SIZE_TEXT,
	UI_SIZE_PERCENT_OF_PARENT,
	UI_SIZE_PERCENT_OF_OTHER_AXIS,
	UI_SIZE_SUM_OF_CHILDREN,
	UI_SIZE_FILL,
};
struct UiSize
{
	UI_SIZE_TYPE type;
	f32 value;
};

enum UI_CHILD_ALIGN_TYPE
{
	UI_CHILD_ALIGN_START,
	UI_CHILD_ALIGN_CENTER,
	UI_CHILD_ALIGN_END,
};

enum UI_TEXT_ALIGN_TYPE
{
	UI_TEXT_ALIGN_LEFT,
	UI_TEXT_ALIGN_CENTER,
	UI_TEXT_ALIGN_RIGHT,
};

enum UI_CHILD_LAYOUT_TYPE
{
	UI_CHILD_LAYOUT_LEFT_TO_RIGHT,
	UI_CHILD_LAYOUT_TOP_TO_BOTTOM,
};

enum UI_AXIS : u32
{
	UI_AXIS_X = 0,
	UI_AXIS_Y = 1,
	UI_AXIS_COUNT = 2,
};

enum UI_FLAGS
{
	UI_FLAG_NULL = (0 << 0),
	UI_FLAG_DRAW_TEXT = (1 << 1),
	UI_FLAG_DRAW_BACKGROUND = (1 << 2),
	UI_FLAG_DRAW_BORDER = (1 << 4),
	UI_FLAG_DRAW_TEXTURE = (1 << 5),
	UI_FLAG_DRAW_LINE_TOPLEFT_BOTTOMRIGHT = (1 << 6),
	UI_FLAG_DRAW_LINE_BOTTOMLEFT_TOPRIGHT = (1 << 7),

	UI_FLAG_INTERACTABLE = (1 << 10),
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

struct UiTextureView
{
	u32 id;
	iv2 dim;
	RectIV2 viewRect;
	void *data;
};

struct UiBlock
{
	u32 flags;
	u32 hash;

	UiSize uiSizes[UI_AXIS_COUNT];
	UiPosition uiPosition[UI_AXIS_COUNT];
	UI_CHILD_ALIGN_TYPE uiChildAlignTypes[UI_AXIS_COUNT];
	UI_CHILD_LAYOUT_TYPE uiChildLayoutType;
	UI_TEXT_ALIGN_TYPE uiTextAlignTypes[UI_AXIS_COUNT];

	UiBlock *firstChild;
	UiBlock *lastChild;
	UiBlock *next;
	UiBlock *prev;
	UiBlock *parent;

	String string;
	v2 textDim;
	UiFont uiFont;
	UiTextureView uiTextureView;
	UiBlockColors uiBlockColors;

	RectV2 rect;

#if DEBUG_MODE
	String debugString;
#endif
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
UiBlock *UiCreateRootBlock(UiState *uiState);
void UiPushParent(UiState *uiState, UiBlock *uiBlock);
void UiPopParent(UiState *uiState, UiBlock *uiBlock);
void UiLayoutBlocks(UiBuffer *uiBuffer, iv2 windowDim, Arena *temporaryArena);
void UiEndFrame(UiState *uiState);

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define DEFER_LOOP(begin, end) for (int CONCAT(_i_, __LINE__) = ((begin), 0); !CONCAT(_i_, __LINE__); CONCAT(_i_, __LINE__) += 1, (end))
#define UI_PARENT_SCOPE(uiState, uiBlock) DEFER_LOOP(UiPushParent(uiState, uiBlock), UiPopParent(uiState, uiBlock))

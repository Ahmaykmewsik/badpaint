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

enum UI_POSITION_OFFSET_TYPE
{
	UI_POSITION_OFFSET_PIXELS,
	UI_POSITION_OFFSET_PERCENT_OF_SELF,
};
struct UiPositionOffset
{
	UI_POSITION_OFFSET_TYPE type;
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
	UI_SIZE_FIT_CHILDREN,
	UI_SIZE_FILL,
	UI_SIZE_FILL_FIXED,
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
	UI_FLAG_TINT_TEXTURE = (1 << 11),
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
	RectIV2 viewRect;
	void *data;
};

struct UiBlock
{
	u32 flags;
	u32 hash;

	UiSize uiSizes[UI_AXIS_COUNT];
	UiPosition uiPosition[UI_AXIS_COUNT];
	UiPositionOffset uiPositionOffset[UI_AXIS_COUNT];
	UI_CHILD_ALIGN_TYPE uiChildAlignTypes[UI_AXIS_COUNT];
	UI_CHILD_LAYOUT_TYPE uiChildLayoutType;
	UI_TEXT_ALIGN_TYPE uiTextAlignTypes[UI_AXIS_COUNT];
	iv2 padding; //ignored on UI_SIZE_FILL and UI_SIZE_FILL_FIXED

	UiBlock *firstChild;
	UiBlock *lastChild;
	UiBlock *next;
	UiBlock *prev;
	UiBlock *parent;

	String string;
	UiFont uiFont;
	UiTextureView uiTextureView;
	UiBlockColors uiBlockColors;
	u32 depthLayer;

	v2 textDim;
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
	u32 sortIndexArray[MAX_UI_BLOCKS];
	Arena arena;
};

enum UI_CURSOR_TYPE
{
	UI_CURSOR_TYPE_DEFAULT,
	UI_CURSOR_TYPE_RESIZE_UP_DOWN,
	UI_CURSOR_TYPE_RESIZE_LEFT_RIGHT,
};

enum INTERACTION_STATE
{
	INTERACTION_STATE_NONACTIVE_NEUTRAL,
	INTERACTION_STATE_NONACTIVE_HOVERED,
	INTERACTION_STATE_DOWN,
	INTERACTION_STATE_ACTIVE_NEUTRAL,
	INTERACTION_STATE_ACTIVE_HOVERED,
	INTERACTION_STATE_DISABLED,
	INTERACTION_STATE_COUNT,
};

struct UiInteractionFrameInput
{
	iv2 windowDim;
	iv2 mousePixelPos;
	b32 isMouseLeftDown;
	b32 isMouseLeftPressed;
	b32 isMouseLeftReleased;
};

struct UiInteractionState
{
	UiInteractionFrameInput uiInteractionFrameInput;
	iv2 mousePixelPosPrevious;
	u32 lastPressedUiHash;
	iv2 lastPressedPos;

	u32 hashMouseHover;
	u32 hashMouseDown;
	u32 hashMousePressed;
	u32 hashMouseReleased;
	UI_CURSOR_TYPE currentUiCursorType;
};

INTERACTION_STATE GetInteractionState(UiInteractionState *UiInteractionState, u32 hash, b32 isActive, b32 isDisabled, b32 downOverride);

struct UiState
{
	b32 initialized;
	UiBuffer uiBuffers[2];
	u32 uiBufferIndex;

	UiBlock *parentStack[64];
	u32 parentStackCount;

	UiInteractionState uiInteractionState;
};

UiState *UiInit(Arena *arena);
void UiInteractionStateUpdate(UiState *uiState, UiInteractionFrameInput *uiInteractionFrameInput);
void UiResetCurrentUiBuffer(UiState *uiState);
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

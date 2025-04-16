#pragma once

#include <base.h>

//TODO: (Ahmayk) remove dependency
#include <input.h>

static const char *G_UI_HASH_TAG = "##";

enum UI_SIZE_KIND
{
	UI_SIZE_KIND_NULL,
	UI_SIZE_KIND_PIXELS,
	UI_SIZE_KIND_TEXTURE,
	UI_SIZE_KIND_TEXT,
	UI_SIZE_KIND_PERCENT_OF_PARENT,
	UI_SIZE_KIND_CHILDREN_OF_SUM,
	UI_SIZE_KIND_SCALE_TEXTURE_IN_PARENT,
};

struct UiSize
{
	UI_SIZE_KIND kind;
	float value;
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
	UI_FLAG_POSITION_IS_CENTER = (1 << 18),
	UI_FLAG_CENTER_IN_PARENT = (1 << 19),
};

struct UiSettings
{
	float startValue;
	float endValue;
	UiSize uiSizes[UI_AXIS_COUNT];
	//TODO: Think of different names for these colors I'm always forgetting which goes to what
	Color backColor;
	Color frontColor;
	Color detailColor;
	Color borderColor;
	Font font;
};

struct UiInputs
{
	float value;
	v2 relativePixelPosition;
	Texture texture;
	v2 manualDim;
	COMMAND command;
	SLIDER_ACTION sliderAction;
};

struct UiBlock
{
	unsigned int index;
	unsigned int frameRendered;

	UiBlock *firstChild;
	UiBlock *lastChild;
	UiBlock *next;
	UiBlock *prev;
	UiBlock *parent;

	u64 flags;
	String string;
	String keyString;
	v2 textDim;

	UiInputs uiInputs;
	UiSettings uiSettings;

	b32 hovered;
	b32 pressed;
	b32 down;
	v2 cursorRelativePixelPos;

	v2 computedRelativePixelPos;
	RectV2 rect;
};

struct UiHashEntry
{
	UiBlock *uiBlock;
	String keyString;
};

struct UiState
{
#define MAX_UI_BOXES 1000
	UiBlock uiBlockes[2][MAX_UI_BOXES];
	//TODO: does it make sense to have 1 uiBlockCount when we have two buffers?
	u32 uiBlockCount;

	UiHashEntry uiHashEntries[MAX_UI_BOXES];

	UiBlock *parentStack[20];
	int parentStackCount;

	UiSettings uiSettings;

	Arena *twoFrameArenaLastFrame;
	Arena *twoFrameArenaThisFrame;

	//NOTE: Ahmayk(Temporary hack)
	CommandInput *commandInputs;
};

struct ReactiveUiColor
{
	Color neutral;
	Color hovered;
	Color down;
	Color disabled;
};

struct ReactiveUiColorState
{
	ReactiveUiColor active;
	ReactiveUiColor nonActive;
};

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define UiDeferLoop(begin, end) for (int CONCAT(_i_, __LINE__) = ((begin), 0); !CONCAT(_i_, __LINE__); CONCAT(_i_, __LINE__) += 1, (end))
#define UiParent() UiDeferLoop(PushUiParent(), PopUiParent())

UiState *GetUiState();
UiInputs *GetUiInputs();

bool IsFlag(UiBlock *uiBlock, unsigned int flags = 0);
UiBlock *GetUiBlockLastFrameOfStringKey(String stringKey);
void SetUiAxis(UiSize uiSize1, UiSize uiSize2);
void CreateUiBlock(unsigned int flags = 0, String string = {});
void PushUiParent();
void PopUiParent();
Color GetReactiveColor(CommandInput *commandInputs, UiBlock *uiBlockLastFrame, ReactiveUiColor reactiveUiColor, bool disabled);
void CreateUiButton(String string, ReactiveUiColorState reactiveUiColorState, bool active, bool disabled = false);
ReactiveUiColorState CreateButtonReactiveUiColorState(Color color);

void CalculateUiPosGivenReletativePositions(UiBlock *uiBlock);
void CalculateUiRelativePositions(UiBlock *uiBlock);
void CalculateUiUpwardsDependentSizes(UiBlock *uiBlock);
void CalculateUiDownwardsDependentSizes(UiBlock *uiBlock);
void RenderUiEntries(UiBlock *uiBlock, v2 windowPixelDim, int uiDepth = 0);

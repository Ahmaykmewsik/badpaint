#pragma once

#if __clang__
#include "headersNondependent.h"
#endif

static String G_UI_HASH_TAG_STRING = {};

enum UI_SIZE_KIND
{
    UI_SIZE_KIND_NULL,
    UI_SIZE_KIND_PIXELS,
    UI_SIZE_KIND_TEXTURE,
    UI_SIZE_KIND_TEXT_NO_WRAPPING,
    UI_SIZE_KIND_TEXT_WRAP_TO_PARENT,
    UI_SIZE_KIND_PERCENT_OF_PARENT,
    UI_SIZE_KIND_CHILDREN_OF_SUM,
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

    UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT = (1 << 11),
    UI_FLAG_CHILDREN_MANUAL_LAYOUT = (1 << 12),
    UI_FLAG_ALIGN_TEXT_CENTERED = (1 << 13),
    UI_FLAG_ALIGN_TEXT_RIGHT = (1 << 14),
    UI_FLAG_ALIGN_TEXTURE_CENTERED = (1 << 15),
    UI_FLAG_INTERACTABLE = (1 << 16),
    UI_FLAG_POSITION_IS_CENTER = (1 << 17),
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
    V2 pixelPosition;
    Texture texture;
};

static UiInputs *G_UI_INPUTS = {};

struct UiBox
{
    unsigned int index;
    unsigned int frameRendered;

    UiBox *firstChild;
    UiBox *lastChild;
    UiBox *next;
    UiBox *prev;
    UiBox *parent;

    Flags flags;
    String string;
    String keyString;
    V2 textDim;

    UiInputs uiInputs;
    UiSettings uiSettings;

    bool hovered;
    bool pressed;
    bool down;
    V2 cursorRelativePixelPos;

    V2 computedRelativePixelPos;
    Rect rect;
};

struct UiHashEntry
{
    UiBox *uiBox;
    String keyString;
};

struct UiState
{
#define MAX_UI_BOXES 1000
    UiBox uiBoxes[2][MAX_UI_BOXES];
    //TODO: does it make sense to have 1 uiBoxCount when we have two buffers?
    int uiBoxCount;

    UiHashEntry uiHashEntries[MAX_UI_BOXES];

    UiBox *parentStack[20];
    int parentStackCount;

    UiSettings uiSettings;

    MemoryArena *twoFrameArenaLastFrame;
    MemoryArena *twoFrameArenaThisFrame;
};

static UiState *G_UI_STATE = {};

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

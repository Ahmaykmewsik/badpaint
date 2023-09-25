#pragma once

#include "headers.h"

void CreateUiBox(unsigned int flags = 0, String string = {})
{
    Assert(G_UI_INPUTS);
    Assert(G_UI_STATE);

    int uiBoxArrayIndexThisFrame = GetFrameModIndexThisFrame();
    Assert(G_UI_STATE->uiBoxCount < ArrayCount(G_UI_STATE->uiBoxes[uiBoxArrayIndexThisFrame]));

    UiBox *uiBox = &G_UI_STATE->uiBoxes[uiBoxArrayIndexThisFrame][G_UI_STATE->uiBoxCount];
    *uiBox = {};
    uiBox->index = G_UI_STATE->uiBoxCount;
    G_UI_STATE->uiBoxCount++;

    if (G_UI_STATE->parentStackCount)
    {
        uiBox->parent = G_UI_STATE->parentStack[G_UI_STATE->parentStackCount - 1];
        Assert(uiBox->parent);

        if (uiBox->parent->firstChild)
        {
            Assert(uiBox->parent->lastChild);
            uiBox->prev = uiBox->parent->lastChild;
            uiBox->parent->lastChild->next = uiBox;
        }
        else
        {
            uiBox->parent->firstChild = uiBox;
        }

        uiBox->parent->lastChild = uiBox;
    }

    uiBox->flags = flags;
    uiBox->uiSettings = G_UI_STATE->uiSettings;
    uiBox->frameRendered = G_CURRENT_FRAME;

    StringArray stringSplitByHashTag = {};
    if (string.length)
    {
        uiBox->string = string;

        Assert(G_UI_HASH_TAG_STRING.length);
        stringSplitByHashTag = SplitStringOnceByTag(uiBox->string, G_UI_HASH_TAG_STRING, G_UI_STATE->twoFrameArenaThisFrame);

        if (stringSplitByHashTag.count == 2)
        {
            uiBox->string = stringSplitByHashTag.strings[0];
            uiBox->keyString = stringSplitByHashTag.strings[1];
        }
        else if (stringSplitByHashTag.count == 1)
        {
            MoveStringToArena(&uiBox->string, G_UI_STATE->twoFrameArenaThisFrame);
        }
        else
        {
            InvalidCodePath
        }

        Assert(uiBox->uiSettings.fontSize);
        Vector2 textDim = MeasureTextEx(uiBox->uiSettings.font, uiBox->string.chars, uiBox->uiSettings.fontSize, 1);
        uiBox->textDim = RayVectorToV2(textDim);
    }


    uiBox->uiInputs = *G_UI_INPUTS;
    *G_UI_INPUTS = {};

    if (stringSplitByHashTag.count > 1)
    {
        String keyString = stringSplitByHashTag.strings[1];
        if (keyString.length)
        {
            unsigned int hashValue = Murmur3String(keyString.chars, 0);

            for (int i = 0;
                 i < keyString.length;
                 i++)
            {
                unsigned int index = (hashValue + i) % ArrayCount(G_UI_STATE->uiHashEntries);
                UiHashEntry *uiHashEntry = &G_UI_STATE->uiHashEntries[index];
                if (!uiHashEntry->uiBox || uiHashEntry->uiBox->frameRendered < G_CURRENT_FRAME)
                {
                    uiHashEntry->uiBox = uiBox;
                    uiHashEntry->keyString = keyString;
                    break;
                }
            }
        }
    }
}

void CreateUiText(String string)
{
    if (string.length)
    {
        CreateUiBox(UI_FLAG_DRAW_TEXT, string);
    }
}

void CreateUiTextWithBackground(String string, unsigned int flags = 0)
{
    if (string.length)
    {
        CreateUiBox(UI_FLAG_DRAW_TEXT | UI_FLAG_DRAW_BACKGROUND | flags, string);
    }
}

void CreateUiText(const char *c)
{
    String string = CreateString(c);
    CreateUiText(string);
}

void SetUiAxis(UiSize uiSize1, UiSize uiSize2)
{
    G_UI_STATE->uiSettings.uiSizes[0] = uiSize1;
    G_UI_STATE->uiSettings.uiSizes[1] = uiSize2;
}

void SetUiAxisToPixelDim(V2 pixelDim)
{
    SetUiAxis({UI_SIZE_KIND_PIXELS, pixelDim.x}, {UI_SIZE_KIND_PIXELS, pixelDim.y});
}

void SetUiTimlineRowAxisPercentOfX(float percentOfParentX)
{
    G_UI_STATE->uiSettings.uiSizes[0] = UiSize{UI_SIZE_KIND_PERCENT_OF_PARENT, percentOfParentX};
    G_UI_STATE->uiSettings.uiSizes[1] = UiSize{UI_SIZE_KIND_PERCENT_OF_PARENT, 1};
}

void PushPixelSize(V2 pixelSize)
{
    G_UI_STATE->uiSettings.uiSizes[0] = {UI_SIZE_KIND_PIXELS, pixelSize.x};
    G_UI_STATE->uiSettings.uiSizes[1] = {UI_SIZE_KIND_PIXELS, pixelSize.x};
}

bool IsFlag(UiBox *uiBox, unsigned int flags = 0)
{
    bool result = uiBox->flags & flags;
    return result;
}

unsigned int GetThisUiBoxIndex()
{
    unsigned int result = 0;

    int uiBoxArrayIndexThisFrame = GetFrameModIndexThisFrame();
    if (G_UI_STATE->uiBoxCount)
        result = G_UI_STATE->uiBoxes[uiBoxArrayIndexThisFrame][G_UI_STATE->uiBoxCount - 1].index;

    return result;
}

void PushUiParent()
{
    Assert(G_UI_STATE->parentStackCount < ArrayCount(G_UI_STATE->parentStack));

    int uiBoxArrayIndexThisFrame = GetFrameModIndexThisFrame();
    G_UI_STATE->parentStack[G_UI_STATE->parentStackCount] = &G_UI_STATE->uiBoxes[uiBoxArrayIndexThisFrame][G_UI_STATE->uiBoxCount - 1];
    G_UI_STATE->parentStackCount++;
}

void PopUiParent()
{
    Assert(G_UI_STATE->parentStackCount > 0);
    G_UI_STATE->parentStackCount--;
}

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define UiDeferLoop(begin, end) for (int CONCAT(_i_, __LINE__) = ((begin), 0); !CONCAT(_i_, __LINE__); CONCAT(_i_, __LINE__) += 1, (end))

#define UiParent() UiDeferLoop(PushUiParent(), PopUiParent())

void CalculateUiUpwardsDependentSizes(UiBox *uiBox)
{
    if (uiBox)
    {
        for (int j = 0;
             j < ArrayCount(uiBox->uiSettings.uiSizes);
             j++)
        {
            UiSize uiSize = uiBox->uiSettings.uiSizes[j];
            switch (uiSize.kind)
            {
            case UI_SIZE_KIND_PERCENT_OF_PARENT:
            {
                if (uiBox->parent && uiBox->parent->rect.dim.elements[j])
                {
                    uiBox->rect.dim.elements[j] = (uiBox->parent->rect.dim.elements[j] * uiSize.value);
                }
                break;
            }
            case UI_SIZE_KIND_TEXT_WRAP_TO_PARENT:
            {
                if (uiBox->parent)
                {
                    //TODO: Uhhhhhh??? Finish this?
                    // MemoryArena *arena = GetTwoFrameArenaThisFrame(gameState->gameMemory);
                    // uiBox->textTokenArray = TokenizeText(gameState, uiBox->string, uiBox->uiSettings.fontInstance, uiBox->parent->rect.dim.x, arena);
                }

                break;
            }
            case UI_SIZE_KIND_CHILDREN_OF_SUM:
            case UI_SIZE_KIND_PIXELS:
            case UI_SIZE_KIND_TEXT_NO_WRAPPING:
                break;
                InvalidDefaultCase
            }
        }

        CalculateUiUpwardsDependentSizes(uiBox->firstChild);
        CalculateUiUpwardsDependentSizes(uiBox->next);
    }
}

void CalculateUiDownwardsDependentSizes(UiBox *uiBox)
{
    if (uiBox)
    {
        CalculateUiDownwardsDependentSizes(uiBox->firstChild);
        CalculateUiDownwardsDependentSizes(uiBox->next);

        bool isHorizontal = IsFlag(uiBox, UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT);

        for (int j = 0;
             j < ArrayCount(uiBox->uiSettings.uiSizes);
             j++)
        {
            UiSize uiSize = uiBox->uiSettings.uiSizes[j];
            switch (uiSize.kind)
            {
            case UI_SIZE_KIND_CHILDREN_OF_SUM:
            {
                float sumOrMaxOfChildren = 0;
                UiBox *child = uiBox->firstChild;
                while (child)
                {
                    Assert(child != uiBox);

                    ((j == 0 && isHorizontal) || (j == 1 && !isHorizontal))
                        ? sumOrMaxOfChildren += child->rect.dim.elements[j]
                        : sumOrMaxOfChildren = Max(child->rect.dim.elements[j], sumOrMaxOfChildren);

                    child = child->next;
                }
                uiBox->rect.dim.elements[j] = sumOrMaxOfChildren;

                if (j == ArrayCount(uiBox->uiSettings.uiSizes) - 1)
                    CalculateUiUpwardsDependentSizes(uiBox->firstChild);
                break;
            }
            case UI_SIZE_KIND_PERCENT_OF_PARENT:
            case UI_SIZE_KIND_PIXELS:
            case UI_SIZE_KIND_TEXT_NO_WRAPPING:
                break;
                InvalidDefaultCase
            }
        }
    }
}

void CalculateUiRelativePositions(UiBox *uiBox)
{
    if (uiBox)
    {
        if (!uiBox->parent || IsFlag(uiBox->parent, UI_FLAG_CHILDREN_MANUAL_LAYOUT))
        {
            uiBox->computedRelativePixelPos = uiBox->uiInputs.pixelPosition;
        }
        else if (uiBox->prev)
        {
            uiBox->computedRelativePixelPos = uiBox->prev->computedRelativePixelPos;

            if (uiBox->parent && IsFlag(uiBox->parent, UI_FLAG_CHILDREN_HORIZONTAL_LAYOUT))
                uiBox->computedRelativePixelPos.x += uiBox->prev->rect.dim.x;
            else
                uiBox->computedRelativePixelPos.y += uiBox->prev->rect.dim.y;
        }

        CalculateUiRelativePositions(uiBox->firstChild);
        CalculateUiRelativePositions(uiBox->next);
    }
}

void CalculateUiPosGivenReletativePositions(UiBox *uiBox)
{
    if (uiBox)
    {
        if (uiBox->parent)
            uiBox->computedRelativePixelPos += uiBox->parent->computedRelativePixelPos;

        uiBox->rect.pos = uiBox->computedRelativePixelPos;

        CalculateUiPosGivenReletativePositions(uiBox->firstChild);
        CalculateUiPosGivenReletativePositions(uiBox->next);
    }
}

UiBox GetValidUiBoxOfIndexLastFrame(unsigned int index)
{
    UiBox result = {};

    if (index)
    {
        int uiBoxArrayIndexLastFrame = GetFrameModIndexLastFrame();
        if (index < ArrayCount(G_UI_STATE->uiBoxes[uiBoxArrayIndexLastFrame]))
        {
            UiBox uiBox = G_UI_STATE->uiBoxes[uiBoxArrayIndexLastFrame][index];
            if (uiBox.frameRendered == G_CURRENT_FRAME - 1)
                result = uiBox;
        }
    }

    return result;
}

Color GetReactiveColor(UiBox *uiBoxLastFrame, ReactiveUiColor reactiveUiColor, bool disabled)
{
    Color result = reactiveUiColor.neutral;

    if (disabled)
        result = reactiveUiColor.disabled;
    else if (uiBoxLastFrame)
    {
        if (uiBoxLastFrame->down)
            result = reactiveUiColor.down;
        else if (uiBoxLastFrame->hovered)
            result = reactiveUiColor.hovered;
    }

    return result;
}

Color GetReactiveColorWithState(UiBox *uiBoxLastFrame, ReactiveUiColorState reactiveUiColorState, bool disabled, bool active)
{
    ReactiveUiColor reactiveUiColor = (active)
                                          ? reactiveUiColorState.active
                                          : reactiveUiColorState.nonActive;
    Color result = GetReactiveColor(uiBoxLastFrame, reactiveUiColor, disabled);
    return result;
}

String GetUiBoxKeyStringOfString(String string)
{
    String result = {};

    StringArray stringArray = SplitStringOnceByTag(string, G_UI_HASH_TAG_STRING);

    if (stringArray.count > 1)
        result = stringArray.strings[1];

    return result;
}

UiBox *GetUiBoxOfStringKeyLastFrame(String stringKey)
{
    UiBox *result = {};

    if (stringKey.length)
    {
        unsigned int hashvalue = Murmur3String(stringKey.chars, 0);

        for (int i = 0;
             i < stringKey.length;
             i++)
        {
            unsigned int index = (hashvalue + i) % ArrayCount(G_UI_STATE->uiHashEntries);
            UiHashEntry *uiHashEntry = &G_UI_STATE->uiHashEntries[index];
            if (uiHashEntry->uiBox && uiHashEntry->keyString == stringKey && uiHashEntry->uiBox->frameRendered == G_CURRENT_FRAME - 1)
            {
                result = uiHashEntry->uiBox;
                break;
            }
        }
    }

    return result;
}

UiBox *GetUiBoxOfStringLastFrame(String string)
{
    String keyString = GetUiBoxKeyStringOfString(string);
    UiBox *result = GetUiBoxOfStringKeyLastFrame(keyString);
    return result;
}

String CreateScriptStringKey(unsigned int scriptLineNumber)
{
    String result = CreateString("scriptEditor_line") + scriptLineNumber;
    return result;
}

//TODO: this could be a hash
UiBox *GetUiBoxLastFrameOfStringKey(String stringKey)
{
    UiBox *result = {};

    int uiBoxArrayIndex = GetFrameModIndexLastFrame();
    for (int uiBoxIndex = 0;
         uiBoxIndex < G_UI_STATE->uiBoxCount;
         uiBoxIndex++)
    {
        UiBox *uiBox = &G_UI_STATE->uiBoxes[uiBoxArrayIndex][uiBoxIndex];
        if (uiBox->keyString == stringKey)
        {
            result = uiBox;
            break;
        }
    }

    return result;
}

void RenderUiEntries(GameState *gameState, UiBox *uiBox, int uiDepth = 0)
{
    if (uiBox)
    {
        V2 windowPixelDim = gameState->windowDim;

        int drawOrder = 0;

        if (IsFlag(uiBox, UI_FLAG_DRAW_BACKGROUND) && uiBox->uiSettings.backColor.a)
        {
            uiBox->rect.dim.x = Clamp(0, uiBox->rect.dim.x, windowPixelDim.x);
            uiBox->rect.dim.y = Clamp(0, uiBox->rect.dim.y, windowPixelDim.y);
            Rectangle rect = RectToRayRectangle(uiBox->rect);
            DrawRectangleRec(rect, uiBox->uiSettings.backColor);
        }

        if (IsFlag(uiBox, UI_FLAG_DRAW_TEXT))
        {
            Vector2 pos = V2ToRayVector(uiBox->rect.pos);
            DrawTextPro(uiBox->uiSettings.font, uiBox->string.chars, pos, {}, 0, uiBox->uiSettings.fontSize, 1, uiBox->uiSettings.frontColor);
        }

        if (IsFlag(uiBox, UI_FLAG_DRAW_TEXTURE))
        {
            Assert(uiBox->uiInputs.texture.id);
            Vector2 pos = V2ToRayVector(uiBox->rect.pos);
            DrawTextureEx(uiBox->uiInputs.texture, pos, 0, 1, WHITE);
        }

        if (IsFlag(uiBox, UI_FLAG_DRAW_BORDER))
        {
            Rect rect = uiBox->rect;
            DrawRectangleLines(rect.pos.x, rect.pos.y, rect.dim.x, rect.dim.y, uiBox->uiSettings.borderColor);
        }

        RenderUiEntries(gameState, uiBox->firstChild, uiDepth + 2);
        RenderUiEntries(gameState, uiBox->next, uiDepth);
    }
}
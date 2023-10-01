#pragma once

#include <stdio.h>
#include "math.h"
#include <stdint.h>

#if __clang__
#include "vn_intrinsics.h"
#endif

// must be set ahead of time
// Requires MemoryArena
static MemoryArena *G_STRING_TEMP_MEM_ARENA;

struct String
{
    char *chars;
    unsigned int length;
};

struct StringArray
{
    String *strings;
    unsigned int count;
};

//-----Char functions------

inline bool CharIsAlphaUpper(char c)
{
    return c >= 'A' && c <= 'Z';
}

inline bool CharIsAlphaLower(char c)
{
    return c >= 'a' && c <= 'z';
}

inline bool CharIsAlpha(char c)
{
    return CharIsAlphaUpper(c) || CharIsAlphaLower(c);
}

inline bool CharIsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

inline bool CharIsVaildNumberStart(char c)
{
    return c == '.' || c == '+' || c == '-' || CharIsDigit(c);
}

inline bool CharIsSymbol(char c)
{
    return (c == '~' || c == '!' || c == '$' || c == '%' || c == '^' ||
            c == '&' || c == '*' || c == '-' || c == '=' || c == '+' ||
            c == '<' || c == '.' || c == '>' || c == '/' || c == '?' ||
            c == '|' || c == '\\' || c == '{' || c == '}' || c == '(' ||
            c == ')' || c == '\\' || c == '[' || c == ']' || c == '#' ||
            c == ',' || c == ';' || c == ':' || c == '@');
}

inline bool CharIsLeftBracket(char c)
{
    return c == '{';
}

inline bool CharIsRightBracket(char c)
{
    return c == '}';
}

inline bool CharIsSpace(char c)
{
    return c == ' ' || c == '\r' || c == '\t' || c == '\f' || c == '\v';
}

inline bool CharIsNewline(char c)
{
    return c == '\n';
}

inline bool CharIsEnd(char c)
{
    return c == '\0';
}

inline bool CharIsSpaceEndOrNewline(char c)
{
    return CharIsSpace(c) || CharIsEnd(c) || CharIsNewline(c);
}

inline char CharToUpper(char c)
{
    return (c >= 'a' && c <= 'z') ? ('A' + (c - 'a')) : c;
}

inline char CharToLower(char c)
{
    return (c >= 'A' && c <= 'Z') ? ('a' + (c - 'A')) : c;
}

inline char CharToForwardSlash(char c)
{
    return (c == '\\' ? '/' : c);
}

inline unsigned int CharArrayLength(char *chars)
{
    unsigned int length = 0;

    if (chars != 0)
        while (*chars++)
            length++;

    return length;
}

inline unsigned int NullTerminatedCharLength(const char *chars)
{
    unsigned int length = 0;

    if (chars != 0)
        while (*chars++)
            length++;

    return length;
}

//-----String functions------

inline String AllocateString(unsigned int length, MemoryArena *arena)
{
    String result;

    if (length)
    {
        result.length = length;
        result.chars = PushArray(arena, length + 1, char);
        result.chars[length] = '\0';
    }
    else
    {
        result = {};
    }

    return result;
}

inline String CreateStringOfLength(const char *sourceChars, unsigned int length, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    Assert(length);
    String result = AllocateString(length, arena);
    memcpy(result.chars, sourceChars, length);
    return result;
}

inline String CreateStringOnArena(const char *sourceChars, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA, int startIndex = 0, int endIndex = 0)
{
    String result = {};
    Assert(!endIndex || (startIndex <= endIndex));

    if (!endIndex || startIndex <= endIndex)
    {
        int sizeSource = NullTerminatedCharLength(sourceChars);
        int length = (endIndex) ? endIndex - startIndex : sizeSource;
        Assert(length <= sizeSource);

        if (length)
            result = CreateStringOfLength(sourceChars + startIndex, length, arena);
    }

    return result;
}

inline String CreateStringOnArena(char *sourceChars, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA, int startIndex = 0, int endIndex = 0)
{
    return CreateStringOnArena((const char *)sourceChars, arena, startIndex, endIndex);
}

inline String CreateString(char *chars, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    return CreateStringOnArena(chars, arena);
}

inline String CreateString(const char *chars, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    return CreateStringOnArena(chars, arena);
}

inline String CreateString(char c, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    String result = AllocateString(1, arena);
    result.chars[0] = c;
    return result;
}

inline String CreateString(String s)
{
    return s;
}

inline void MoveStringToArena(String *string, MemoryArena *arena)
{
    *string = CreateStringOnArena(string->chars, arena);
}

inline bool StringCompare(String s1, String s2)
{
    bool result = s1.length == s2.length;
    if (result)
    {
        for (int i = 0;
             i < s1.length;
             i++)
        {
            if (s1.chars[i] != s2.chars[i])
            {
                result = false;
                break;
            }
        }
    }
    return result;
}

inline void ValidateEditedStringLength(String string)
{
    int nullTerminatedLength = NullTerminatedCharLength(string.chars);
    Assert(nullTerminatedLength == string.length);
}

inline String InsertString(String stringToEdit, String stringToInsert, unsigned int index, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    Assert(index <= stringToEdit.length);

    String result = AllocateString(stringToEdit.length + stringToInsert.length, arena);
    memcpy(result.chars, stringToEdit.chars, index);
    memcpy(result.chars + index, stringToInsert.chars, stringToInsert.length);
    memcpy(result.chars + index + stringToInsert.length, stringToEdit.chars + index, stringToEdit.length - index);

    ValidateEditedStringLength(result);

    return result;
}

inline String RemoveRangeInString(String stringToEdit, unsigned int startIndex, unsigned int endIndex, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    Assert(startIndex <= stringToEdit.length);
    Assert(endIndex <= stringToEdit.length);
    Assert(endIndex >= startIndex);

    unsigned int gapLength = (endIndex - startIndex);
    String result = AllocateString(stringToEdit.length - gapLength, arena);
    Assert(result.chars != stringToEdit.chars);

    memcpy(result.chars, stringToEdit.chars, startIndex);
    memcpy(result.chars + startIndex, stringToEdit.chars + endIndex, stringToEdit.length + gapLength);

    ValidateEditedStringLength(result);

    return result;
}

inline String RemoveIndexInString(String stringToEdit, unsigned int index, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    Assert(index <= stringToEdit.length);
    String result = RemoveRangeInString(stringToEdit, index, index + 1, arena);
    return result;
}

inline StringArray SplitByChar(String string, char c, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    StringArray result = {};
    int numSplitsPrecalculated = 1;
    for (int i = 0;
         i <= string.length;
         i++)
    {
        if (string.chars[i] == c)
            numSplitsPrecalculated++;
    }
    result.strings = PushArray(arena, numSplitsPrecalculated, String);
    int firstLetterOfSplitIndex = 0;

    for (int i = 0;
         i <= string.length;
         i++)
    {
        if (string.chars[i] == c || i == string.length)
        {
            String split = CreateStringOnArena(string.chars, arena, firstLetterOfSplitIndex, i);
            firstLetterOfSplitIndex = i + 1;
            result.strings[result.count] = split;
            result.count++;
        }
    }
    return result;
}

inline StringArray SplitStringOnceByTag(String string, String tag, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    StringArray result = {};

    if (string.length >= tag.length)
    {
        for (int i = 0;
             i < string.length - tag.length;
             i++)
        {
            for (int k = 0;
                 k < tag.length;
                 k++)
            {
                if (string.chars[i + k] != tag.chars[k])
                    break;

                if (k == tag.length - 1)
                {
                    result.count = 2;
                    result.strings = PushArray(arena, result.count, String);
                    if (i)
                        result.strings[0] = CreateStringOnArena(string.chars, arena, 0, i);
                    else
                        result.strings[0] = {};

                    if (string.length - 1 - i + tag.length > 0)
                        result.strings[1] = CreateStringOnArena(string.chars, arena, i + tag.length, string.length);
                    else
                        result.strings[1] = {};
                }
            }
        }
    }

    if (!result.count)
    {
        result.count = 1;
        result.strings = PushArray(arena, result.count, String);
        result.strings[0] = CreateStringOnArena(string.chars, arena);
    }

    return result;
}

inline StringArray SplitByWords(String string, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    return SplitByChar(string, ' ', arena);
}

inline int IndexOfChar(String string, char c, int startIndex = 0)
{
    int result = startIndex;

    Assert(startIndex <= string.length);

    for (;
         result < string.length;
         result++)
    {
        if (string.chars[result] == c)
            break;
    }
    return result;
}

inline String GetSlicedString(String string, int startIndex, int endIndex, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    String result = {};

    Assert(endIndex > startIndex);
    if (string.length)
    {
        result = AllocateString(endIndex - startIndex, arena);
        memcpy(result.chars, string.chars + startIndex, result.length);
    }

    ValidateEditedStringLength(result);

    return result;
}

inline String GetStringUpToChar(String string, int inputIndex, char tagEnd, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    String result = {};
    if (string.length)
    {
        int indexOfTagEnd = IndexOfChar(string, tagEnd, inputIndex);
        result = GetSlicedString(string, inputIndex, indexOfTagEnd, arena);
    }
    return result;
}

inline String CopyJoinedCharsIntoString(const char *c1, unsigned int chars1Length, const char *c2, unsigned int chars2Length, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    String result = AllocateString(chars1Length + chars2Length, arena);

    memcpy(result.chars, c1, chars1Length);
    memcpy(result.chars + chars1Length, c2, chars2Length);
    ValidateEditedStringLength(result);

    return result;
}

inline String JoinStrings(String s1, String s2, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    String result = {};

    if (!s2.length)
        result = s1;
    else if (!s1.length)
        result = s2;
    else
        result = CopyJoinedCharsIntoString(s1.chars, s1.length, s2.chars, s2.length, arena);

    return result;
}

inline String AddCharToString(String s, char c, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    String result = {};

    if (!c)
        result = s;
    else if (!s.length)
        result = CreateString(c, arena);
    else
        result = CopyJoinedCharsIntoString(s.chars, s.length, &c, 1, arena);

    return result;
}

inline String RemoveNumCharFromFront(String s, unsigned int n, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    String result = AllocateString(s.length - n, arena);

    if (result.length)
        memcpy(result.chars, s.chars, result.length);

    ValidateEditedStringLength(result);

    return result;
}

struct StartEndIndicies
{
    int start;
    int end;
};

inline StartEndIndicies GetStartEndIndiciesOfTrimmedWhitespace(String string)
{
    StartEndIndicies result = {};
    result.end = string.length - 1;

    while (result.start < string.length && CharIsSpaceEndOrNewline(string.chars[result.start]))
        result.start++;
    while (result.end > 0 && CharIsSpaceEndOrNewline(string.chars[result.end]))
        result.end--;

    return result;
}

inline void TrimWhitespace(String *string)
{
    StartEndIndicies startEndIndicies = GetStartEndIndiciesOfTrimmedWhitespace(*string);

    // TODO: I'm fairly sure we're not doing anything fishy here with the indexing,
    // but maybe double check with a test sometime
    if (startEndIndicies.start > 0)
    {
        string->chars = &string->chars[startEndIndicies.start];
        string->length = startEndIndicies.end - startEndIndicies.start + 1;
    }
    if (startEndIndicies.end < string->length - 1)
    {
        string->chars[startEndIndicies.end + 1] = '\0';
        string->length = startEndIndicies.end - startEndIndicies.start + 1;
    }
}

inline bool StringIsInt(String s)
{
    bool result = false;

    if (s.length)
    {
        StartEndIndicies startEndIndicies = GetStartEndIndiciesOfTrimmedWhitespace(s);

        int firstNonWhiteSpaceIndex = startEndIndicies.start;
        if (startEndIndicies.start <= startEndIndicies.end &&
            !(startEndIndicies.start == startEndIndicies.end && !(CharIsDigit(s.chars[startEndIndicies.start]))) &&
            CharIsVaildNumberStart(s.chars[startEndIndicies.start]))
        {
            for (;
                 startEndIndicies.start <= startEndIndicies.end;
                 startEndIndicies.start++)
            {
                if ((startEndIndicies.start != firstNonWhiteSpaceIndex) && !CharIsDigit(s.chars[startEndIndicies.start]))
                    break;

                if (startEndIndicies.start == startEndIndicies.end)
                    result = true;
            }
        }
    }
    return result;
}

// Refactored from: https://www.geeksforgeeks.org/check-given-string-valid-number-integer-floating-point/
inline bool StringIsFloat(String s)
{
    bool result = false;
    if (s.length)
    {
        StartEndIndicies startEndIndicies = GetStartEndIndiciesOfTrimmedWhitespace(s);

        int firstNonWhiteSpaceIndex = startEndIndicies.start;
        if (startEndIndicies.start <= startEndIndicies.end &&
            !(startEndIndicies.start == startEndIndicies.end && !(CharIsDigit(s.chars[startEndIndicies.start]))) &&
            CharIsVaildNumberStart(s.chars[startEndIndicies.start]))
        {
            bool flagDot = false;
            for (;
                 startEndIndicies.start <= startEndIndicies.end;
                 startEndIndicies.start++)
            {
                if (s.chars[startEndIndicies.start] == '.')
                {
                    if (flagDot || (startEndIndicies.start + 1 > s.length) || !(CharIsDigit(s.chars[startEndIndicies.start + 1])))
                        break;
                    flagDot = true;
                }
                else if ((startEndIndicies.start != firstNonWhiteSpaceIndex) && !CharIsDigit(s.chars[startEndIndicies.start]))
                    break;

                if (startEndIndicies.start == startEndIndicies.end)
                    result = true;
            }
        }
    }

    return result;
}

inline int StringToInt(String string)
{
    int result = 0;
    if (string.length)
    {
        Assert(StringIsInt(string));
        result = atoi(string.chars);
    }
    return result;
}

inline String IntToString(int i, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    //TODO: figure out how large this should be
    String result = AllocateString(10, arena);
    sprintf(result.chars, "%d", i);
    result.length = CharArrayLength(result.chars);
    return result;
}

inline String FloatToString(float f, MemoryArena *arena = G_STRING_TEMP_MEM_ARENA)
{
    //TODO: figure out how large this should be
    String result = AllocateString(10, arena);
    sprintf(result.chars, "%.2f", f);
    result.length = CharArrayLength(result.chars);

    return result;
}

inline float StringToFloat(String string)
{
    //TODO: rewrite so this doesn't crash on fail
    float number = atof(string.chars);
    return number;
}

inline void ToLowercase(String string)
{
    for (int i = 0;
         i < string.length;
         i++)
    {
        string.chars[i] = CharToLower(string.chars[i]);
    }
}

inline void ToUppercase(String string)
{
    for (int i = 0;
         i < string.length;
         i++)
    {
        string.chars[i] = CharToUpper(string.chars[i]);
    }
}

inline String operator+(String s1, String s2)
{
    return JoinStrings(s1, s2);
}

inline String operator+(String s, char *chars)
{
    String stringOfChars = CreateString(chars);
    return JoinStrings(s, stringOfChars);
}

inline String operator+(char *chars, String s)
{
    String stringOfChars = CreateString(chars);
    return JoinStrings(stringOfChars, s);
}

inline String operator+(String s, const char *chars)
{
    String stringOfChars = CreateString(chars);
    return JoinStrings(s, stringOfChars);
}

inline String operator+(const char *chars, String s)
{
    String stringOfChars = CreateString(chars);
    return JoinStrings(stringOfChars, s);
}

inline String operator+(String s, int i)
{
    return s + IntToString(i);
}

inline String operator+(String s, unsigned int i)
{
    return s + IntToString(i);
}

inline String operator+(String s, float f)
{
    return s + FloatToString(f);
}

// String operator+(String s, char c)
// {
//     String stringOfChar = CreateStringUsingChars(&c);
//     return JoinStrings(s, stringOfChar);
// }

// String operator+(char c, String s)
// {
//     String stringOfChar = CreateStringUsingChars(&c);
//     return JoinStrings(stringOfChar, s);
// }

inline String &operator+=(String &left, String right)
{
    return (left = left + right);
}

inline String &operator+=(String &left, const char *right)
{
    unsigned int rightLength = NullTerminatedCharLength(right);
    return left = CopyJoinedCharsIntoString(left.chars, left.length, right, rightLength);
}

inline String &operator+=(String &left, char *right)
{
    unsigned int rightLength = CharArrayLength(right);
    return left = CopyJoinedCharsIntoString(left.chars, left.length, right, rightLength);
}

inline String &operator+=(String &left, char right)
{
    return left = AddCharToString(left, right);
}

inline String &operator+=(String &left, float right)
{
    return (left = left + right);
}

inline bool operator==(String s1, String s2)
{
    return StringCompare(s1, s2);
}

// TODO: Surely we don't need to copy a const string for comparison do we
inline bool operator==(const char *c, String s)
{
    String tempString = CreateString(c);
    return StringCompare(tempString, s);
}

inline bool operator==(char *c, String s)
{
    String tempString = CreateString(c);
    return StringCompare(tempString, s);
}

inline bool operator==(String s, char *c)
{
    String tempString = CreateString(c);
    return StringCompare(tempString, s);
}

inline bool operator==(String s, const char *c)
{
    String tempString = CreateString(c);
    return StringCompare(tempString, s);
}

inline bool operator!=(const char *c, String s)
{
    return !(c == s);
}

inline bool operator!=(char *c, String s)
{
    return !(c == s);
}

inline bool operator!=(String s, char *c)
{
    return !(c == s);
}

inline bool operator!=(String s, const char *c)
{
    return !(c == s);
}

//NOTE: NO MORE PRINTING FOR YOU BUSTER
//(Unless you uncomment this)
#if 0
inline void Print(String s)
{
    printf("%s\n", s.chars);
}

inline void Print(char c)
{
    printf("%c\n", c);
}

inline void Print(char *chars)
{
    printf("%s\n", chars);
}

inline void Print(const char *chars)
{
    printf("%s\n", chars);
}

inline void Print(long l)
{
    printf("%ld\n", l);
}

inline void Print(double d)
{
    printf("%f\n", d);
}

inline void Print(int i)
{
    printf("%i\n", i);
}

inline void Print(unsigned int i)
{
    printf("%i\n", i);
}

inline void Print(float f)
{
    printf("%f\n", f);
}

inline void Print(bool b)
{
    String result = (b)
                        ? CreateString("") + CONSOLE_GREEN + "TRUE" + CONSOLE_DEFAULT
                        : CreateString("") + CONSOLE_RED + "FALSE" + CONSOLE_DEFAULT;
    printf("%s\n", result.chars);
}

inline void Print(long long unsigned int i)
{
    printf("%llu", i);
}
#endif

inline uintptr_t ConvertString_(String string, const char **stringArray, int sizeOfStringArray)
{
    int result = 0;

    if (string.chars)
    {
        for (int i = 0;
             i < sizeOfStringArray;
             i++)
        {
            if (stringArray[i] == string)
            {
                result = i;
                break;
            }
        }
    }

    return (uintptr_t)result;
}

inline unsigned int LevenshteinDistance(String s1, String s2)
{
    unsigned int m = s1.length;
    unsigned int n = s2.length;

    unsigned int **dp = PushArray(G_STRING_TEMP_MEM_ARENA, m + 1, unsigned int *);
    for (unsigned int i = 0; i <= m; ++i)
        dp[i] = PushArray(G_STRING_TEMP_MEM_ARENA, n + 1, unsigned int);

    for (int i = 0;
         i <= m;
         ++i)
    {
        dp[i][0] = i;
    }

    for (int j = 0;
         j <= n;
         ++j)
    {
        dp[0][j] = j;
    }

    for (int i = 1;
         i <= m;
         ++i)
    {
        for (int j = 1;
             j <= n;
             ++j)
        {
            if (s1.chars[i - 1] == s2.chars[j - 1])
                dp[i][j] = dp[i - 1][j - 1];
            else
            {
                unsigned int substitution = dp[i - 1][j - 1];
                unsigned int deletion = dp[i - 1][j];
                unsigned int insertion = dp[i][j - 1];

                if (substitution <= deletion && substitution <= insertion)
                    dp[i][j] = substitution + 1;
                else if (deletion <= substitution && deletion <= insertion)
                    dp[i][j] = deletion + 1;
                else
                    dp[i][j] = insertion + 1;
            }
        }
    }

    unsigned result = dp[m][n];

    return result;
}

inline String CleanStringForDiscord(String string)
{
    String result = {};

    result.chars = PushArray(G_STRING_TEMP_MEM_ARENA, string.length * 2 + 1, char);

    for (int i = 0;
         i < string.length;
         i++, result.length++)
    {
        char c = string.chars[i];
        switch (c)
        {
        case '\"':
        case '\\':
            result.chars[result.length++] = '\\';
            result.chars[result.length] = c;
            break;
        case '\r':
        case '\n':
            result.chars[result.length] = ' ';
            break;
        default:
            result.chars[result.length] = c;
            break;
        }
    }

    return result;
}
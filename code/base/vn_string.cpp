
#include <base/macros.h>
#include <base/memory.h>
#include <base/vn_string.h>

#include <stdio.h>
#include "math.h"
#include <stdint.h>
#include <cstring>  //memcpy
#include <stdlib.h> //strtod

//-----Char functions------

bool CharIsAlphaUpper(char c)
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

bool CharIsSpace(char c)
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

bool CharIsSpaceEndOrNewline(char c)
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

unsigned int NullTerminatedCharLength(const char *chars)
{
	unsigned int length = 0;

	if (chars != 0)
		while (*chars++)
			length++;

	return length;
}

//-----String functions------

Arena *StringArena()
{
	static Arena result = {};
	static b32 init = false;
	if (!init)
	{
		init = true;
		result = ArenaInit(STRING_ARENA_SIZE);
		result.flags |= ARENA_FLAG_CIRCULAR;
	}
	return &result;
}

String AllocateString(u32 length, Arena *arena)
{
	String result;

	if (length)
	{
		result.length = length;
		result.chars = (char*) ArenaPushSize(arena, length * sizeof(char), {});
	}
	else
	{
		result = {};
	}

	return result;
}

String ReallocString(String s, Arena *arena)
{
	String result = {};
	if (s.length > 0)
	{
		result = AllocateString(s.length, arena);
		memcpy(result.chars, s.chars, s.length);
	}
	return result;
}

String CreateString(const char *stringLiteral, Arena *arena)
{
	String result = {};
	int length = NullTerminatedCharLength(stringLiteral);
	if (length > 0)
	{
		result = AllocateString(length , arena);
		memcpy(result.chars, stringLiteral, length);
	}
	return result;
}

String CreateSubString(String s, u32 startIndex, u32 endIndex)
{
	String result = {};
	if (ASSERT(startIndex <= endIndex) &&
			ASSERT(startIndex < s.length) &&
			ASSERT(endIndex < s.length))
	{
		result.chars = s.chars + startIndex;
		result.length = (endIndex - startIndex) + 1;
	}
	return result;
}

String JoinStrings(String s1, String s2, Arena *arena)
{
	String result = {};

	if (ASSERT(s1.chars && s2.chars))
	{
		if (!s2.length)
		{
			result = s1;
		}
		else if (!s1.length)
		{
			result = s2;
		}
		else
		{
			result = AllocateString(s1.length + s2.length, arena);
			memcpy(result.chars, s1.chars, s1.length);
			memcpy(result.chars + s1.length, s2.chars, s2.length);
		}
	}

	return result;
}

String StringAppendChar(String s, char c, Arena *arena)
{
	String result = {};

	result = AllocateString(s.length + 1, arena);
	memcpy(result.chars, s.chars, s.length);
	memcpy(result.chars + s.length, &c, 1);

	return result;
}

b32 StringCompare(String s1, String s2)
{
	bool result = s1.length == s2.length;
	if (result)
	{
		for (u32 i = 0; i < s1.length; i++)
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

i32 StringGetCharIndex(String s, char c, u32 startIndex)
{
	i32 result = -1;
	ASSERT(startIndex < s.length);
	for (u32 i = startIndex; i < s.length; i++)
	{
		if (s.chars[i] == c)
		{
			result = i;
			break;
		}
	}
	return result;
}

String TrimWhitespace(String string)
{
	String result = string;
	u32 start = 0;
	u32 end = string.length - 1;
	while (start < string.length && CharIsSpaceEndOrNewline(string.chars[start]))
	{
		start++;
	}
	while (end > 0 && CharIsSpaceEndOrNewline(string.chars[end]))
	{
		end--;
	}
	result = CreateSubString(string, start, end);
	return result;
}

StringArray SplitStringOnceByTag(String string, String tag, Arena *arena)
{
	StringArray result = {};

	if (string.length >= tag.length)
	{
		for (u32 i = 0; i < string.length - tag.length; i++)
		{
			for (u32 k = 0; k < tag.length; k++)
			{
				if (string.chars[i + k] != tag.chars[k])
					break;

				if (k == tag.length - 1)
				{
					result.count = 2;
					result.strings = ARENA_PUSH_ARRAY(arena, result.count, String);
					if (i)
					{
						result.strings[0] = CreateSubString(string, 0, i - 1);
					}
					else
					{
						result.strings[0] = {};
					}

					if (string.length - 1 - i + tag.length > 0)
					{
						result.strings[1] = CreateSubString(string, i + tag.length, string.length - 1);
					}
					else
					{
						result.strings[1] = {};
					}
				}
			}
		}
	}

	if (!result.count)
	{
		result.count = 1;
		result.strings = ARENA_PUSH_ARRAY(arena, result.count, String);
		result.strings[0] = string;
	}

	return result;
}

b32 StringIsF32(String s)
{
	b32 result = false;
	if (s.length)
	{
		String trimmedString = TrimWhitespace(s);
		if (trimmedString.length && CharIsVaildNumberStart(trimmedString.chars[0]))
		{
			b32 flagDot = false;
			for (u32 i = 0; i <= trimmedString.length; i++)
			{
				if (s.chars[i] == '.')
				{
					if (flagDot || (i + 1 > s.length) || !(CharIsDigit(s.chars[i + 1])))
					{
						break;
					}
					flagDot = true;
				}
				else if ((i > 0) && !CharIsDigit(s.chars[i]))
				{
					break;
				}

				if (i == trimmedString.length - 1)
				{
					result = true;
				}
			}
		}
	}

	return result;
}

f32 StringToF32(String s)
{
	char *end = s.chars + s.length;
	f32 number = (f32) strtod(s.chars, &end);
	return number;
}

String U32ToString(u32 i, Arena *arena)
{
	//TODO: figure out how large this should be
	String result = AllocateString(30, arena);
	//TODO: (Ahmayk) should probably replace this
	sprintf_s(result.chars, 30, "%d", i);
	result.length = CharArrayLength(result.chars);
	return result;
}

String U64ToString(u64 n, Arena *arena)
{
	//TODO: figure out how large this should be
	String result = AllocateString(30, arena);
	//TODO: (Ahmayk) should probably replace this
	sprintf_s(result.chars, 30, "%lld", n);
	result.length = CharArrayLength(result.chars);
	return result;
}

void Print(String s)
{
	printf("%.*s\n", s.length, s.chars);
}

void Print(char c)
{
	printf("%c\n", c);
}

void Print(char *chars)
{
	printf("%s\n", chars);
}

void Print(const char *chars)
{
	printf("%s\n", chars);
}

void Print(long l)
{
	printf("%ld\n", l);
}

void Print(double d)
{
	printf("%f\n", d);
}

void Print(int i)
{
	printf("%i\n", i);
}

void Print(unsigned int i)
{
	printf("%i\n", i);
}

void Print(float f)
{
	printf("%f\n", f);
}

void Print(long long unsigned int i)
{
	printf("%llu", i);
}

String CleanStringForDiscord(String string, Arena *arena)
{
	String result = {};

	result.chars = ARENA_PUSH_ARRAY(arena, string.length * 2 + 1, char);

	for (u32 i = 0; i < string.length; i++, result.length++)
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

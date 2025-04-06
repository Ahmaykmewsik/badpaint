#pragma once

#include <base/macros.h>
#include <base/memory.h>

#define STRING_ARENA_SIZE (MegaByte * 1)

struct String
{
    char *chars;
    u32 length;
};

struct StringArray
{
    String *strings;
    u32 count;
};

bool CharIsSpace(char c);
bool CharIsSpaceEndOrNewline(char c);

Arena *StringArena();
u32 NullTerminatedCharLength(const char *chars);
String AllocateString(u32 length, Arena *arena);
String CreateString(const char *stringLiteral, Arena *arena);
String CreateSubString(String s, u32 startIndex, u32 endIndex);
String ReallocString(String s, Arena *arena);
String JoinStrings(String s1, String s2, Arena *arena);
String StringAppendChar(String s, char c, Arena *arena);
i32 StringGetCharIndex(String s, char c, u32 startIndex); //NOTE: (Ahmayk) Returns -1 if not found
b32 StringCompare(String s1, String s2);
StringArray SplitStringOnceByTag(String string, String tag, Arena *arena);

b32 StringIsF32(String s);
f32 StringToF32(String s);
String U32ToString(u32 n, Arena *arena);
String U64ToString(u64 n, Arena *arena);

#define STRING_CHAR(c) String{(char*)c, 1}
#define STRING(chars) String{(char*)chars, NullTerminatedCharLength(chars)}
#define C_STRING_NULL_TERMINATED(string) StringAppendChar(string, '\0', StringArena()).chars

void Print(String s);
void Print(char c);
void Print(char *chars);
void Print(const char *chars);
void Print(long l);
void Print(double d);
void Print(int i);
void Print(unsigned int i);
void Print(float f);
void Print(long long unsigned int i);

inline String operator+(String s1, String s2)
{
    return JoinStrings(s1, s2, StringArena());
}

inline String operator+(String s, const char *chars)
{
    return JoinStrings(s, STRING(chars), StringArena());
}

inline String operator+(const char *chars, String s)
{
    return JoinStrings(STRING(chars), s, StringArena());
}

inline String &operator+=(String &left, String right)
{
    return (left = left + right);
}

inline String &operator+=(String &left, const char *right)
{
    return left = left + STRING(right); 
}

inline b32 operator==(String s1, String s2)
{
    return StringCompare(s1, s2);
}

inline b32 operator==(String s, const char *chars)
{
    return StringCompare(s, STRING(chars));
}

String CleanStringForDiscord(String string, Arena *arena);

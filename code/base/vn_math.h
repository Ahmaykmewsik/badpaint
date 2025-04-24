#pragma once

#include <base/macros.h>
#include <xmmintrin.h>

#define PI 3.14159265358979323846f
#define WINDOWS_FILETIME_TO_UNIXTIME(ft) \
	(UINT)((*(LONGLONG *)&(ft)-116444736000000000) / 10000000)

union v2
{
	struct
	{
		f32 x, y;
	};

	f32 elements[2];
};

v2 operator+(v2 a, v2 b);
v2 operator-(v2 a, v2 b);
v2 operator*(v2 a, v2 b);
v2 operator/(v2 a, v2 b);
void operator+=(v2 &a, v2 b);
void operator-=(v2 &a, v2 b);
void operator*=(v2 &a, v2 b);
void operator/=(v2 &a, v2 b);

v2 operator+(v2 a, f32 b);
v2 operator-(v2 a, f32 b);
v2 operator*(v2 a, f32 b);
v2 operator/(v2 a, f32 b);
void operator+=(v2 &a, f32 b);
void operator-=(v2 &a, f32 b);
void operator*=(v2 &a, f32 b);
void operator/=(v2 &a, f32 b);

v2 operator*(v2 a, f64 b);

union iv2
{
	struct
	{
		i32 x, y;
	};

	i32 elements[2];

    operator v2()
	{
        return v2{(f32)x, (f32)y};
    }
};

iv2 operator+(iv2 a, iv2 b);
iv2 operator-(iv2 a, iv2 b);
iv2 operator*(iv2 a, iv2 b);
iv2 operator/(iv2 a, iv2 b);
void operator+=(iv2 &a, iv2 b);
void operator-=(iv2 &a, iv2 b);
void operator*=(iv2 &a, iv2 b);
void operator/=(iv2 &a, iv2 b);
b32 operator==(iv2 a, iv2 b);
b32 operator!=(iv2 a, iv2 b);

v2 operator+(iv2 a, v2 b);
v2 operator-(iv2 a, v2 b);
v2 operator*(iv2 a, v2 b);
v2 operator/(iv2 a, v2 b);

v2 operator+(v2 a, iv2 b);
v2 operator-(v2 a, iv2 b);
v2 operator*(v2 a, iv2 b);
v2 operator/(v2 a, iv2 b);
void operator+=(v2 &a, iv2 b);
void operator-=(v2 &a, iv2 b);
void operator*=(v2 &a, iv2 b);
void operator/=(v2 &a, iv2 b);

iv2 operator+(iv2 a, i32 b);
iv2 operator-(iv2 a, i32 b);
iv2 operator*(iv2 a, i32 b);
iv2 operator/(iv2 a, i32 b);
void operator+=(iv2 &a, i32 b);
void operator-=(iv2 &a, i32 b);
void operator*=(iv2 &a, i32 b);
void operator/=(iv2 &a, i32 b);

v2 operator+(iv2 a, f32 b);
v2 operator-(iv2 a, f32 b);
v2 operator*(iv2 a, f32 b);
v2 operator/(iv2 a, f32 b);

union v3
{
	struct
	{
		f32 x, y, z;
	};

	f32 elements[3];
};

v3 operator+(v3 a, v3 b);
v3 operator-(v3 a, v3 b);
void operator+=(v3 &a, v3 b);
void operator-=(v3 &a, v3 b);

v3 operator*(v3 a, f32 b);
v3 operator*(f32 a, v3 b); //NOTE: (Marc) idk about this one
void operator*=(v3 &a, f32 b);

union v4
{
	struct
	{
		union
		{
			v3 xyz;
			struct
			{
				f32 x, y, z;
			};
		};

		f32 w;
	};

	struct
	{
		union
		{
			v3 rgb;
			struct
			{
				f32 r, g, b;
			};
		};

		f32 a;
	};

	__m128 sse;
};

v4 operator*(v4 a, f32 b);

union ColorU32
{
	struct
	{
		u8 r;
		u8 g;
		u8 b;
		u8 a;
	};

	struct
	{
		u32 color;
	};
};

#define COLORU32_BLACK     ColorU32{0, 0, 0, 255}
#define COLORU32_GRAY      ColorU32{130, 130, 130, 255}
#define COLORU32_DARKGRAY  ColorU32{80, 80, 80, 255}
#define COLORU32_RED       ColorU32{230, 41, 55, 255}
#define COLORU32_BLUE      ColorU32{0, 0, 255, 255}

struct LineV2
{
	v2 start;
	v2 end;
};

struct LineV3
{
	v3 start;
	v3 end;
};

struct RectV2
{
	v2 pos;
	v2 dim;
};

struct RectIV2
{
	iv2 pos;
	iv2 dim;
};

struct V2Verticies
{
	v2 verticies[4];
};

struct V3Verticies
{
	v3 verticies[4];
};

struct Angle
{
	f32 theta;
	f32 perpendicularTest;
};

v2 V3ToV2(v3 v);
v3 V4ToV3(v4 v);
v4 V3ToV4_1(v3 v);
v3 V2ToV3_0(v2 v);
v4 V3ToV4_W(v3 v, f32 w);
v4 V3ToV4_1(v3 v);

b32 IsZeroV2(v2 v);
b32 IsZeroIV2(iv2 v);
b32 IsZeroV3(v3 v);
f32 SignF32(f32 f);
f32 RoundF32(f32 f);
i32 RoundI32(f32 f);
u64 RoundU64(f64 f);
v2 RoundV2(v2 v);
v3 RoundV3(v3 v);
f32 FloorF32(f32 f);
v2 FloorV2(v2 v);
f32 CeilF32(f32 f);
v2 CeilV2(v2 v);
f32 AbsF32(f32 f);
v2 AbsV2(v2 v);
v3 AbsV3(v3 v);
u32 MinU32(u32 i, u32 j);
i32 MinI32(i32 i, i32 j);
f32 MinF32(f32 i, f32 j);
u32 MaxU32(u32 i, u32 j);
i32 MaxI32(i32 i, i32 j);
f32 MaxF32(f32 i, f32 j);
i32 ClampI32(i32 min, i32 value, i32 max);
f32 ClampF32(f32 min, f32 value, f32 max);
f64 ClampF64(f64 min, f64 value, f64 max);
f32 DistanceV2(v2 v1, v2 v2);
f32 SafeDivideF32(f32 a, f32 b);
f64 SafeDivideF64(f64 a, f64 b);
f32 SafeDivideI32(i32 a, i32 b);
u32 ModNextU32(u32 i, u32 size);
u32 ModBackU32(u32 i, u32 size);
f32 SquareF32(f32 a);
f64 SquareF64(f64 a);

f32 LerpF32(f32 start, f32 amount, f32 end);
v2 LerpV2(v2 start, f32 amount, v2 end);
f32 InverseLerpF32(f32 start, f32 value, f32 end);
f64 InverseLerpF64(f64 start, f64 value, f64 end);
v3 CrossProductV3(v3 v1, v3 v2);
f32 DotProductV3(v3 v1, v3 v2);
v2 NormalizeV2(v2 v);
v3 NormalizeV3(v3 v);
v4 NormalizeV4(v4 v);
f32 MapF32(f32 value, f32 inputStart, f32 inputEnd, f32 outputStart, f32 outputEnd);
f64 MapF64(f64 value, f64 inputStart, f64 inputEnd, f64 outputStart, f64 outputEnd);
v2 MapV2(v2 value, v2 inputStart, v2 inputEnd, v2 outputStart, v2 outputEnd);
v3 MapV3(v3 value, v3 inputStart, v3 inputEnd, v3 outputStart, v3 outputEnd);
f32 MapNormalizeF32(f32 start, f32 value, f32 end);

f32 SqrtF32(f32 f);
f32 SqrtI32(i32 i);
f32 LengthV2(v2 v);
f32 LengthV3(v3 v);
v2 PerpV2(v2 v);
v3 PerpV3(v3 v);
Angle GetAngleOfVectorsInRadeons(v2 v1, v2 v2);
f32 SnapToIntervalF32(f32 f, f32 interval);
v2 SnapToIntervalV2(v2 v, f32 interval);
v3 SnapToIntervalV3(v3 v, f32 interval);
b32 IsInRectV2(v2 pos, RectV2 rect);
b32 IsInterceptRectV2(RectV2 a, RectV2 b);
b32 IsInterceptRectIV2(RectIV2 a, RectIV2 b);
v2 PositionInCenterV2(v2 parentDim, v2 childDim);

u32 Murmur3U32(u32 key, u32 seed = 0, u32 prevHash = 0);
u32 Murmur3F32(f32 key, u32 seed, u32 prevHash = 0);
u32 Murmur3String(const char *key, u32 seed = 69);
u32 DJB33HashU32(u32 *data, u32 len, u32 seed = 5381);

i32 RandomInRangeI32(i32 min, i32 max);
f32 RandomF32ZeroToOne();

void ApplyAccelerationV2(v2 *pos, v2 *velocity, v2 acceleration, f64 timestep);

v4 HSLToRGBV4(float hue, float saturation, float luminance);
v4 HexToRGBV4(u32 hexValue);
ColorU32 HexToColorU32(u32 hexValue);

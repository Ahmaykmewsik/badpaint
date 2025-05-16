#include <base/vn_math.h>

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <xmmintrin.h>

#include <stdlib.h> // rand()
// ------------------------------------------------------------------------
// ------------------------------- v2, v2 ---------------------------------
// ------------------------------------------------------------------------

v2 operator+(v2 a, v2 b)
{
	v2 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return (result);
}

v2 operator-(v2 a, v2 b)
{
	v2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return (result);
}

v2 operator*(v2 a, v2 b)
{
	v2 result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	return (result);
}

v2 operator/(v2 a, v2 b)
{
	v2 result = {};
	if (ASSERT(b.x))
		result.x = a.x / b.x;
	if (ASSERT(b.y))
		result.y = a.y / b.y;
	return (result);
}

void operator+=(v2 &a, v2 b)
{
	a = a + b;
}

void operator-=(v2 &a, v2 b)
{
	a = a - b;
}

void operator*=(v2 &a, v2 b)
{
	a = a * b;
}

void operator/=(v2 &a, v2 b)
{
	a = a / b;
}

// ------------------------------------------------------------------------
// ------------------------------- v2, f32 --------------------------------
// ------------------------------------------------------------------------

v2 operator+(v2 a, f32 b)
{
	v2 result;
	result.x = a.x + b;
	result.y = a.y + b;
	return (result);
}

v2 operator-(v2 a, f32 b)
{
	v2 result;
	result.x = a.x - b;
	result.y = a.y - b;
	return (result);
}

v2 operator*(v2 a, f32 b)
{
	v2 result;
	result.x = a.x * b;
	result.y = a.y * b;
	return (result);
}

v2 operator/(v2 a, f32 b)
{
	v2 result = {};
	if (ASSERT(b))
	{
		result.x = a.x / b;
		result.y = a.y / b;
	}
	return (result);
}

void operator+=(v2 &a, f32 b)
{
	a = a + b;
}

void operator-=(v2 &a, f32 b)
{
	a = a - b;
}

void operator*=(v2 &a, f32 b)
{
	a = a * b;
}

void operator/=(v2 &a, f32 b)
{
	a = a / b;
}

// ------------------------------------------------------------------------
// ------------------------------- v2, f64 --------------------------------
// ------------------------------------------------------------------------

v2 operator*(v2 a, f64 b)
{
	v2 result;
	result.x = (f32) (a.x * b);
	result.y = (f32) (a.y * b);
	return result;
}

// ------------------------------------------------------------------------
// ------------------------------- iv2, iv2 -------------------------------
// ------------------------------------------------------------------------

iv2 operator+(iv2 a, iv2 b)
{
	iv2 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return (result);
}

iv2 operator-(iv2 a, iv2 b)
{
	iv2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return (result);
}

iv2 operator*(iv2 a, iv2 b)
{
	iv2 result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	return (result);
}

iv2 operator/(iv2 a, iv2 b)
{
	iv2 result = {};
	if (ASSERT(b.x))
		result.x = a.x / b.x;
	if (ASSERT(b.y))
		result.y = a.y / b.y;
	return (result);
}

b32 operator==(iv2 a, iv2 b)
{
	b32 result = (a.x == b.x && a.y == b.y);
	return result;
}

b32 operator!=(iv2 a, iv2 b)
{
	b32 result = !(a == b);
	return result;
}

void operator+=(iv2 &a, iv2 b)
{
	a = a + b;
}

void operator-=(iv2 &a, iv2 b)
{
	a = a - b;
}

void operator*=(iv2 &a, iv2 b)
{
	a = a * b;
}

void operator/=(iv2 &a, iv2 b)
{
	a = a / b;
}

// ------------------------------------------------------------------------
// ------------------------------- iv2, v2 --------------------------------
// ------------------------------------------------------------------------

v2 operator+(iv2 a, v2 b)
{
	v2 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return (result);
}

v2 operator-(iv2 a, v2 b)
{
	v2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return (result);
}

v2 operator*(iv2 a, v2 b)
{
	v2 result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	return (result);
}

v2 operator/(iv2 a, v2 b)
{
	v2 result = {};
	if (ASSERT(b.x))
		result.x = a.x / b.x;
	if (ASSERT(b.y))
		result.y = a.y / b.y;
	return (result);
}

//NOTE: (Marc) This transforms the vector from iv2 to v2, so no shorthand operators

// ------------------------------------------------------------------------
// ------------------------------- v2, iv2 --------------------------------
// ------------------------------------------------------------------------

v2 operator+(v2 a, iv2 b)
{
	v2 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return (result);
}

v2 operator-(v2 a, iv2 b)
{
	v2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return (result);
}

v2 operator*(v2 a, iv2 b)
{
	v2 result;
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	return (result);
}

v2 operator/(v2 a, iv2 b)
{
	v2 result = {};
	if (ASSERT(b.x))
		result.x = a.x / b.x;
	if (ASSERT(b.y))
		result.y = a.y / b.y;
	return (result);
}

void operator+=(v2 &a, iv2 b)
{
	a = a + b;
}

void operator-=(v2 &a, iv2 b)
{
	a = a - b;
}

void operator*=(v2 &a, iv2 b)
{
	a = a * b;
}

void operator/=(v2 &a, iv2 b)
{
	a = a / b;
}

// ------------------------------------------------------------------------
// ------------------------------- iv2, i32 -------------------------------
// ------------------------------------------------------------------------

iv2 operator+(iv2 a, i32 b)
{
	iv2 result;
	result.x = a.x + b;
	result.y = a.y + b;
	return (result);
}

iv2 operator-(iv2 a, i32 b)
{
	iv2 result;
	result.x = a.x - b;
	result.y = a.y - b;
	return (result);
}

iv2 operator*(iv2 a, i32 b)
{
	iv2 result;
	result.x = a.x * b;
	result.y = a.y * b;
	return (result);
}

iv2 operator/(iv2 a, i32 b)
{
	iv2 result = {};
	if (ASSERT(b))
	{
		result.x = a.x / b;
		result.y = a.y / b;
	}

	return (result);
}

void operator+=(iv2 &a, i32 b)
{
	a = a + b;
}

void operator-=(iv2 &a, i32 b)
{
	a = a - b;
}

void operator*=(iv2 &a, i32 b)
{
	a = a * b;
}

void operator/=(iv2 &a, i32 b)
{
	a = a / b;
}

// ------------------------------------------------------------------------
// ------------------------------- iv2, f32 -------------------------------
// ------------------------------------------------------------------------

v2 operator+(iv2 a, f32 b)
{
	v2 result;
	result.x = a.x + b;
	result.y = a.y + b;
	return (result);
}

v2 operator-(iv2 a, f32 b)
{
	v2 result;
	result.x = a.x - b;
	result.y = a.y - b;
	return (result);
}

v2 operator*(iv2 a, f32 b)
{
	v2 result;
	result.x = a.x * b;
	result.y = a.y * b;
	return (result);
}

v2 operator/(iv2 a, f32 b)
{
	v2 result = {};
	if (ASSERT(b))
	{
		result.x = a.x / b;
		result.y = a.y / b;
	}
	return (result);
}

// ------------------------------------------------------------------------
// ------------------------------- v3, v3 ---------------------------------
// ------------------------------------------------------------------------

v3 operator+(v3 a, v3 b)
{
	v3 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return (result);
}

v3 operator-(v3 a, v3 b)
{
	v3 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return (result);
}

void operator+=(v3 &a, v3 b)
{
	a = a + b;
}

void operator-=(v3 &a, v3 b)
{
	a = a - b;
}

// ------------------------------------------------------------------------
// ------------------------------- v3, f32 --------------------------------
// ------------------------------------------------------------------------

v3 operator*(v3 a, f32 b)
{
	v3 result;
	result.x = a.x * b;
	result.y = a.y * b;
	result.z = a.z * b;
	return (result);
}

v3 operator*(f32 a, v3 b)
{
	v3 result = b * a;
	return (result);
}

void operator*=(v3 &a, f32 b)
{
	a = a * b;
}

// ------------------------------------------------------------------------
// ------------------------------- v4, f32 --------------------------------
// ------------------------------------------------------------------------

v4 operator*(v4 a, f32 b)
{
	v4 result;
	__m128 scalar = _mm_set1_ps(b);
	result.sse = _mm_mul_ps(a.sse, scalar);
	return result;
}

// ------------------------------------------------------------------------
// ----------------------------- functions --------------------------------
// ------------------------------------------------------------------------

v2 IV2ToV2(iv2 v)
{
	v2 result = v2{(f32)v.x, (f32)v.y};
	return result;
}

v2 V3ToV2(v3 v)
{
	v2 result = v2{v.x, v.y};
	return result;
}

v3 V4ToV3(v4 v)
{
	v3 result = v3{v.x, v.y, v.z};
	return result;
}

v3 V2ToV3_1(v2 v)
{
	v3 result;
	result.x = v.x;
	result.y = v.y;
	result.z = 1;
	return result;
}

v3 V2ToV3_0(v2 v)
{
	v3 result;
	result.x = v.x;
	result.y = v.y;
	result.z = 0;
	return result;
}

v4 V3ToV4_W(v3 v, f32 w)
{
	v4 result;
	result.sse = _mm_setr_ps(v.x, v.y, v.z, w);
	return result;
}

v4 V3ToV4_1(v3 v)
{
	v4 result = V3ToV4_W(v, 1);
	return result;
}

V3Verticies V2VerticiesToV3Verticies_0(V2Verticies *v2Verticies)
{
	V3Verticies result;

	for (u32 i = 0; i < 4; i++)
	{
		result.verticies[i] = V2ToV3_0(v2Verticies->verticies[i]);
	}

	return result;
}

V2Verticies V3VerticiesToV2Verticies(V3Verticies *v3Verticies)
{
	V2Verticies result;

	for (u32 i = 0; i < 4; i++)
	{
		result.verticies[i] = V3ToV2(v3Verticies->verticies[i]);
	}

	return result;
}

V2Verticies RectV2ToV2Verticies(RectV2 rect)
{
	V2Verticies result;
	v2 min = rect.pos;
	v2 max = rect.pos + rect.dim;
	result.verticies[0] = v2{max.x, max.y};
	result.verticies[1] = v2{max.x, min.y};
	result.verticies[2] = v2{min.x, min.y};
	result.verticies[3] = v2{min.x, max.y};
	return result;
}

V3Verticies RectV2ToV3Verticies(RectV2 rect)
{
	V3Verticies result;
	v2 min = rect.pos;
	v2 max = rect.pos + rect.dim;
	result.verticies[0] = v3{max.x, max.y, 0};
	result.verticies[1] = v3{max.x, min.y, 0};
	result.verticies[2] = v3{min.x, min.y, 0};
	result.verticies[3] = v3{min.x, max.y, 0};
	return result;
}

v2 GetRectV2TopLeftPos(RectV2 rect)
{
	v2 result = rect.pos + v2{0, rect.dim.y};
	return result;
}

//-------------------------------------------------------------
//------------------------Math Math----------------------------
//-------------------------------------------------------------

b32 IsZeroV2(v2 v)
{
	b32 result = !(v.x || v.y);
	return result;
}

b32 IsZeroIV2(iv2 v)
{
	b32 result = !(v.x || v.y);
	return result;
}

b32 IsZeroV3(v3 v)
{
	b32 result = !(v.x || v.y || v.z);
	return result;
}

f32 SignF32(f32 f)
{
    u32 u;
    memcpy(&u, &f, sizeof(u));
    u = (u & 0x80000000) | 0x3F800000;
    memcpy(&f, &u, sizeof(f));
    return f;
}

f32 RoundF32(f32 f)
{
	f32 result = (f32) round(f);
	return result;
}

i32 RoundI32(f32 f)
{
	i32 result = (i32) round(f);
	return result;
}

u64 RoundU64(f64 f)
{
	u64 result = (u64) round(f);
	return result;
}

v2 RoundV2(v2 v)
{
	v2 result;
	result.x = (f32) round(v.x);
	result.y = (f32) round(v.y);
	return result;
}

v3 RoundV3(v3 v)
{
	v3 result;
	result.x = (f32) round(v.x);
	result.y = (f32) round(v.y);
	result.z = (f32) round(v.z);
	return result;
}

f32 FloorF32(f32 f)
{
	f32 result = (f32) floor(f);
	return result;
}

v2 FloorV2(v2 v)
{
	v2 result = v2{(f32)floor(v.x), (f32)floor(v.y)};
	return result;
}

f32 CeilF32(f32 f)
{
	f32 result = (f32) ceil(f);
	return result;
}

v2 CeilV2(v2 v)
{
	v2 result = v2{(f32)ceil(v.x), (f32)ceil(v.y)};
	return result;
}

f32 AbsF32(f32 f)
{
	f32 result = fabsf(f);
	return result;
}

v2 AbsV2(v2 v)
{
	v2 result;

	result.x = fabsf(v.x);
	result.y = fabsf(v.y);

	return result;
}

v3 AbsV3(v3 v)
{
	v3 result;

	result.x = fabsf(v.x);
	result.y = fabsf(v.y);
	result.z = fabsf(v.z);

	return result;
}

f32 SafeDivideF32(f32 a, f32 b)
{
	f32 result = {};

	if (b)
	{
		result = a / b;
	}

	return result;
}

f64 SafeDivideF64(f64 a, f64 b)
{
	f64 result = {};

	if (b)
	{
		result = a / b;
	}

	return result;
}

f32 SafeDivideI32(i32 a, i32 b)
{
	f32 result = {};

	if (b)
	{
		result = a / (f32) b;
	}

	return result;
}

f32 DotProductV2(v2 v1, v2 v2)
{
	f32 result = (v1.x * v2.x) + (v1.y * v2.y);
	return result;
}

f32 DotProductV3(v3 v1, v3 v2)
{
	f32 result = (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
	return result;
}

f32 DotProductV4(v4 v1, v4 v2)
{
	f32 result;

	__m128 SSEResultOne = _mm_mul_ps(v1.sse, v2.sse);
	__m128 SSEResultTwo =
		_mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
	SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
	SSEResultTwo =
		_mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
	SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
	_mm_store_ss(&result, SSEResultOne);

	return result;
}

v3 CrossProductV3(v3 v1, v3 v2)
{
	v3 result = {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
		v1.x * v2.y - v1.y * v2.x};

	return result;
}

f32 SqrtF32(f32 f)
{
	__m128 In = _mm_set_ss(f);
	__m128 Out = _mm_sqrt_ss(In);
	f32 result = _mm_cvtss_f32(Out);

	return result;
}

f32 SqrtI32(i32 i)
{
    __m128 In = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
    __m128 Out = _mm_sqrt_ss(In);
    f32 result = _mm_cvtss_f32(Out);
    return result;
}

f32 InvSqrtF32(f32 f)
{
	f32 result = 1.0f / SqrtF32(f);
	return result;
}

f32 LengthV2(v2 v)
{
	f32 result = SqrtF32(DotProductV2(v, v));
	return result;
}

f32 LengthV3(v3 v)
{
	f32 result = SqrtF32(DotProductV3(v, v));
	return result;
}

v2 PerpV2(v2 v)
{
	v2 result = v2{-v.y, v.x};
	return result;
}

v3 PerpV3(v3 v)
{
	v3 result = {};

	f32 min = (f32)fabs(v.x);
	v3 cardinalAxis = {1.0f, 0.0f, 0.0f};

	if (fabs(v.y) < min)
	{
		min = (f32)fabs(v.y);
		v3 tmp = {0.0f, 1.0f, 0.0f};
		cardinalAxis = tmp;
	}

	if (fabs(v.z) < min)
	{
		v3 tmp = {0.0f, 0.0f, 1.0f};
		cardinalAxis = tmp;
	}

	result = CrossProductV3(v, cardinalAxis);

	return result;
}

Angle GetAngleOfVectorsInRadeons(v2 v1, v2 v2)
{
	Angle result = {};

	f32 lengths = LengthV2(v1) * LengthV2(v2);
	if (lengths)
	{
		result.theta = DotProductV2(v1, v2) / lengths;
		result.perpendicularTest = DotProductV2(v1, PerpV2(v2));
	}

	return result;
}

f32 SnapToIntervalF32(f32 f, f32 interval)
{
	f32 result = 0;
	if (interval != 0)
	{
		result = RoundF32(f / interval) * interval;
	}
	return result;
}

v2 SnapToIntervalV2(v2 v, f32 interval)
{
	v2 result;

	result.x = SnapToIntervalF32(v.x, interval);
	result.y = SnapToIntervalF32(v.y, interval);

	return result;
}

v3 SnapToIntervalV3(v3 v, f32 interval)
{
	v3 result;

	result.x = SnapToIntervalF32(v.x, interval);
	result.y = SnapToIntervalF32(v.y, interval);
	result.z = SnapToIntervalF32(v.z, interval);

	return result;
}

f32 MostlyEqualsF32(f32 f1, f32 f2, f32 epsilon)
{
	return f1 >= f2 - epsilon && f1 <= f2 + epsilon;
}

b32 IsInRectV2(v2 pos, RectV2 rect)
{
	b32 result = ((pos.x >= rect.pos.x) && (pos.y >= rect.pos.y) &&
			(pos.x <= (rect.pos.x + rect.dim.x)) &&
			(pos.y <= (rect.pos.y + rect.dim.y)));

	return result;
}

b32 IsInterceptRectV2(RectV2 a, RectV2 b)
{
    bool result = (a.pos.x < b.pos.x + b.dim.x) &&
           (a.pos.x + a.dim.x > b.pos.x) &&
           (a.pos.y < b.pos.y + b.dim.y) &&
           (a.pos.y + a.dim.y > b.pos.y);
	return result;
}

b32 IsInterceptRectIV2(RectIV2 a, RectIV2 b)
{
    bool result = (a.pos.x < b.pos.x + b.dim.x) &&
           (a.pos.x + a.dim.x > b.pos.x) &&
           (a.pos.y < b.pos.y + b.dim.y) &&
           (a.pos.y + a.dim.y > b.pos.y);
	return result;
}

u32 ModNextU32(u32 i, u32 size)
{
	return (i + 1) % size;
}

u32 ModBackU32(u32 i, u32 size)
{
	return (size + (i - 1)) % size;
}

u32 MinU32(u32 i, u32 j)
{
	u32 result = i;

	if (i > j)
	{
		result = j;
	}

	return result;
}

i32 MinI32(i32 i, i32 j)
{
	u32 result = i;

	if (i > j)
	{
		result = j;
	}

	return result;
}

f32 MinF32(f32 i, f32 j)
{
	f32 result = i;

	if (i > j)
	{
		result = j;
	}

	return result;
}

u32 MaxU32(u32 i, u32 j)
{
	u32 result = i;

	if (i < j)
	{
		result = j;
	}

	return result;
}

i32 MaxI32(i32 i, i32 j)
{
	i32 result = i;

	if (i < j)
	{
		result = j;
	}

	return result;
}


f32 MaxF32(f32 i, f32 j)
{
	f32 result = i;

	if (i < j)
	{
		result = j;
	}

	return result;
}

i32 ClampI32(i32 min, i32 value, i32 max)
{
	i32 result = value;

	if (result < min)
		result = min;
	else if (result > max)
		result = max;

	return (result);
}

f32 ClampF32(f32 min, f32 value, f32 max)
{
	f32 result = value;

	if (result < min)
		result = min;
	else if (result > max)
		result = max;

	return (result);
}

f64 ClampF64(f64 min, f64 value, f64 max)
{
	f64 result = value;

	if (result < min)
		result = min;
	else if (result > max)
		result = max;

	return (result);
}

void FlipV2Y(v2 *v, f32 parentHeight)
{
	v->y = parentHeight - v->y;
}

void FlipRectV2Y(RectV2 *rect, f32 parentHeight)
{
	rect->pos.y = parentHeight - rect->pos.y - rect->dim.y;
}

f32 DistanceV2(v2 v1, v2 v2)
{
	f32 result = 0.0f;

	f32 dx = v2.x - v1.x;
	f32 dy = v2.y - v1.y;
	result = SqrtF32(dx * dx + dy * dy);

	return result;
}

f32 DistanceV3(v3 v1, v3 v2)
{
	f32 result = 0.0f;

	f32 dx = v2.x - v1.x;
	f32 dy = v2.y - v1.y;
	f32 dz = v2.z - v1.z;
	result = SqrtF32(dx * dx + dy * dy + dz * dz);

	return result;
}

v2 NormalizeV2(v2 v)
{
	v2 result = {};
	f32 length = SqrtF32((v.x * v.x) + (v.y * v.y));

	if (length > 0)
	{
		result.x = v.x * 1.0f / length;
		result.y = v.y * 1.0f / length;
	}

	return result;
}

// TODO: use HMM optimization
v3 NormalizeV3(v3 v)
{
	v3 result = v;

	f32 length = SqrtF32(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length == 0.0f)
		length = 1.0f;
	f32 ilength = 1.0f / length;

	result.x *= ilength;
	result.y *= ilength;
	result.z *= ilength;

	return result;
}

v4 NormalizeV4(v4 v)
{
	v4 result = v * InvSqrtF32(DotProductV4(v, v));
	return result;
}

f32 LerpF32(f32 start, f32 amount, f32 end)
{
	f32 result = (1.0f - amount) * start + amount * end;
	return result;
}

v2 LerpV2(v2 start, f32 amount, v2 end)
{
	v2 result;

	result.x = LerpF32(start.x, amount, end.x);
	result.y = LerpF32(start.y, amount, end.y);

	return result;
}

v3 LerpV3(v3 start, f32 amount, v3 end)
{
	v3 result;

	result.x = LerpF32(start.x, amount, end.x);
	result.y = LerpF32(start.y, amount, end.y);
	result.z = LerpF32(start.z, amount, end.z);

	return result;
}

f32 InverseLerpF32(f32 start, f32 value, f32 end)
{
	f32 result = SafeDivideF32(value - start, end - start);
	return result;
}

f64 InverseLerpF64(f64 start, f64 value, f64 end)
{
	f64 result = SafeDivideF64(value - start, end - start);
	return result;
}

f32 MapF32(f32 value, f32 inputStart, f32 inputEnd, f32 outputStart, f32 outputEnd)
{
	f32 result = value;

	if (inputEnd - inputStart != 0)
		result = (value - inputStart) / (inputEnd - inputStart) *
			(outputEnd - outputStart) +
			outputStart;

	return result;
}

f64 MapF64(f64 value, f64 inputStart, f64 inputEnd, f64 outputStart, f64 outputEnd)
{
	f64 result = value;

	if (inputEnd - inputStart != 0)
		result = (value - inputStart) / (inputEnd - inputStart) *
			(outputEnd - outputStart) +
			outputStart;

	return result;
}

v2 MapV2(v2 value, v2 inputStart, v2 inputEnd, v2 outputStart, v2 outputEnd)
{
	v2 result;
	result.x = MapF32(value.x, inputStart.x, inputEnd.x, outputStart.x, outputEnd.x);
	result.y = MapF32(value.y, inputStart.y, inputEnd.y, outputStart.y, outputEnd.y);
	return result;
}

v3 MapV3(v3 value, v3 inputStart, v3 inputEnd, v3 outputStart, v3 outputEnd)
{
	v3 result;
	result.x = MapF32(value.x, inputStart.x, inputEnd.x, outputStart.x, outputEnd.x);
	result.y = MapF32(value.y, inputStart.y, inputEnd.y, outputStart.y, outputEnd.y);
	result.z = MapF32(value.z, inputStart.z, inputEnd.z, outputStart.z, outputEnd.z);
	return result;
}

f32 MapNormalizeF32(f32 start, f32 value, f32 end)
{
	f32 result = 0;

	if (end - start)
	{
		result = (value - start) / (end - start);
	}

	return result;
}

void CenterV2(v2 *pos, v2 boxDim, v2 entityDim)
{
	*pos += (boxDim - entityDim) * 0.5f;
}

f32 GetTargetFrameMsF32(f32 framerate)
{
	f32 result = SafeDivideF32(1 , framerate) * 1000.0f;
	ASSERT(result);
	return result;
}

f32 FramesToMsF32(f32 framerate, f32 frames)
{
	f32 result = frames * GetTargetFrameMsF32(framerate);
	return result;
}

f32 MsToFramesF32(f32 framerate, f32 ms)
{
	f32 result = SafeDivideF32(ms , GetTargetFrameMsF32(framerate));
	return result;
}

u32 MilisecondsToRoundedAnimationFrame(f32 framerate, f32 miliseconds)
{
	u32 result = (u32) FloorF32(MsToFramesF32(framerate, miliseconds));
	return result;
}

u32 NextPowerOfTwo(u32 x)
{
	u32 result = 1;

	if (x != 0)
	{
		x -= 1;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x += 1;
		result = x;
	}

	return result;
}

f32 SquareF32(f32 a)
{
	f32 Result = a * a;
	return (Result);
}

f64 SquareF64(f64 a)
{
	f64 Result = a * a;
	return (Result);
}

f32 CosF32(f32 f)
{
	f32 result = (f32) cos(f);
	return result;
}

f32 SinF32(f32 f)
{
	f32 result = (f32) sin(f);
	return result;
}

v4 SRGB255ToLinear1(v4 C)
{
	v4 Result;

	f32 Inv255 = 1.0f / 255.0f;

	Result.r = SquareF32(Inv255 * C.r);
	Result.g = SquareF32(Inv255 * C.g);
	Result.b = SquareF32(Inv255 * C.b);
	Result.a = Inv255 * C.a;

	return (Result);
}

v4 Linear1ToSRGB255(v4 C)
{
	v4 Result;

	f32 One255 = 255.0f;

	Result.r = One255 * SqrtF32(C.r);
	Result.g = One255 * SqrtF32(C.g);
	Result.b = One255 * SqrtF32(C.b);
	Result.a = One255 * C.a;

	return (Result);
}

f32 FontPointsToPixels(f32 points, f32 dpi)
{
	f32 result = points * (1.0f / 72.0f) * dpi;
	return result;
}

v2 PositionInCenterV2(v2 parentDim, v2 childDim)
{
	v2 result = (parentDim * 0.5f) - (childDim * 0.5f);
	return result;
}

#define MURMUR_C1 0xcc9e2d51
#define MURMUR_C2 0x1b873593
#define MURMUR_R1 15
#define MURMUR_R2 13
#define MURMUR_M 5
#define MURMUR_N 0xe6546b64

u32 CombineHashes(u32 hash1, u32 hash2)
{
	hash1 ^= hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2);
	return hash1;
}

u32 Murmur3U32(u32 key, u32 seed, u32 prevHash)
{
	u32 hash = seed;

	key *= MURMUR_C1;
	key = (key << MURMUR_R1) | (key >> (32 - MURMUR_R1));
	key *= MURMUR_C2;

	hash ^= key;
	hash =
		((hash << MURMUR_R2) | (hash >> (32 - MURMUR_R2))) * MURMUR_M + MURMUR_N;

	if (prevHash)
		hash = CombineHashes(hash, prevHash);

	return hash;
}

u32 Murmur3F32(f32 key, u32 seed, u32 prevHash)
{
	u32 int_key;
	memcpy(&int_key, &key, sizeof(u32));
	u32 hash = Murmur3U32(int_key, seed, prevHash);
	return hash;
}

u32 Murmur3StringLength(const char *key, u32 length, u32 seed)
{
	u32 hash = seed;
	u32 nblocks = length / 4;
	u32 *blocks = (u32 *)(key);

	for (u32 i = 0; i < nblocks; i++)
	{
		hash = Murmur3U32(blocks[i], hash);
	}

	u8 *tail = (u8 *)(key + nblocks * 4);
	u32 k1 = 0;

	switch (length & 3)
	{
		case 3:
			k1 ^= tail[2] << 16;
		case 2:
			k1 ^= tail[1] << 8;
		case 1:
			k1 ^= tail[0];
			k1 *= MURMUR_C1;
			k1 = (k1 << MURMUR_R1) | (k1 >> (32 - MURMUR_R1));
			k1 *= MURMUR_C2;
			hash ^= k1;
	}

	hash ^= length;
	hash ^= (hash >> 16);
	hash *= 0x85ebca6b;
	hash ^= (hash >> 13);
	hash *= 0xc2b2ae35;
	hash ^= (hash >> 16);

	return hash;
}

u32 Murmur3String(const char *key, u32 seed)
{
	u32 length = (u32) strlen(key);
	u32 result = Murmur3StringLength(key, length, seed);
    return result;
}

u32 DJB33HashU32(u32 *data, u32 len, u32 seed)
{
	u32 h = seed;
	while (len--) 
	{
		h += (h << 5);
		h ^= *data++;
	}
	return h;
}

// NOTE: LCG parameters
static u64 G_VN_RANDOM_SEED = time(NULL);
static const u32 G_VN_RAND_A = 1664525;
static const u32 G_VN_RAND_C = 1013904223;
static const u32 G_CN_RAND_M = UINT32_MAX; // 2^32 - 1

u32 vn_rand()
{
	G_VN_RANDOM_SEED = (G_VN_RAND_A * G_VN_RANDOM_SEED + G_VN_RAND_C) % G_CN_RAND_M;
	return (u32) G_VN_RANDOM_SEED;
}

i32 RandomInRangeI32(i32 min, i32 max)
{
	if (min > max)
	{
		i32 tmp = max;
		max = min;
		min = tmp;
	}

	u32 randU32 = rand();
	i32 result = (randU32 % (abs(max - min) + 1) + min);
	return result; 
}

f32 RandomF32ZeroToOne()
{
    union { u32 u32; f32 f; } u;
    u.u32 = (vn_rand() >> 9) | 0x3f800000;
    return u.f - 1.0f;
}

//NOTE: (Marc) Semi-Implicit Euler
void ApplyAccelerationV2(v2 *pos, v2 *velocity, v2 acceleration, f64 timestep)
{
	*velocity = acceleration * timestep + *velocity;
	*pos += (acceleration * SquareF64(timestep) * 0.5) + (*velocity * timestep);
}

f32 HueToRGBF32(float p, float q, float t)
{
	f32 result = p;
	if (t < 0)
		t += 1;
	if (t > 1)
		t -= 1;

	if (t < 1.0f/6)
		result = p + (q - p) * 6 * t;
	else if (t < 1.0f/2)
		result = q;
	else if (t < 2.0f/3)
		result = p + (q - p) * (2.0f/3 - t) * 6;

	return result;
}

v4 HSLToRGBV4(float h, float s, float l)
{
	v4 result;

	if(0 == s)
	{
		result.r = l;
		result.g = l;
		result.b = l;
	}
	else
	{
		float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
		float p = 2 * l - q;
		result.r = HueToRGBF32(p, q, h + 1.0f/3) * 255.0f;
		result.g = HueToRGBF32(p, q, h) * 255.0f;
		result.b = HueToRGBF32(p, q, h - 1.0f/3) * 255.0f;
	}

	return result;
}

v4 HexToRGBV4(u32 hexValue)
{
	v4 result;
	result.r = ((hexValue >> 16) & 0xFF) / 255.0f;
	result.g = ((hexValue >> 8) & 0xFF) / 255.0f;
	result.b = ((hexValue) & 0xFF) / 255.0f;
	result.a = 1;
	return result;
}

ColorU32 HexToColorU32(u32 hexValue)
{
	ColorU32 result;
	result.r = (hexValue >> 16) & 0xFF;
	result.g = (hexValue >> 8) & 0xFF;
	result.b = (hexValue) & 0xFF;
	result.a = 255;
	return result;
}

ColorU32 AddConstantToColor(ColorU32 color, i8 constant)
{
	ColorU32 result = color;
	result.r = (u8) ClampI32(0, result.r + constant, 255);
	result.g = (u8) ClampI32(0, result.g + constant, 255);
	result.b = (u8) ClampI32(0, result.b + constant, 255);
	return result;
}

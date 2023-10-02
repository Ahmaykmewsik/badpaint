#pragma once

#include "stdint.h"
#include "vn_intrinsics.h"
#include <stdint.h>
#include "stdlib.h"
#include <xmmintrin.h>
#include <time.h>

#define PI 3.14159265358979323846f
#define WINDOWS_FILETIME_TO_UNIXTIME(ft) (UINT)((*(LONGLONG*)&(ft)-116444736000000000)/10000000)

//-------------------------------------------------------------
//---------------------------V2--------------------------------
//-------------------------------------------------------------

union V2
{
    struct
    {
        float x, y;
    };

    float elements[2];
};

inline bool IsZero(V2 v)
{
    bool result = !(v.x || v.y);
    return result;
}

inline V2 operator+(V2 a, V2 b)
{
    V2 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return (result);
}

inline V2 operator-(V2 a, V2 b)
{
    V2 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return (result);
}

inline V2 operator*(V2 a, V2 b)
{
    V2 result;

    result.x = a.x * b.x;
    result.y = a.y * b.y;

    return (result);
}

inline V2 operator/(V2 v1, V2 v2)
{
    Assert(v2.x && v2.y);
    V2 result;
    if (v2.x)
        result.x = v1.x / v2.x;
    if (v2.y)
        result.y = v1.y / v2.y;
    return (result);
}

inline void operator+=(V2 &a, V2 b)
{
    a = a + b;
}

inline void operator-=(V2 &a, V2 b)
{
    a = a - b;
}

inline void operator*=(V2 &a, V2 b)
{
    a = a * b;
}

inline bool operator==(V2 a, V2 b)
{
    bool result = (a.x == b.x) && (a.y == b.y);
    return result;
}

inline bool operator!=(V2 a, V2 b)
{
    bool result = !(a == b);
    return result;
}

//----V2 & int----

inline V2 operator+(V2 v, int i)
{
    V2 result;

    result.x = v.x + i;
    result.y = v.y + i;

    return (result);
}

inline V2 operator-(V2 v, int i)
{
    V2 result;

    result.x = v.x - i;
    result.y = v.y - i;

    return (result);
}

inline V2 operator*(V2 v, int i)
{
    V2 result;

    result.x = v.x * i;
    result.y = v.y * i;

    return (result);
}

inline V2 operator/(V2 v, int i)
{
    Assert(i);
    V2 result;
    if (i)
    {
        result.x = v.x / i;
        result.y = v.y / i;
    }

    return (result);
}

inline V2 operator/(int i, V2 v)
{
    Assert(v.x && v.y);

    V2 result;
    if (!v.x || !v.y)
    {
        result.x = i / v.x;
        result.y = i / v.y;
    }

    return (result);
}

inline void operator+=(V2 &a, int b)
{
    a = a + b;
}

inline void operator-=(V2 &a, int b)
{
    a = a - b;
}

inline void operator*=(V2 &a, int b)
{
    a = a * b;
}

inline void operator/=(V2 &a, int b)
{
    a = a / b;
}

//----V2 & float----

inline V2 operator+(V2 v, float f)
{
    V2 result;

    result.x = v.x + f;
    result.y = v.y + f;

    return (result);
}

inline V2 operator+(float f, V2 v)
{
    return v + f;
}

inline V2 operator-(V2 v, float f)
{
    V2 result;

    result.x = v.x - f;
    result.y = v.y - f;

    return (result);
}

inline V2 operator-(float f, V2 v)
{
    V2 result;

    result.x = f - v.x;
    result.y = f - v.y;

    return (result);
}

inline V2 operator*(V2 v, float f)
{
    V2 result;

    result.x = v.x * f;
    result.y = v.y * f;

    return (result);
}

inline V2 operator*(float f, V2 v)
{
    return v * f;
}

inline V2 operator/(V2 v, float f)
{
    Assert(f);
    V2 result;
    if (f)
    {
        result.x = v.x / f;
        result.y = v.y / f;
    }

    return (result);
}

inline void operator+=(V2 &a, float b)
{
    a = a + b;
}

inline void operator-=(V2 &a, float b)
{
    a = a - b;
}

inline void operator*=(V2 &a, float b)
{
    a = a * b;
}

inline void operator/=(V2 &a, float b)
{
    a = a / b;
}

//-------------------------------------------------------------
//---------------------------V3--------------------------------
//-------------------------------------------------------------

union V3
{
    struct
    {
        float x, y, z;
    };

    float elements[3];
};

inline bool IsZero(V3 v)
{
    bool result = !(v.x || v.y || v.x);
    return result;
}

inline V3 operator+(V3 a, V3 b)
{
    V3 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return (result);
}

inline V3 operator-(V3 a, V3 b)
{
    V3 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return (result);
}

inline V3 operator*(V3 a, V3 b)
{
    V3 result;

    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;

    return (result);
}

inline V3 operator/(V3 v1, V3 v2)
{
    Assert(v2.x && v2.y && v2.z);

    V3 result = {};

    if (v2.x)
        result.x = v1.x / v2.x;
    if (v2.y)
        result.y = v1.y / v2.y;
    if (v2.z)
        result.z = v1.z / v2.z;

    return (result);
}

inline void operator+=(V3 &a, V3 b)
{
    a = a + b;
}

inline void operator-=(V3 &a, V3 b)
{
    a = a - b;
}

inline void operator*=(V3 &a, V3 b)
{
    a = a * b;
}

inline void operator/=(V3 &a, V3 b)
{
    a = a / b;
}

inline bool operator==(V3 a, V3 b)
{
    bool result = (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
    return result;
}

inline bool operator!=(V3 a, V3 b)
{
    bool result = !(a == b);
    return result;
}

//----V2 & int----

inline V3 operator+(V3 v, int i)
{
    V3 result;

    result.x = v.x + i;
    result.y = v.y + i;
    result.z = v.z + i;

    return (result);
}

inline V3 operator-(V3 v, int i)
{
    V3 result;

    result.x = v.x - i;
    result.y = v.y - i;
    result.z = v.z - i;

    return (result);
}

inline V3 operator*(V3 v, int i)
{
    V3 result;

    result.x = v.x * i;
    result.y = v.y * i;
    result.z = v.z * i;

    return (result);
}

inline V3 operator/(V3 v, int i)
{
    Assert(i);
    V3 result;
    if (i)
    {
        result.x = v.x / i;
        result.y = v.y / i;
        result.z = v.z / i;
    }

    return (result);
}

inline void operator+=(V3 &a, int b)
{
    a = a + b;
}

inline void operator-=(V3 &a, int b)
{
    a = a - b;
}

inline void operator*=(V3 &a, int b)
{
    a = a * b;
}

inline void operator/=(V3 &a, int b)
{
    a = a / b;
}

//----V3 & float----

inline V3 operator+(V3 v, float f)
{
    V3 result;

    result.x = v.x + f;
    result.y = v.y + f;
    result.z = v.z + f;

    return (result);
}

inline V3 operator-(V3 v, float f)
{
    V3 result;

    result.x = v.x - f;
    result.y = v.y - f;
    result.z = v.z - f;

    return (result);
}

inline V3 operator*(V3 v, float f)
{
    V3 result;

    result.x = v.x * f;
    result.y = v.y * f;
    result.z = v.z * f;

    return (result);
}

inline V3 operator/(V3 v, float f)
{
    Assert(f);
    V3 result;
    if (f)
    {
        result.x = v.x / f;
        result.y = v.y / f;
        result.z = v.z / f;
    }

    return (result);
}

inline void operator+=(V3 &a, float b)
{
    a = a + b;
}

inline void operator-=(V3 &a, float b)
{
    a = a - b;
}

inline void operator*=(V3 &a, float b)
{
    a = a * b;
}

inline void operator/=(V3 &a, float b)
{
    a = a / b;
}

//-------------------------------------------------------------
//---------------------------V4--------------------------------
//-------------------------------------------------------------

typedef union V4
{
    struct
    {
        union
        {
            V3 xyz;
            struct
            {
                float x, y, z;
            };
        };

        float w;
    };

    struct
    {
        union
        {
            V3 rgb;
            struct
            {
                float r, g, b;
            };
        };

        float a;
    };

    __m128 sse;

} V4;

inline V4 operator*(V4 v, float f)
{
    V4 result;

    __m128 scalar = _mm_set1_ps(f);
    result.sse = _mm_mul_ps(v.sse, scalar);

    return result;
}

//-------------------------------------------------------------
//-----------------------Physics Structs-----------------------
//-------------------------------------------------------------

struct LineV2
{
    V2 start;
    V2 end;
};

struct LineV3
{
    V3 start;
    V3 end;
};

struct BoxV2
{
    V2 min;
    V2 max;
};

struct Rect
{
    V2 pos;
    V2 dim;
};

struct V2Verticies
{
    V2 verticies[4];
};

struct V3Verticies
{
    V3 verticies[4];
};

//-------------------------------------------------------------
//-----------------------Conversions---------------------------
//-------------------------------------------------------------

inline V2 V3ToV2(V3 v)
{
    V2 result = V2{v.x, v.y};
    return result;
}

inline V3 V4ToV3(V4 v)
{
    V3 result = V3{v.x, v.y, v.z};
    return result;
}

inline V3 V2ToV3_1(V2 v)
{
    V3 result;

    result.x = v.x;
    result.y = v.y;
    result.z = 1;

    return result;
}

inline V3 V2ToV3_0(V2 v)
{
    V3 result;

    result.x = v.x;
    result.y = v.y;
    result.z = 0;

    return result;
}

inline V4 V3ToV4(V3 v, float w)
{
    V4 result;
    result.sse = _mm_setr_ps(v.x, v.y, v.z, w);
    return result;
}

inline V4 V3ToV4_1(V3 v)
{
    V4 result = V3ToV4(v, 1);
    return result;
}

inline V3 operator+(V3 a, V2 b)
{
    return a + V2ToV3_0(b);
}

inline V3 operator-(V3 a, V2 b)
{
    return a - V2ToV3_0(b);
}

inline V3 operator-(V2 a, V3 b)
{
    return V2ToV3_0(a) - b;
}

inline V3 operator*(V3 a, V2 b)
{
    return a * V2ToV3_0(b);
}

inline V3Verticies V2VerticiesToV3Verticies_0(V2Verticies *v2Verticies)
{
    V3Verticies result;

    for (int i = 0;
         i < 4;
         i++)
    {
        result.verticies[i] = V2ToV3_0(v2Verticies->verticies[i]);
    }

    return result;
}

inline V2Verticies V3VerticiesToV2Verticies(V3Verticies *v3Verticies)
{
    V2Verticies result;

    for (int i = 0;
         i < 4;
         i++)
    {
        result.verticies[i] = V3ToV2(v3Verticies->verticies[i]);
    }

    return result;
}

inline Rect BoxV2ToRect(BoxV2 box)
{
    Rect result;

    result.pos.x = box.min.x;
    result.pos.y = box.min.y;
    //NOTE: we need to include the points themselves in the dimentions
    result.dim.x = fmax(0, box.max.x - box.min.x + 1);
    result.dim.y = fmax(0, box.max.y - box.min.y + 1);

    return result;
}

inline BoxV2 RectToBoxV2(Rect rect)
{
    BoxV2 result;

    result.min = rect.pos;
    result.max = rect.pos + rect.dim;

    return result;
}

inline V2Verticies RectToV2Verticies(Rect rect)
{
    V2Verticies result;

    BoxV2 box;
    box.min = rect.pos;
    box.max = rect.pos + rect.dim;

    result.verticies[0] = V2{box.max.x, box.max.y};
    result.verticies[1] = V2{box.max.x, box.min.y};
    result.verticies[2] = V2{box.min.x, box.min.y};
    result.verticies[3] = V2{box.min.x, box.max.y};

    return result;
}

inline V3Verticies RectToV3Verticies(Rect rect)
{
    V3Verticies result;

    BoxV2 box;
    box.min = rect.pos;
    box.max = rect.pos + rect.dim;

    result.verticies[0] = V3{box.max.x, box.max.y, 0};
    result.verticies[1] = V3{box.max.x, box.min.y, 0};
    result.verticies[2] = V3{box.min.x, box.min.y, 0};
    result.verticies[3] = V3{box.min.x, box.max.y, 0};

    return result;
}

inline V2 GetRectTopLeftPos(Rect rect)
{
    V2 result = rect.pos + V2{0, rect.dim.y};
    return result;
}

//-------------------------------------------------------------
//------------------------Math Math----------------------------
//-------------------------------------------------------------

inline float Round(float f)
{
    float result = round(f);

    return result;
}

inline V2 Round(V2 v2)
{
    V2 result;

    result.x = round(v2.x);
    result.y = round(v2.y);

    return result;
}

inline V3 Round(V3 v3)
{
    V3 result;

    result.x = round(v3.x);
    result.y = round(v3.y);
    result.z = round(v3.z);

    return result;
}

inline float Floor(float f)
{
    float result = floor(f);
    return result;
}

inline V2 Floor(V2 v)
{
    V2 result = V2{(float)floor(v.x), (float)floor(v.y)};
    return result;
}

inline float Ceil(float f)
{
    float result = ceil(f);
    return result;
}

inline V2 Ceil(V2 v)
{
    V2 result = V2{(float)ceil(v.x), (float)ceil(v.y)};
    return result;
}

inline V2 abs(V2 v)
{
    V2 result;

    result.x = abs(v.x);
    result.y = abs(v.y);

    return result;
}

inline V3 abs(V3 v)
{
    V3 result;

    result.x = abs(v.x);
    result.y = abs(v.y);
    result.z = abs(v.z);

    return result;
}

inline float DotProductV2(V2 v1, V2 v2)
{
    float result = (v1.x * v2.x) + (v1.y * v2.y);
    return result;
}

inline float DotProductV3(V3 v1, V3 v2)
{
    float result = (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
    return result;
}

inline float DotProductV4(V4 v1, V4 v2)
{
    float result;

    __m128 SSEResultOne = _mm_mul_ps(v1.sse, v2.sse);
    __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    _mm_store_ss(&result, SSEResultOne);

    return result;
}

inline V3 CrossProductV3(V3 v1, V3 v2)
{
    V3 result = {v1.y * v2.z - v1.z * v2.y,
                 v1.z * v2.x - v1.x * v2.z,
                 v1.x * v2.y - v1.y * v2.x};

    return result;
}

inline float SqrtFloat(float f)
{
    __m128 In = _mm_set_ss(f);
    __m128 Out = _mm_sqrt_ss(In);
    float result = _mm_cvtss_f32(Out);

    return result;
}

inline float InvSqrtFloat(float f)
{
    float result = 1.0f / SqrtFloat(f);
    return result;
}

inline float Length(V2 v)
{
    float result = SqrtFloat(DotProductV2(v, v));
    return result;
}

inline float Length(V3 v)
{
    float result = SqrtFloat(DotProductV3(v, v));
    return result;
}

inline V2 PerpV2(V2 v)
{
    V2 result = V2{-v.y, v.x};
    return result;
}

inline V3 PerpV3(V3 v)
{
    V3 result = {};

    float min = (float)fabs(v.x);
    V3 cardinalAxis = {1.0f, 0.0f, 0.0f};

    if (fabs(v.y) < min)
    {
        min = (float)fabs(v.y);
        V3 tmp = {0.0f, 1.0f, 0.0f};
        cardinalAxis = tmp;
    }

    if (fabs(v.z) < min)
    {
        V3 tmp = {0.0f, 0.0f, 1.0f};
        cardinalAxis = tmp;
    }

    result = CrossProductV3(v, cardinalAxis);

    return result;
}

struct Angle
{
    float theta;
    float perpendicularTest;
};

inline Angle GetAngleOfVectors(V2 v1, V2 v2)
{
    Angle result = {};

    float lengths = Length(v1) * Length(v2);
    if (lengths)
    {
        result.theta = DotProductV2(v1, v2) / lengths;
        result.perpendicularTest = DotProductV2(v1, PerpV2(v2));
    }

    return result;
}

inline float SnapToInterval(float f, float interval)
{
    float result = Round(f / interval) * interval;

    return result;
}

inline V2 SnapToInterval(V2 v, float interval)
{
    V2 result;

    result.x = SnapToInterval(v.x, interval);
    result.y = SnapToInterval(v.y, interval);

    return result;
}

inline V3 SnapToInterval(V3 v, float interval)
{
    V3 result;

    result.x = SnapToInterval(v.x, interval);
    result.y = SnapToInterval(v.y, interval);
    result.z = SnapToInterval(v.z, interval);

    return result;
}

inline float MostlyEquals(float f1, float f2, float epsilon)
{
    return f1 >= f2 - epsilon &&
           f1 <= f2 + epsilon;
}

inline bool IsInRect2D(V2 pos, Rect rect)
{
    bool result = ((pos.x >= rect.pos.x) &&
                   (pos.y >= rect.pos.y) &&
                   (pos.x <= (rect.pos.x + rect.dim.x)) &&
                   (pos.y <= (rect.pos.y + rect.dim.y)));

    return result;
}

inline void ModNext(int &i, int size)
{
    i = (i + 1) % size;
}

inline void ModNext(unsigned int &i, int size)
{
    i = (i + 1) % size;
}

inline void ModNext(volatile unsigned int &i, int size)
{
    i = (i + 1) % size;
}

inline void ModBack(int &i, int size)
{
    i = (size + (i - 1)) % size;
}

inline void ModBack(unsigned int &i, int size)
{
    i = (size + (i - 1)) % size;
}

inline float Min(float i, float j)
{
    float result = i;

    if (i > j)
        result = j;

    return result;
}

inline float Max(float i, float j)
{
    float result = i;

    if (i < j)
        result = j;

    return result;
}

#if 1
inline float Clamp(float min, float value, float max)
{
    float result = value;

    if (result < min)
        result = min;
    else if (result > max)
        result = max;

    return (result);
}
#endif

inline bool IsCollisionBox(BoxV2 box1, BoxV2 box2)
{
    bool result = !((box2.max.x <= box1.min.x) ||
                    (box2.min.x >= box1.max.x) ||
                    (box2.max.y <= box1.min.y) ||
                    (box2.min.y >= box1.max.y));
    return (result);
}

inline void FlipV2Y(V2 *v, float parentHeight)
{
    v->y = parentHeight - v->y;
}

inline void FlipRectY(Rect *rect, float parentHeight)
{
    rect->pos.y = parentHeight - rect->pos.y - rect->dim.y;
}

inline float DistanceV2(V2 v1, V2 v2)
{
    float result = 0.0f;

    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    result = SqrtFloat(dx * dx + dy * dy);

    return result;
}

inline float DistanceV3(V3 v1, V3 v2)
{
    float result = 0.0f;

    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;
    result = SqrtFloat(dx * dx + dy * dy + dz * dz);

    return result;
}

inline unsigned int IndexToFlag(int index)
{
    return 1 << index;
}

inline bool IsFlagSet(int index, unsigned int flags)
{
    return IndexToFlag(index) & flags;
}

inline V2 NormalizeV2(V2 v)
{
    V2 result = {};
    float length = SqrtFloat((v.x * v.x) + (v.y * v.y));

    if (length > 0)
    {
        result.x = v.x * 1.0f / length;
        result.y = v.y * 1.0f / length;
    }

    return result;
}

//TODO: use HMM optimization
inline V3 NormalizeV3(V3 v)
{
    V3 result = v;

    float length = SqrtFloat(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length == 0.0f)
        length = 1.0f;
    float ilength = 1.0f / length;

    result.x *= ilength;
    result.y *= ilength;
    result.z *= ilength;

    return result;
}

inline V4 NormalizeV4(V4 v)
{
    V4 result = v * InvSqrtFloat(DotProductV4(v, v));
    return result;
}

inline float Lerp(float start, float amount, float end)
{
    float result = (1.0 - amount) * start + amount * end;
    return result;
}

inline V2 Lerp(V2 start, float amount, V2 end)
{
    V2 result;

    result.x = Lerp(start.x, amount, end.x);
    result.y = Lerp(start.y, amount, end.y);

    return result;
}

inline V3 Lerp(V3 start, float amount, V3 end)
{
    V3 result;

    result.x = Lerp(start.x, amount, end.x);
    result.y = Lerp(start.y, amount, end.y);
    result.z = Lerp(start.z, amount, end.z);

    return result;
}

inline float Map(float value, float inputStart, float inputEnd, float outputStart, float outputEnd)
{
    float result = value;

    if (inputEnd - inputStart != 0)
        result = (value - inputStart) / (inputEnd - inputStart) * (outputEnd - outputStart) + outputStart;

    return result;
}

inline V2 Map(V2 value, V2 inputStart, V2 inputEnd, V2 outputStart, V2 outputEnd)
{
    V2 result;
    result.x = Map(value.x, inputStart.x, inputEnd.x, outputStart.x, outputEnd.x);
    result.y = Map(value.y, inputStart.y, inputEnd.y, outputStart.y, outputEnd.y);
    return result;
}

inline V3 Map(V3 value, V3 inputStart, V3 inputEnd, V3 outputStart, V3 outputEnd)
{
    V3 result;
    result.x = Map(value.x, inputStart.x, inputEnd.x, outputStart.x, outputEnd.x);
    result.y = Map(value.y, inputStart.y, inputEnd.y, outputStart.y, outputEnd.y);
    result.z = Map(value.z, inputStart.z, inputEnd.z, outputStart.z, outputEnd.z);
    return result;
}

inline float MapNormalize(float value, float start, float end)
{
    float result = 0;

    if (end - start)
        result = (value - start) / (end - start);

    return result;
}

inline void CenterV2(V2 *pos, V2 boxDim, V2 entityDim)
{
    *pos += (boxDim - entityDim) * 0.5f;
}

inline float GetTargetFrameMs(float framerate)
{
    Assert(framerate);
    float result = (1 / framerate) * 1000.0f;
    Assert(result);
    return result;
}

inline float FramesToMs(float framerate, float frames)
{
    float result = frames * GetTargetFrameMs(framerate);
    return result;
}

inline float MsToFrames(float framerate, float ms)
{
    float result = ms / GetTargetFrameMs(framerate);
    return result;
}

inline int MilisecondsToRoundedAnimationFrame(float framerate, float miliseconds)
{
    int result = Floor(MsToFrames(framerate, miliseconds));
    return result;
}

inline float SafeDivide(float a, float b)
{
    float result = {};

    if (b)
        result = a / b;

    return result;
}

inline uint32_t NextPowerOfTwo(uint32_t x)
{
    uint32_t result = 1;

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

inline float Square(float a)
{
    float Result = a * a;

    return (Result);
}

inline V4 SRGB255ToLinear1(V4 C)
{
    V4 Result;

    float Inv255 = 1.0f / 255.0f;

    Result.r = Square(Inv255 * C.r);
    Result.g = Square(Inv255 * C.g);
    Result.b = Square(Inv255 * C.b);
    Result.a = Inv255 * C.a;

    return (Result);
}

inline V4 Linear1ToSRGB255(V4 C)
{
    V4 Result;

    float One255 = 255.0f;

    Result.r = One255 * SqrtFloat(C.r);
    Result.g = One255 * SqrtFloat(C.g);
    Result.b = One255 * SqrtFloat(C.b);
    Result.a = One255 * C.a;

    return (Result);
}

inline float FontPointsToPixels(float points, float dpi)
{
    float result = points * (1.0 / 72.0) * dpi;
    return result;
}

inline V2 PositionInCenter(V2 parentDim, V2 childDim)
{
    V2 result = (parentDim * 0.5f) - (childDim * 0.5f);
    return result;
}

#define MURMUR_C1 0xcc9e2d51
#define MURMUR_C2 0x1b873593
#define MURMUR_R1 15
#define MURMUR_R2 13
#define MURMUR_M 5
#define MURMUR_N 0xe6546b64

inline uint32_t CombineHashes(uint32_t hash1, uint32_t hash2)
{
    hash1 ^= hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2);
    return hash1;
}

inline uint32_t Murmur3Int(uint32_t key, uint32_t seed, uint32_t prevHash = 0)
{
    uint32_t hash = seed;

    key *= MURMUR_C1;
    key = (key << MURMUR_R1) | (key >> (32 - MURMUR_R1));
    key *= MURMUR_C2;

    hash ^= key;
    hash = ((hash << MURMUR_R2) | (hash >> (32 - MURMUR_R2))) * MURMUR_M + MURMUR_N;

    if (prevHash)
        hash = CombineHashes(hash, prevHash);

    return hash;
}

inline uint32_t Murmur3Float(float key, uint32_t seed, uint32_t prevHash = 0)
{
    uint32_t int_key;
    memcpy(&int_key, &key, sizeof(uint32_t));
    uint32_t hash = Murmur3Int(int_key, seed, prevHash);
    return hash;
}

inline uint32_t Murmur3String(const char *key, uint32_t seed)
{
    uint32_t hash = seed;
    uint32_t len = strlen(key);
    int nblocks = len / 4;
    uint32_t *blocks = (uint32_t *)(key);

    for (int i = 0; i < nblocks; i++)
    {
        hash = Murmur3Int(blocks[i], hash);
    }

    const uint8_t *tail = (const uint8_t *)(key + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3)
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

    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);

    return hash;
}

// NOTE: LCG parameters 
static uint32_t G_VN_RANDOM_SEED = time(NULL); 
static const uint32_t G_VN_RAND_A = 1664525;
static const uint32_t G_VN_RAND_C = 1013904223;
static const uint32_t G_CN_RAND_M = UINT32_MAX;  // 2^32 - 1

inline uint32_t vn_rand() {
    G_VN_RANDOM_SEED = (G_VN_RAND_A * G_VN_RANDOM_SEED + G_VN_RAND_C) % G_CN_RAND_M;
    return G_VN_RANDOM_SEED;
}

inline unsigned int RandomInRangeInt(int min, int max)
{
    if (min > max)
    {
        int tmp = max;
        max = min;
        min = tmp;
    }

    return (vn_rand()%(abs(max - min) + 1) + min);
}

//-------------------------------------------------------------
//---------------------------Matrix-----------------------------
//-------------------------------------------------------------

//NOTE: disabled for raylib compatibility
#if 0
union Matrix
{
    float elements[4][4];
    V4 columns[4];
};

inline V4 LinearCombineV4Matrix(V4 v, Matrix matrix)
{
    V4 result;

    result.sse = _mm_mul_ps(_mm_shuffle_ps(v.sse, v.sse, 0x00), matrix.columns[0].sse);
    result.sse = _mm_add_ps(result.sse, _mm_mul_ps(_mm_shuffle_ps(v.sse, v.sse, 0x55), matrix.columns[1].sse));
    result.sse = _mm_add_ps(result.sse, _mm_mul_ps(_mm_shuffle_ps(v.sse, v.sse, 0xaa), matrix.columns[2].sse));
    result.sse = _mm_add_ps(result.sse, _mm_mul_ps(_mm_shuffle_ps(v.sse, v.sse, 0xff), matrix.columns[3].sse));

    return result;
}

inline Matrix operator*(Matrix a, Matrix b)
{
    Matrix result;

    result.columns[0] = LinearCombineV4Matrix(b.columns[0], a);
    result.columns[1] = LinearCombineV4Matrix(b.columns[1], a);
    result.columns[2] = LinearCombineV4Matrix(b.columns[2], a);
    result.columns[3] = LinearCombineV4Matrix(b.columns[3], a);

    return (result);
}

inline void operator*=(Matrix &a, Matrix b)
{
    a = a * b;
}

inline V2 LinearCombineV2Matrix(V2 v, Matrix matrix)
{
    V4 v4 = V4{v.x, v.y, 0, 1};
    v4 = LinearCombineV4Matrix(v4, matrix);
    V2 result = V2{v4.x, v4.y};

    return result;
}

inline V3 LinearCombineV3Matrix(V3 v, Matrix matrix)
{
    V4 v4 = V3ToV4_1(v);
    v4 = LinearCombineV4Matrix(v4, matrix);
    V3 result = V4ToV3(v4);

    return result;
}

inline V2 V2Transformed(V2 v, Matrix matrix)
{
    V2 result = LinearCombineV2Matrix(v, matrix);
    return result;
}

inline V3 V3Transformed(V3 v, Matrix matrix)
{
    V3 result = LinearCombineV3Matrix(v, matrix);
    return result;
}

inline V3 DehomogenizeV4(V4 v)
{
    V3 result;

    V4 dehomogenized;
    __m128 scalar = _mm_set1_ps(v.w);
    dehomogenized.sse = _mm_div_ps(v.sse, scalar);
    result = V4ToV3(dehomogenized);

    return result;
}

inline Matrix MatrixIdentity()
{
    Matrix result = {};

    result.elements[0][0] = 1;
    result.elements[1][1] = 1;
    result.elements[2][2] = 1;
    result.elements[3][3] = 1;

    return result;
}

inline Matrix MatrixTranslateV3(V3 translate)
{
    Matrix result = MatrixIdentity();

    result.elements[3][0] = translate.x;
    result.elements[3][1] = translate.y;
    result.elements[3][2] = translate.z;

    return result;
}

inline Matrix MatrixInverseTranslateV3(V3 translate)
{
    Matrix result = MatrixIdentity();

    result.elements[3][0] = -translate.x;
    result.elements[3][1] = -translate.y;
    result.elements[3][2] = -translate.z;

    return result;
}

inline Matrix MatrixScaleV2(V2 scale)
{
    Matrix result = MatrixIdentity();

    result.elements[0][0] = scale.x;
    result.elements[1][1] = scale.y;
    result.elements[2][2] = 1;

    return result;
}

inline Matrix MatrixScaleV3(V3 scale)
{
    Matrix result = MatrixIdentity();

    result.elements[0][0] = scale.x;
    result.elements[1][1] = scale.y;
    result.elements[2][2] = scale.z;

    return result;
}

inline Matrix MatrixTranspose(Matrix matrix)
{
    Matrix result = matrix;
    _MM_TRANSPOSE4_PS(result.columns[0].sse, result.columns[1].sse, result.columns[2].sse, result.columns[3].sse);
    return result;
}

//TODO: investigate cases where we can use a specialized invert function instead of a general one
inline Matrix MatrixInvert(Matrix m)
{
    Matrix result;
    V3 C01 = CrossProductV3(m.columns[0].xyz, m.columns[1].xyz);
    V3 C23 = CrossProductV3(m.columns[2].xyz, m.columns[3].xyz);
    V3 B10 = (m.columns[0].xyz * m.columns[1].w) - (m.columns[1].xyz * m.columns[0].w);
    V3 B32 = (m.columns[2].xyz * m.columns[3].w) - (m.columns[3].xyz * m.columns[2].w);

    float InvDeterminant = 1.0f / (DotProductV3(C01, B32) + DotProductV3(C23, B10));
    C01 = C01 * InvDeterminant;
    C23 = C23 * InvDeterminant;
    B10 = B10 * InvDeterminant;
    B32 = B32 * InvDeterminant;

    result.columns[0] = V3ToV4((CrossProductV3(m.columns[1].xyz, B32) + (C23 * m.columns[1].w)), -DotProductV3(m.columns[1].xyz, C23));
    result.columns[1] = V3ToV4((CrossProductV3(B32, m.columns[0].xyz) - (C23 * m.columns[0].w)), +DotProductV3(m.columns[0].xyz, C23));
    result.columns[2] = V3ToV4((CrossProductV3(m.columns[3].xyz, B10) + (C01 * m.columns[3].w)), -DotProductV3(m.columns[3].xyz, C01));
    result.columns[3] = V3ToV4((CrossProductV3(B10, m.columns[2].xyz) - (C01 * m.columns[2].w)), +DotProductV3(m.columns[2].xyz, C01));

    return MatrixTranspose(result);
}

//-------------------------------------------------------------
//-----------------------Quaternions---------------------------
//-------------------------------------------------------------

typedef union Quaternion
{
    struct
    {
        union
        {
            V3 xyz;
            struct
            {
                float x, y, z;
            };
        };

        float w;
    };

    float elements[4];

    __m128 sse;

} Quaternion;

inline Quaternion operator*(Quaternion q1, Quaternion q2)
{
    Quaternion result = {};

    __m128 SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(q1.sse, q1.sse, _MM_SHUFFLE(0, 0, 0, 0)), _mm_setr_ps(0.f, -0.f, 0.f, -0.f));
    __m128 SSEResultTwo = _mm_shuffle_ps(q2.sse, q2.sse, _MM_SHUFFLE(0, 1, 2, 3));
    __m128 SSEResultThree = _mm_mul_ps(SSEResultTwo, SSEResultOne);

    SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(q1.sse, q1.sse, _MM_SHUFFLE(1, 1, 1, 1)), _mm_setr_ps(0.f, 0.f, -0.f, -0.f));
    SSEResultTwo = _mm_shuffle_ps(q2.sse, q2.sse, _MM_SHUFFLE(1, 0, 3, 2));
    SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

    SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(q1.sse, q1.sse, _MM_SHUFFLE(2, 2, 2, 2)), _mm_setr_ps(-0.f, 0.f, 0.f, -0.f));
    SSEResultTwo = _mm_shuffle_ps(q2.sse, q2.sse, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

    SSEResultOne = _mm_shuffle_ps(q1.sse, q1.sse, _MM_SHUFFLE(3, 3, 3, 3));
    SSEResultTwo = _mm_shuffle_ps(q2.sse, q2.sse, _MM_SHUFFLE(3, 2, 1, 0));
    result.sse = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

    return result;
}

inline void operator*=(Quaternion &a, Quaternion b)
{
    a = a * b;
}

inline Quaternion operator/(Quaternion q, float f)
{
    Quaternion result;

    __m128 scalar = _mm_set1_ps(f);
    result.sse = _mm_div_ps(q.sse, scalar);

    return result;
}

inline bool operator==(Quaternion q1, Quaternion q2)
{
    bool result = q1.x == q2.x &&
                  q1.y == q2.y &&
                  q1.z == q2.z &&
                  q1.w == q2.w;

    return result;
}

inline bool operator!=(Quaternion q1, Quaternion q2)
{
    bool result = !(q1 == q2);
    return result;
}

inline Quaternion QuaternionIdentity()
{
    Quaternion result = Quaternion{0, 0, 0, 1};
    return result;
}

inline Quaternion QuaternionFromEuler(V3 axis, float angle)
{
    Quaternion result;

    V3 axisNormalized = NormalizeV3(axis);
    float sineOfRotation = sinf(angle / 2.0f);
    result.xyz = axisNormalized * sineOfRotation;
    result.w = cosf(angle / 2.0f);

    return result;
}

inline float DotProductQuaternion(Quaternion q1, Quaternion q2)
{
    float Result;

    __m128 SSEResultOne = _mm_mul_ps(q1.sse, q2.sse);
    __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    _mm_store_ss(&Result, SSEResultOne);

    return Result;
}

inline Quaternion QuaternionInvert(Quaternion q)
{
    Quaternion result;

    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;
    result = result / DotProductQuaternion(q, q);

    return result;
}

inline Quaternion GetQuaternionFromPoints(V2 v1, V2 v2, V2 origin, float snapAngleToInterval)
{
    Quaternion result = QuaternionIdentity();

    v1 = origin - v1;
    v2 = origin - v2;

    Angle angle = GetAngleOfVectors(v1, v2);
    if (angle.theta >= -1 && angle.theta <= 1)
    {
        float euler = acos(angle.theta);
        if (snapAngleToInterval)
            euler = SnapToInterval(euler, snapAngleToInterval);

        V3 axis = V3{0, 0, 1};
        result = QuaternionFromEuler(axis, euler);

        if (angle.perpendicularTest > 0)
            result = QuaternionInvert(result);
    }

    return result;
}

inline Quaternion NormalizeQuaternion(Quaternion q)
{
    V4 v = {q.x, q.y, q.z, q.w};
    v = NormalizeV4(v);
    Quaternion result = {v.x, v.y, v.z, v.w};

    return result;
}

inline Matrix QuaternionToMatrix(Quaternion q)
{
    Matrix Result;

    Quaternion normalizedQ = NormalizeQuaternion(q);

    float xx, yy, zz,
        xy, xz, yz,
        wx, wy, wz;

    xx = normalizedQ.x * normalizedQ.x;
    yy = normalizedQ.y * normalizedQ.y;
    zz = normalizedQ.z * normalizedQ.z;
    xy = normalizedQ.x * normalizedQ.y;
    xz = normalizedQ.x * normalizedQ.z;
    yz = normalizedQ.y * normalizedQ.z;
    wx = normalizedQ.w * normalizedQ.x;
    wy = normalizedQ.w * normalizedQ.y;
    wz = normalizedQ.w * normalizedQ.z;

    Result.elements[0][0] = 1.0f - 2.0f * (yy + zz);
    Result.elements[0][1] = 2.0f * (xy + wz);
    Result.elements[0][2] = 2.0f * (xz - wy);
    Result.elements[0][3] = 0.0f;

    Result.elements[1][0] = 2.0f * (xy - wz);
    Result.elements[1][1] = 1.0f - 2.0f * (xx + zz);
    Result.elements[1][2] = 2.0f * (yz + wx);
    Result.elements[1][3] = 0.0f;

    Result.elements[2][0] = 2.0f * (xz + wy);
    Result.elements[2][1] = 2.0f * (yz - wx);
    Result.elements[2][2] = 1.0f - 2.0f * (xx + yy);
    Result.elements[2][3] = 0.0f;

    Result.elements[3][0] = 0.0f;
    Result.elements[3][1] = 0.0f;
    Result.elements[3][2] = 0.0f;
    Result.elements[3][3] = 1.0f;

    return Result;
}

inline Quaternion _QuaternionMix(Quaternion q1, float mixLeft, Quaternion q2, float mixRight)
{
    Quaternion result;

    __m128 ScalarLeft = _mm_set1_ps(mixLeft);
    __m128 ScalarRight = _mm_set1_ps(mixRight);
    __m128 SSEResultOne = _mm_mul_ps(q1.sse, ScalarLeft);
    __m128 SSEResultTwo = _mm_mul_ps(q2.sse, ScalarRight);
    result.sse = _mm_add_ps(SSEResultOne, SSEResultTwo);

    return result;
}

inline Quaternion QuaternionNormalizedLerp(Quaternion q1, Quaternion q2, float time)
{
    Quaternion result = _QuaternionMix(q1, 1.0f - time, q2, time);
    result = NormalizeQuaternion(result);
    return result;
}

inline Quaternion QuaternionSlerp(Quaternion q1, Quaternion q2, float time)
{
    Quaternion result;

    float cosTheta = DotProductQuaternion(q1, q2);

    if (cosTheta < 0.0f)
    { /* NOTE(lcf): Take shortest path on Hyper-sphere */
        cosTheta = -cosTheta;
        q2 = Quaternion{-q2.x, -q2.y, -q2.z, -q2.w};
    }

    /* NOTE(lcf): Use Normalized Linear interpolation when vectors are roughly not L.I. */
    if (cosTheta > 0.9995f)
    {
        result = QuaternionNormalizedLerp(q1, q2, time);
    }
    else
    {
        float angle = cosf(cosTheta);
        float mixLeft = sinf((1.0f - time) * angle);
        float mixRight = sinf(time * angle);

        result = _QuaternionMix(q1, mixLeft, q2, mixRight);
        result = NormalizeQuaternion(result);
    }

    return result;
}
#endif
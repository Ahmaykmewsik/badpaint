#pragma once

#if __clang__
#include "headersNondependent.h"
#endif

inline Vector2 V2ToRayVector(V2 v)
{
    return Vector2{v.x, v.y};
}

inline V2 RayVectorToV2(Vector2 v)
{
    return V2{v.x, v.y};
}

inline V3 RayVectorToV3(Vector3 v)
{
    return V3{v.x, v.y, v.z};
}

inline Vector3 V3ToRayVector(V3 v)
{
    return Vector3{v.x, v.y, v.z};
}

inline Rectangle RectToRayRectangle(Rect r)
{
    return Rectangle{r.pos.x, r.pos.y, r.dim.x, r.dim.y};
}

inline Rect RayRectangleToRect(Rectangle r)
{
    return Rect{V2{r.x, r.y}, V2{r.width, r.height}};
}

inline V4 ColorNormalizeV4(Color color)
{
    V4 result;

    result.r = (float)color.r / 255.0f;
    result.g = (float)color.g / 255.0f;
    result.b = (float)color.b / 255.0f;
    result.a = (float)color.a / 255.0f;

    return result;
}

inline bool operator==(Color a, Color b)
{
    bool result = a.r == b.r &&
                  a.g == b.g &&
                  a.b == b.b &&
                  a.a == b.a;
    return result;
}

inline bool operator!=(Color a, Color b)
{
    bool result = !(a == b);
    return result;
}

inline V2 WidthHeightToV2(int width, int height)
{
    V2 result = V2{(float)width, (float)height};
    return result;
}

inline V2 GetTextureDim(Texture texture)
{
    V2 result = WidthHeightToV2(texture.width, texture.height) ;
    return result;
}
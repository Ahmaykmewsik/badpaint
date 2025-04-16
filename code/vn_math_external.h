#pragma once

inline Vector2 V2ToRayVector(v2 v)
{
	return Vector2{v.x, v.y};
}

inline v2 RayVectorToV2(Vector2 v)
{
	return v2{v.x, v.y};
}

inline v3 RayVectorToV3(Vector3 v)
{
	return v3{v.x, v.y, v.z};
}

inline Vector3 V3ToRayVector(v3 v)
{
	return Vector3{v.x, v.y, v.z};
}

inline Rectangle RectToRayRectangle(RectV2 r)
{
	return Rectangle{r.pos.x, r.pos.y, r.dim.x, r.dim.y};
}

inline RectV2 RayRectangleToRect(Rectangle r)
{
	return RectV2{v2{r.x, r.y}, v2{r.width, r.height}};
}

inline v4 ColorNormalizeV4(Color color)
{
	v4 result;

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

inline v2 WidthHeightToV2(int width, int height)
{
	v2 result = v2{(float)width, (float)height};
	return result;
}

inline v2 GetTextureDim(Texture texture)
{
	v2 result = WidthHeightToV2(texture.width, texture.height) ;
	return result;
}

inline Color RandomColor()
{
	Color result;
	result.r = (u8) RandomInRangeI32(0, 255);
	result.g = (u8) RandomInRangeI32(0, 255);
	result.b = (u8) RandomInRangeI32(0, 255);
	result.a = 255; 
	return result;
}

#pragma once
#include "LuaWrapper/LuaGlobalReflect.h"

#define IS_ZERO(x) (fabs(x) < FLT_EPSILON)
#define IS_NOT_ZERO(x) (fabs(x) >= FLT_EPSILON)
#define IS_EQUAL(a, b) (fabs((a) - (b)) < FLT_EPSILON)
#define IS_NOT_EQUAL(a, b) (fabs((a) - (b)) >= FLT_EPSILON)
#define SAFE_DIVIDE(a, b, c) (IS_NOT_ZERO(b) ? ((a) / (b)) : c)

struct CMatrix3D;

struct float3
{
	float3 Normalize() const;
	float Length() const;
	float3 operator + (const float3& v) const;
	float3 operator - (const float3& v) const;
	const float3& operator += (const float3& v);
	const float3& operator -= (const float3& v);
	float Dot(const float3& v) const;
	float3 Cross(const float3& v) const;
	float3 operator - () const;
	float3 operator + (float n) const;
	float3 operator - (float n) const;
	float3 operator * (float n) const;
	float3 operator / (float n) const;
	const float3& operator += (float n);
	const float3& operator -= (float n);
	const float3& operator *= (float n);
	const float3& operator /= (float n);
	bool operator == (const float3& v);
	bool operator != (const float3& v);
	float3 operator * (const CMatrix3D& m) const;
	float& operator [] (size_t i);
	float operator [] (size_t i) const;
	static float3 Vector(const float3& p0, const float3& p1, bool inv_y = false);

	float x = 0;
	float y = 0;
	float z = 0;
};

struct float4
{
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 1;
};

struct CMatrix3D
{
	CMatrix3D()
	{
		m[0] = { 1, 0, 0 };
		m[1] = { 0, 1, 0 };
		m[2] = { 0, 0, 1 };
		m[3] = { 0, 0, 0 };
	}

	void SetD0(float x, float y, float z)
	{
		m[0] = { x, y, z };
	}
	void SetD1(float x, float y, float z)
	{
		m[1] = { x, y, z };
	}
	void SetD2(float x, float y, float z)
	{
		m[2] = { x, y, z };
	}
	void SetD3(float x, float y, float z)
	{
		m[3] = { x, y, z };
	}
	const float3& operator [] (size_t n) const
	{
		return m[n];
	}

	float3& operator [] (size_t n)
	{
		return m[n];
	}

	float3 m[4];

	Lua_wrap_cpp_class(CMatrix3D, Lua_ctor(), Lua_mf(SetD0), Lua_mf(SetD1), Lua_mf(SetD2), Lua_mf(SetD3))
};
Lua_global_add_cpp_class(CMatrix3D)

inline float3 float3::Normalize() const
{
	float n = Length();
	if (IS_NOT_ZERO(n))
	{
		n = 1.0f / n;
		return *this * n;
	}
	return { 0, 0, 0 };
}
inline float float3::Length() const
{
	return sqrtf(x * x + y * y + z * z);
}

inline float3 float3::Vector(const float3& p0, const float3& p1, bool inv_y)
{
	return { p1.x - p0.x, inv_y ? p0.y - p1.y : p1.y - p0.y, p1.z - p0.z };
}

inline float3 float3::operator + (const float3& v) const
{
	return { x + v.x, y + v.y, z + v.z };
}
inline float3 float3::operator - (const float3& v) const
{
	return { x - v.x, y - v.y, z - v.z };
}
inline const float3& float3::operator += (const float3& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}
inline const float3& float3::operator -= (const float3& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}
inline float float3::Dot(const float3& v) const
{
	return x * v.x + y * v.y + z * v.z;
}
inline float3 float3::Cross(const float3& v) const
{
	float3 vv;
	vv.x = y * v.z - z * v.y;
	vv.y = z * v.x - x * v.z;
	vv.z = x * v.y - y * v.x;
	return vv;
}
inline float3 float3::operator - () const
{
	return { -x, -y, -z };
}
inline float3 float3::operator + (float n) const
{
	return { x + n, y + n, z + n };
}
inline float3 float3::operator - (float n) const
{
	return { x - n, y - n, z - n };
}
inline float3 float3::operator * (float n) const
{
	return { x * n, y * n, z * n };
}
inline float3 float3::operator / (float n) const
{
	n = 1.0f / n;
	return { x * n, y * n, z * n };
}
inline const float3& float3::operator += (float n)
{
	x += n;
	y += n;
	z += n;
	return *this;
}
inline const float3& float3::operator -= (float n)
{
	x -= n;
	y -= n;
	z -= n;
	return *this;
}
inline const float3& float3::operator *= (float n)
{
	x *= n;
	y *= n;
	z *= n;
	return *this;
}
inline const float3& float3::operator /= (float n)
{
	n = 1.0f / n;
	x *= n;
	y *= n;
	z *= n;
	return *this;
}
inline bool float3::operator == (const float3& v)
{
	return IS_EQUAL(x, v.x) && IS_EQUAL(y, v.y) && IS_EQUAL(z, v.z);
}
inline bool float3::operator != (const float3& v)
{
	return IS_NOT_EQUAL(x, v.x) || IS_NOT_EQUAL(y, v.y) || IS_NOT_EQUAL(z, v.z);
}

inline float3 float3::operator * (const CMatrix3D& m) const
{
	return { x * m[0].x + y * m[1].x + z * m[2].x + m[3].x,
			 x * m[0].y + y * m[1].y + z * m[2].y + m[3].y,
			 x * m[0].z + y * m[1].z + z * m[2].z + m[3].z };
}

inline float& float3::operator [] (size_t i)
{
	return *((float*)this + i);
}
inline float float3::operator [] (size_t i) const
{
	return *((float*)this + i);
}

inline float Cross2D(float x0, float y0, float x1, float y1)
{
	return x0 * y1 - y0 * x1;
}
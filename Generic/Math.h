#pragma once
#include "LuaWrapper/LuaGlobalReflect.h"

#define IS_ZERO(x) (fabs(x) < FLT_EPSILON)
#define NOT_ZERO(x) (fabs(x) >= FLT_EPSILON)
#define EQUAL(a, b) (fabs((a) - (b)) < FLT_EPSILON)
#define NOT_EQUAL(a, b) (fabs((a) - (b)) >= FLT_EPSILON)
#define SAFE_DIVIDE(a, b, c) (IS_NOT_ZERO(b) ? ((a) / (b)) : c)

struct CMatrix3D;

struct float2
{
	float x = 0;
	float y = 0;
};

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
	const float3& operator = (const std::tuple<float, float>& t);
	const float3& operator = (const std::tuple<float, float, float>& t);
	float3 operator - () const;
	float3 operator + (float n) const;
	float3 operator - (float n) const;
	float3 operator * (float n) const;
	float3 operator / (float n) const;
	const float3& operator += (float n);
	const float3& operator -= (float n);
	const float3& operator *= (float n);
	const float3& operator /= (float n);
	float3 operator * (const float3& v) const;
	bool operator == (const float3& v);
	bool operator != (const float3& v);
	float3 operator * (const CMatrix3D& m) const;
	float& operator [] (size_t i);
	float operator [] (size_t i) const;
	bool Same(const float3& v);
	static float3 Vector(const float3& p0, const float3& p1);

	float x = 0;
	float y = 0;
	float z = 0;
};

struct float4
{
	const float4& operator = (const float3& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}
	operator float3& ()
	{
		return *(float3*)this;
	}
	operator const float3& () const
	{
		return *(float3*)this;
	}
	float& operator [] (size_t i)
	{
		return *((float*)this + i);
	}
	float operator [] (size_t i) const
	{
		return *((float*)this + i);
	}
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 1;
};

struct CMatrix3D
{
	CMatrix3D();

	void Identity();
	void SetVecX(float x, float y, float z, float w);
	void SetVecY(float x, float y, float z, float w);
	void SetVecZ(float x, float y, float z, float w);
	void SetVecW(float x, float y, float z, float w);
	std::tuple<float, float, float, float> GetVecX();
	std::tuple<float, float, float, float> GetVecY();
	std::tuple<float, float, float, float> GetVecZ();
	std::tuple<float, float, float, float> GetVecW();
	void SetRotation(float x, float y, float z, float angle);
	void SetPosition(float x, float y, float z);
	void Rotate(float x, float y, float z, float angle, bool rotatePos);
	void Move(float x, float y, float z);
	void Transform(LuacObj<CMatrix3D> M, size_t d);
	void Tranpose(size_t d);
	void Projection();
	void Copy();
	void SetMultiplied(LuacObj<CMatrix3D> M1, LuacObj<CMatrix3D> M2);
	const CMatrix3D& operator *= (const CMatrix3D& n);
	const float4& operator [] (size_t n) const;
	float4& operator [] (size_t n);

	float4 m[4];

	Lua_wrap_cpp_class(CMatrix3D, Lua_ctor(), Lua_mf(Identity), Lua_mf(SetVecX), Lua_mf(SetVecY), Lua_mf(SetVecZ), Lua_mf(SetVecW),
		Lua_mf(GetVecX), Lua_mf(GetVecY), Lua_mf(GetVecZ), Lua_mf(GetVecW), 
		Lua_mf(SetRotation), Lua_mf(SetPosition), Lua_mf(Rotate), Lua_mf(Move), 
		Lua_mf(Transform))
};
Lua_global_add_cpp_class(CMatrix3D)

inline float3 float3::Normalize() const
{
	float n = Length();
	if (NOT_ZERO(n))
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

inline float3 float3::Vector(const float3& p0, const float3& p1)
{
	return { p1.x - p0.x, p1.y - p0.y, p1.z - p0.z };
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
inline const float3& float3::operator = (const std::tuple<float, float>& t)
{
	x = std::get<0>(t);
	y = std::get<1>(t);
	return *this;
}
inline const float3& float3::operator = (const std::tuple<float, float, float>& t)
{
	x = std::get<0>(t);
	y = std::get<1>(t);
	z = std::get<2>(t);
	return *this;
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
inline float3 float3::operator * (const float3& v) const
{
	return { x * v.x, y * v.y, z * v.z };
}
inline bool float3::operator == (const float3& v)
{
	return EQUAL(x, v.x) && EQUAL(y, v.y) && EQUAL(z, v.z);
}
inline bool float3::operator != (const float3& v)
{
	return NOT_EQUAL(x, v.x) || NOT_EQUAL(y, v.y) || NOT_EQUAL(z, v.z);
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

inline bool float3::Same(const float3& v)
{
	return x == v.x && y && v.y && z == v.z;
}

inline float Cross2D(float x0, float y0, float x1, float y1)
{
	return x0 * y1 - y0 * x1;
}

inline CMatrix3D::CMatrix3D()
{
	Identity();
}

inline void CMatrix3D::Identity()
{
	m[0] = { 1, 0, 0 };
	m[1] = { 0, 1, 0 };
	m[2] = { 0, 0, 1 };
	m[3] = { 0, 0, 0 };
}

inline void CMatrix3D::SetVecX(float x, float y, float z, float w)
{
	m[0] = { x, y, z, w };
}
inline void CMatrix3D::SetVecY(float x, float y, float z, float w)
{
	m[1] = { x, y, z, w };
}
inline void CMatrix3D::SetVecZ(float x, float y, float z, float w)
{
	m[2] = { x, y, z, w };
}
inline void CMatrix3D::SetVecW(float x, float y, float z, float w)
{
	m[3] = { x, y, z, w };
}
inline std::tuple<float, float, float, float> CMatrix3D::GetVecX()
{
	return { m[0].x, m[0].y, m[0].z, m[0].w };
}
inline std::tuple<float, float, float, float> CMatrix3D::GetVecY()
{
	return { m[1].x, m[1].y, m[1].z, m[1].w };
}
inline std::tuple<float, float, float, float> CMatrix3D::GetVecZ()
{
	return { m[2].x, m[2].y, m[2].z, m[2].w };
}
inline std::tuple<float, float, float, float> CMatrix3D::GetVecW()
{
	return { m[3].x, m[3].y, m[3].z, m[3].w };
}
inline void CMatrix3D::SetRotation(float x, float y, float z, float angle)
{
	if (angle == 0)
		return;
	m[0] = { 1, 0, 0, m[0].w };
	m[1] = { 0, 1, 0, m[1].w };
	m[2] = { 0, 0, 1, m[2].w };
	m[3] = { 0, 0, 0, m[3].w };
	if (x == 0)
	{
		if (y == 0)
		{
			if (z == 0)
				return;
			float s = sinf(angle);
			float c = cosf(angle);
			m[0][0] = c;
			m[0][1] = s;
			m[1][0] = -s;
			m[1][1] = c;
			return;
		}
		else if (z == 0)
		{
			float s = sinf(angle);
			float c = cosf(angle);
			m[0][0] = c;
			m[0][2] = -s;
			m[2][0] = s;
			m[2][2] = c;
			return;
		}
	}
	else if (y == 0 && z == 0)
	{
		float s = sinf(angle);
		float c = cosf(angle);
		m[1][1] = c;
		m[1][2] = s;
		m[2][1] = -s;
		m[2][2] = c;
		return;
	}

	float d = x * x + y * y + z * z;
	if (d == 0)
		return;
	d = 1 / sqrtf(d);
	float3 n = { x * d, y * d, z * d };

	float s = sinf(angle);
	float c = cosf(angle);

	float3 a = { s, c, 1 - c };
	float3 c2 = { a.z, a.z, a.z };
	float3 c1 = { a.y, a.y, a.y };
	float3 c0 = { a.x, a.x, a.x };

	float3 n0 = { n.y, n.z, n.x };
	float3 n1 = { n.z, n.x, n.y };

	float3 v0 = c2 * n0 * n1;
	float3 r0 = c2 * n * n + c1;
	float3 r1 = c0 * n + v0;
	float3 r2 = -c0 * n + v0;
	v0 = r0;
	float3 v1 = { r1.z, r2.y, r2.z };
	float3 v2 = { r1.y, r2.x, r1.y };

	m[0] = { v0.x, v1.x, v1.y };
	m[1] = { v1.z, v0.y, r1.x };
	m[2] = { v2.x, v2.y, v0.z };
}
inline void CMatrix3D::SetPosition(float x, float y, float z)
{
	float3& v = m[3];
	v = { x, y, z };
}
inline void CMatrix3D::Rotate(float x, float y, float z, float angle, bool rotatePos)
{
	CMatrix3D M;
	M.SetRotation(x, y, z, angle);
	Transform(const_cast<CMatrix3D*>(&M), rotatePos ? 4 : 3);
}
inline void CMatrix3D::Move(float x, float y, float z)
{
	float3& v = m[3];
	v += {x, y, z};
}
inline void CMatrix3D::Transform(LuacObj<CMatrix3D> M, size_t d)
{
	CMatrix3D& n = *M;
	d = d % 5;
	for (size_t i = 0; i < d; i++)
		for (size_t j = 0; j < d; j++)
		{
			float v = 0;
			for (size_t k = 0; k < d; k++)
				v += m[i][j] * n[k][j];
			m[i][j] = v;
		}
}
inline void CMatrix3D::Projection()
{

}
inline const CMatrix3D& CMatrix3D::operator *= (const CMatrix3D& n)
{
	Transform(const_cast<CMatrix3D*>(&n), 4);
	return *this;
}
inline const float4& CMatrix3D::operator [] (size_t n) const
{
	return m[n];
}

inline float4& CMatrix3D::operator [] (size_t n)
{
	return m[n];
}
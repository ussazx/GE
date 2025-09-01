#pragma once
#include "LuaWrapper/LuaGlobalReflect.h"

#define IS_ZERO(x) (fabs(x) < FLT_EPSILON)
#define NOT_ZERO(x) (fabs(x) >= FLT_EPSILON)
#define EQUAL(a, b) (fabs((a) - (b)) < FLT_EPSILON)
#define NOT_EQUAL(a, b) (fabs((a) - (b)) >= FLT_EPSILON)
#define SAFE_DIVIDE(a, b, c) (IS_NOT_ZERO(b) ? ((a) / (b)) : c)
#define M_PI 3.141592654f

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
	const float3& operator = (const std::tuple<float, float, float, float>& t);
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
	const float3& operator *= (const CMatrix3D& m);
	float3 MulVec3(const CMatrix3D& m) const;
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
		return *this;
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
	float w = 0;
};

struct CMatrix3D
{
	CMatrix3D();

	void Identity();
	void SetRow1(float x, float y, float z, float w);
	void SetRow2(float x, float y, float z, float w);
	void SetRow3(float x, float y, float z, float w);
	void SetRow4(float x, float y, float z, float w);
	std::tuple<float, float, float, float> GetRow1();
	std::tuple<float, float, float, float> GetRow2();
	std::tuple<float, float, float, float> GetRow3();
	std::tuple<float, float, float, float> GetRow4();
	void SetRotation(float x, float y, float z, float angle);
	void SetPosition(float x, float y, float z);
	void Rotate(float x, float y, float z, float angle, bool rotatePos);
	void RotateLocalX(float angle, bool rotatePos);
	void RotateLocalY(float angle, bool rotatePos);
	void RotateLocalZ(float angle, bool rotatePos);
	void Move(float x, float y, float z);
	void Transform(LuacObj<CMatrix3D> M, size_t d = 4);
	void Perspective(float fov, float width, float height, float nearZ, float farZ);
	void Copy();
	void SetByTransposed(LuacObj<CMatrix3D> M, size_t d = 4);
	void SetByMultiplied(LuacObj<CMatrix3D> M1, LuacObj<CMatrix3D> M2);
	const CMatrix3D& operator *= (const CMatrix3D& n);

	struct Row
	{
		Row(float* f) : p(f) {}
		Row(const float* f) : p(const_cast<float*>(f)) {}
		float& operator [] (size_t n)
		{
			return p[n * 4];
		}
		const float& operator [] (size_t n) const
		{
			return p[n * 4];
		}
		float* p;
	};
	Row operator [] (size_t n)
	{
		return Row(m + n);
	}
	Row operator [] (size_t n) const
	{
		return Row(m + n);
	}

	union
	{
		float m[16];
		float4 col[4];
	};

	Lua_wrap_cpp_class(CMatrix3D, Lua_ctor(), Lua_mf(Identity), Lua_mf(SetRow1), Lua_mf(SetRow2), Lua_mf(SetRow3), Lua_mf(SetRow4),
		Lua_mf(GetRow1), Lua_mf(GetRow2), Lua_mf(GetRow3), Lua_mf(GetRow4), 
		Lua_mf(SetRotation), Lua_mf(SetPosition), Lua_mf(Move), 
		Lua_mf(Rotate), Lua_mf(RotateLocalX), Lua_mf(RotateLocalY), Lua_mf(RotateLocalZ),
		Lua_mf(Transform), Lua_mf(SetByTransposed), Lua_mf(SetByMultiplied), Lua_mf(Perspective))
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
inline const float3& float3::operator = (const std::tuple<float, float, float, float>& t)
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
	return { x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0],
			 x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1],
			 x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2] };
}
inline const float3& float3::operator *= (const CMatrix3D& m)
{
	*this = *this * m;
	return *this;
}
inline float3 float3::MulVec3(const CMatrix3D& m) const
{
	return { x * m[0][0] + y * m[1][0] + z * m[2][0],
			 x * m[0][1] + y * m[1][1] + z * m[2][1],
			 x * m[0][2] + y * m[1][2] + z * m[2][2] };
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
	col[0] = { 1, 0, 0 };
	col[1] = { 0, 1, 0 };
	col[2] = { 0, 0, 1 };
	col[3] = { 0, 0, 0, 1 };
}

inline void CMatrix3D::SetRow1(float x, float y, float z, float w)
{
	CMatrix3D& M = *this;
	M[0][0] = x;
	M[0][1] = y;
	M[0][2] = z;
	M[0][3] = w;
}
inline void CMatrix3D::SetRow2(float x, float y, float z, float w)
{
	CMatrix3D& M = *this;
	M[1][0] = x;
	M[1][1] = y;
	M[1][2] = z;
	M[1][3] = w;
}
inline void CMatrix3D::SetRow3(float x, float y, float z, float w)
{
	CMatrix3D& M = *this;
	M[2][0] = x;
	M[2][1] = y;
	M[2][2] = z;
	M[2][3] = w;
}
inline void CMatrix3D::SetRow4(float x, float y, float z, float w)
{
	CMatrix3D& M = *this;
	M[3][0] = x;
	M[3][1] = y;
	M[3][2] = z;
	M[3][3] = w;
}
inline std::tuple<float, float, float, float> CMatrix3D::GetRow1()
{
	CMatrix3D& M = *this;
	return { M[0][0], M[0][1], M[0][2], M[0][3] };
}
inline std::tuple<float, float, float, float> CMatrix3D::GetRow2()
{
	CMatrix3D& M = *this;
	return { M[1][0], M[1][1], M[1][2], M[1][3] };
}
inline std::tuple<float, float, float, float> CMatrix3D::GetRow3()
{
	CMatrix3D& M = *this;
	return { M[2][0], M[2][1], M[2][2], M[2][3] };
}
inline std::tuple<float, float, float, float> CMatrix3D::GetRow4()
{
	CMatrix3D& M = *this;
	return { M[3][0], M[3][1], M[3][2], M[3][3] };
}
inline void CMatrix3D::SetRotation(float x, float y, float z, float angle)
{
	CMatrix3D& M = *this;
	if (angle == 0)
		return;
	angle *= M_PI / 180.0f;
	if (x == 0)
	{
		if (y == 0)
		{
			if (z == 0)
				return;
			float s = sinf(angle);
			float c = cosf(angle);
			M[0][0] = c;
			M[0][1] = s;
			M[1][0] = -s;
			M[1][1] = c;
			return;
		}
		else if (z == 0)
		{
			float s = sinf(angle);
			float c = cosf(angle);
			M[0][0] = c;
			M[0][2] = -s;
			M[2][0] = s;
			M[2][2] = c;
			return;
		}
	}
	else if (y == 0 && z == 0)
	{
		float s = sinf(angle);
		float c = cosf(angle);
		M[1][1] = c;
		M[1][2] = s;
		M[2][1] = -s;
		M[2][2] = c;
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

	M.SetRow1(v0.x, v1.x, v1.y, M[0][3]);
	M.SetRow2(v1.z, v0.y, r1.x, M[1][3]);
	M.SetRow3(v2.x, v2.y, v0.z, M[2][3]);
}
inline void CMatrix3D::SetPosition(float x, float y, float z)
{
	CMatrix3D& M = *this;
	M.SetRow4(x, y, z, M[3][3]);
}
inline void CMatrix3D::Rotate(float x, float y, float z, float angle, bool rotatePos)
{
	CMatrix3D M;
	M.SetRotation(x, y, z, angle);
	Transform(const_cast<CMatrix3D*>(&M), rotatePos ? 4 : 3);
}
inline void CMatrix3D::RotateLocalX(float angle, bool rotatePos)
{
	CMatrix3D M;
	float3 v{};
	v = GetRow1();
	M.SetRotation(v.x, v.y, v.z, angle);
	Transform(const_cast<CMatrix3D*>(&M), rotatePos ? 4 : 3);
}
inline void CMatrix3D::RotateLocalY(float angle, bool rotatePos)
{
	CMatrix3D M;
	float3 v{};
	v = GetRow2();
	M.SetRotation(v.x, v.y, v.z, angle);
	Transform(const_cast<CMatrix3D*>(&M), rotatePos ? 4 : 3);
}
inline void CMatrix3D::RotateLocalZ(float angle, bool rotatePos)
{
	CMatrix3D M;
	float3 v{};
	v = GetRow3();
	M.SetRotation(v.x, v.y, v.z, angle);
	Transform(const_cast<CMatrix3D*>(&M), rotatePos ? 4 : 3);
}
inline void CMatrix3D::Move(float x, float y, float z)
{
	CMatrix3D& M = *this;
	M.SetRow4(M[3][0] + x, M[3][1] + y, M[3][2] + z, M[3][3]);
}
inline void CMatrix3D::Transform(LuacObj<CMatrix3D> N, size_t d)
{
	d %= 5;
	CMatrix3D& M = *this;
	CMatrix3D& n = *N;
	float4 v;
	for (size_t i = 0; i < d; i++)
	{
		v = {};
		for (size_t j = 0; j < d; j++)
			for (size_t k = 0; k < d; k++)
				v[j] += M[i][k] * n[k][j];
		for (size_t j = 0; j < d; j++)
			M[i][j] = v[j];
	}
}
inline void CMatrix3D::SetByTransposed(LuacObj<CMatrix3D> N, size_t d)
{
	CMatrix3D& M = *this;
	d %= 5;
	CMatrix3D& n = *N;
	for (size_t i = 0; i < d; i++)
		for (size_t j = 0; j < d; j++)
			M[i][j] = n[j][i];
}
inline void CMatrix3D::Perspective(float fovD, float width, float height, float nearZ, float farZ)
{
	CMatrix3D& M = *this;
	if (fovD == 0 || width == 0 || height == 0 || nearZ == farZ)
		return;
	float h = 1 / tanf(fovD * M_PI / 180.0f * 0.5f);
	float aspect = width / height;
	float w = h / aspect;
	M.SetRow1(w, 0, 0, 0);
	M.SetRow2(0, -h, 0, 0);
	M.SetRow3(0, 0, farZ / (farZ - nearZ), 1);
	M.SetRow4(0, 0, -M[2][2] * nearZ, 0);
}
inline void CMatrix3D::SetByMultiplied(LuacObj<CMatrix3D> M1, LuacObj<CMatrix3D> M2)
{
	*this = *M1;
	*this *= *M2;
}
inline const CMatrix3D& CMatrix3D::operator *= (const CMatrix3D& n)
{
	Transform(const_cast<CMatrix3D*>(&n), 4);
	return *this;
}
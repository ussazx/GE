#pragma once
#include "LuaWrapper/LuaGlobalReflect.h"

#define IS_ZERO(x) (fabs(x) < FLT_EPSILON)
#define NOT_ZERO(x) (fabs(x) >= FLT_EPSILON)
#define EQUAL(a, b) (fabs((a) - (b)) < FLT_EPSILON)
#define NOT_EQUAL(a, b) (fabs((a) - (b)) >= FLT_EPSILON)
#define SAFE_DIVIDE(a, b, c) (IS_NOT_ZERO(b) ? ((a) / (b)) : c)
#define M_PI 3.141592654f

struct CMatrix;

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
	float3 operator * (const CMatrix& m) const;
	const float3& operator *= (const CMatrix& m);
	float3 MulVec3(const CMatrix& m) const;
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

struct CMatrix
{
	CMatrix();

	void Identity();
	void SetRow1(float x, float y, float z, float w);
	void SetRow2(float x, float y, float z, float w);
	void SetRow3(float x, float y, float z, float w);
	void SetRow4(float x, float y, float z, float w);
	void SetDiagonalTL(float m00, float m11, float m22, float m33);
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
	void Scale(float x, float y, float z);
	void Move(float x, float y, float z);
	void MoveLocal(float x, float y, float z);
	void Transform(LuacObj<CMatrix> M, size_t d = 4);
	void Perspective(float fov, float width, float height, float nearZ, float farZ);
	void CopyFrom(LuacObj<CMatrix> M);
	void SetByScaled(float x, float z, float y, LuacObj<CMatrix> M);
	void SetByTransposed(LuacObj<CMatrix> M, size_t d = 4);
	void SetByMultiplied(LuacObj<CMatrix> M1, LuacObj<CMatrix> M2);
	void SetByInverted(LuacObj<CMatrix> M);
	void SetByMatrixToView(LuacObj<CMatrix> M);
	void TransformByInverted(LuacObj<CMatrix> M);
	std::tuple<float, float, float> PointTransform(float x, float y, float z);
	std::tuple<float, float, float> VectorTransform(float x, float y, float z);
	std::tuple<float, float, float> GetPosition();
	const CMatrix& operator *= (const CMatrix& n);

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

	Lua_wrap_cpp_class(CMatrix, Lua_ctor(), Lua_mf(Identity), Lua_mf(SetRow1), Lua_mf(SetRow2), Lua_mf(SetRow3), Lua_mf(SetRow4),
		Lua_mf(GetRow1), Lua_mf(GetRow2), Lua_mf(GetRow3), Lua_mf(GetRow4),
		Lua_mf(SetRotation), Lua_mf(SetPosition), Lua_mf(Move), Lua_mf(MoveLocal),
		Lua_mf(Rotate), Lua_mf(RotateLocalX), Lua_mf(RotateLocalY), Lua_mf(RotateLocalZ), Lua_mf(Scale),
		Lua_mf(PointTransform), Lua_mf(VectorTransform),
		Lua_mf(Transform), Lua_mf(SetByScaled), Lua_mf(SetByTransposed), Lua_mf(SetByMultiplied), Lua_mf(TransformByInverted),
		Lua_mf(SetByInverted), Lua_mf(SetByMatrixToView), Lua_mf(Perspective), Lua_mf(CopyFrom), Lua_mf(GetPosition), Lua_mf(SetDiagonalTL))
};
Lua_global_add_cpp_class(CMatrix)

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

inline float3 float3::operator * (const CMatrix& m) const
{
	return { x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0],
			 x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1],
			 x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2] };
}
inline const float3& float3::operator *= (const CMatrix& m)
{
	*this = *this * m;
	return *this;
}
inline float3 float3::MulVec3(const CMatrix& m) const
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
	return x == v.x && y == v.y && z == v.z;
}

inline float Vec2Cross(float x0, float y0, float x1, float y1)
{
	return x0 * y1 - y0 * x1;
}

inline CMatrix::CMatrix()
{
	Identity();
}

inline void CMatrix::Identity()
{
	col[0] = { 1, 0, 0 };
	col[1] = { 0, 1, 0 };
	col[2] = { 0, 0, 1 };
	col[3] = { 0, 0, 0, 1 };
}

inline void CMatrix::SetRow1(float x, float y, float z, float w)
{
	CMatrix& M = *this;
	M[0][0] = x;
	M[0][1] = y;
	M[0][2] = z;
	M[0][3] = w;
}
inline void CMatrix::SetRow2(float x, float y, float z, float w)
{
	CMatrix& M = *this;
	M[1][0] = x;
	M[1][1] = y;
	M[1][2] = z;
	M[1][3] = w;
}
inline void CMatrix::SetRow3(float x, float y, float z, float w)
{
	CMatrix& M = *this;
	M[2][0] = x;
	M[2][1] = y;
	M[2][2] = z;
	M[2][3] = w;
}
inline void CMatrix::SetRow4(float x, float y, float z, float w)
{
	CMatrix& M = *this;
	M[3][0] = x;
	M[3][1] = y;
	M[3][2] = z;
	M[3][3] = w;
}
inline void CMatrix::SetDiagonalTL(float m00, float m11, float m22, float m33)
{
	CMatrix& M = *this;
	M[0][0] = m00;
	M[1][1] = m11;
	M[2][2] = m22;
	M[3][3] = m33;
}
inline std::tuple<float, float, float, float> CMatrix::GetRow1()
{
	CMatrix& M = *this;
	return { M[0][0], M[0][1], M[0][2], M[0][3] };
}
inline std::tuple<float, float, float, float> CMatrix::GetRow2()
{
	CMatrix& M = *this;
	return { M[1][0], M[1][1], M[1][2], M[1][3] };
}
inline std::tuple<float, float, float, float> CMatrix::GetRow3()
{
	CMatrix& M = *this;
	return { M[2][0], M[2][1], M[2][2], M[2][3] };
}
inline std::tuple<float, float, float, float> CMatrix::GetRow4()
{
	CMatrix& M = *this;
	return { M[3][0], M[3][1], M[3][2], M[3][3] };
}
inline void CMatrix::SetRotation(float x, float y, float z, float angle)
{
	CMatrix& M = *this;
	M.Identity();
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
inline void CMatrix::SetPosition(float x, float y, float z)
{
	CMatrix& M = *this;
	M.SetRow4(x, y, z, M[3][3]);
}
inline std::tuple<float, float, float> CMatrix::GetPosition()
{
	CMatrix& M = *this;
	return { M[3][0], M[3][1], M[3][2] };
}
inline void CMatrix::Rotate(float x, float y, float z, float angle, bool rotatePos)
{
	CMatrix M;
	M.SetRotation(x, y, z, angle);
	Transform(const_cast<CMatrix*>(&M), rotatePos ? 4 : 3);
}
inline void CMatrix::RotateLocalX(float angle, bool rotatePos)
{
	CMatrix M;
	float3 v{};
	v = GetRow1();
	M.SetRotation(v.x, v.y, v.z, angle);
	Transform(const_cast<CMatrix*>(&M), rotatePos ? 4 : 3);
}
inline void CMatrix::RotateLocalY(float angle, bool rotatePos)
{
	CMatrix M;
	float3 v{};
	v = GetRow2();
	M.SetRotation(v.x, v.y, v.z, angle);
	Transform(const_cast<CMatrix*>(&M), rotatePos ? 4 : 3);
}
inline void CMatrix::RotateLocalZ(float angle, bool rotatePos)
{
	CMatrix M;
	float3 v{};
	v = GetRow3();
	M.SetRotation(v.x, v.y, v.z, angle);
	Transform(const_cast<CMatrix*>(&M), rotatePos ? 4 : 3);
}
inline void CMatrix::Move(float x, float y, float z)
{
	CMatrix& M = *this;
	M.SetRow4(M[3][0] + x, M[3][1] + y, M[3][2] + z, M[3][3]);
}
inline void CMatrix::MoveLocal(float x, float y, float z)
{
	float3 v1{}, v2{}, v3{}, v4{};
	CMatrix& M = *this;
	v1 = M.GetRow1();
	v2 = M.GetRow2();
	v3 = M.GetRow3();
	v4 = M.GetRow4();
	v4 += v1 * x + v2 * y + v3 * z;
	M.SetRow4(v4.x, v4.y, v4.z, M[3][3]);
}
inline void CMatrix::Scale(float x, float y, float z)
{
	float3 v1{}, v2{}, v3{};
	CMatrix& M = *this;
	v1 = M.GetRow1();
	v2 = M.GetRow2();
	v3 = M.GetRow3();
	v1 *= x;
	v2 *= y;
	v3 *= z;
	M.SetRow1(v1.x, v1.y, v1.z, M[0][3]);
	M.SetRow2(v2.x, v2.y, v2.z, M[1][3]);
	M.SetRow3(v3.x, v3.y, v3.z, M[2][3]);
}
inline void CMatrix::SetByScaled(float x, float z, float y, LuacObj<CMatrix> m)
{
	float3 v1{}, v2{}, v3{};
	CMatrix& M = *m;
	v1 = M.GetRow1();
	v2 = M.GetRow2();
	v3 = M.GetRow3();
	v1 *= x;
	v2 *= y;
	v3 *= z;
	SetRow1(v1.x, v1.y, v1.z, M[0][3]);
	SetRow2(v2.x, v2.y, v2.z, M[1][3]);
	SetRow3(v3.x, v3.y, v3.z, M[2][3]);
	SetRow4(M[3][0], M[3][1], M[3][2], M[3][3]);
}
inline void CMatrix::Transform(LuacObj<CMatrix> N, size_t d)
{
	d %= 5;
	CMatrix& M = *this;
	CMatrix& n = *N;
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
inline void CMatrix::SetByTransposed(LuacObj<CMatrix> N, size_t d)
{
	CMatrix& M = *this;
	d %= 5;
	CMatrix& n = *N;
	for (size_t i = 0; i < d; i++)
		for (size_t j = 0; j < d; j++)
			M[i][j] = n[j][i];
}
inline void CMatrix::SetByMultiplied(LuacObj<CMatrix> M1, LuacObj<CMatrix> M2)
{
	if (this == M2)
	{
		CMatrix m = *M2;
		*this = *M1;
		*this *= m;
	}
	else
	{
		*this = *M1;
		*this *= *M2;
	}
}
inline void CMatrix::SetByInverted(LuacObj<CMatrix> n)
{
	CMatrix N = *n;
	CMatrix& M = *this;
	float s[6];
	float c[6];
	s[0] = N[0][0] * N[1][1] - N[1][0] * N[0][1];
	s[1] = N[0][0] * N[1][2] - N[1][0] * N[0][2];
	s[2] = N[0][0] * N[1][3] - N[1][0] * N[0][3];
	s[3] = N[0][1] * N[1][2] - N[1][1] * N[0][2];
	s[4] = N[0][1] * N[1][3] - N[1][1] * N[0][3];
	s[5] = N[0][2] * N[1][3] - N[1][2] * N[0][3];

	c[0] = N[2][0] * N[3][1] - N[3][0] * N[2][1];
	c[1] = N[2][0] * N[3][2] - N[3][0] * N[2][2];
	c[2] = N[2][0] * N[3][3] - N[3][0] * N[2][3];
	c[3] = N[2][1] * N[3][2] - N[3][1] * N[2][2];
	c[4] = N[2][1] * N[3][3] - N[3][1] * N[2][3];
	c[5] = N[2][2] * N[3][3] - N[3][2] * N[2][3];

	/* Assumes it is invertible */
	float idet = 1.0f / (s[0] * c[5] - s[1] * c[4] + s[2] * c[3] + s[3] * c[2] - s[4] * c[1] + s[5] * c[0]);

	M[0][0] = (N[1][1] * c[5] - N[1][2] * c[4] + N[1][3] * c[3]) * idet;
	M[0][1] = (-N[0][1] * c[5] + N[0][2] * c[4] - N[0][3] * c[3]) * idet;
	M[0][2] = (N[3][1] * s[5] - N[3][2] * s[4] + N[3][3] * s[3]) * idet;
	M[0][3] = (-N[2][1] * s[5] + N[2][2] * s[4] - N[2][3] * s[3]) * idet;

	M[1][0] = (-N[1][0] * c[5] + N[1][2] * c[2] - N[1][3] * c[1]) * idet;
	M[1][1] = (N[0][0] * c[5] - N[0][2] * c[2] + N[0][3] * c[1]) * idet;
	M[1][2] = (-N[3][0] * s[5] + N[3][2] * s[2] - N[3][3] * s[1]) * idet;
	M[1][3] = (N[2][0] * s[5] - N[2][2] * s[2] + N[2][3] * s[1]) * idet;

	M[2][0] = (N[1][0] * c[4] - N[1][1] * c[2] + N[1][3] * c[0]) * idet;
	M[2][1] = (-N[0][0] * c[4] + N[0][1] * c[2] - N[0][3] * c[0]) * idet;
	M[2][2] = (N[3][0] * s[4] - N[3][1] * s[2] + N[3][3] * s[0]) * idet;
	M[2][3] = (-N[2][0] * s[4] + N[2][1] * s[2] - N[2][3] * s[0]) * idet;

	M[3][0] = (-N[1][0] * c[3] + N[1][1] * c[1] - N[1][2] * c[0]) * idet;
	M[3][1] = (N[0][0] * c[3] - N[0][1] * c[1] + N[0][2] * c[0]) * idet;
	M[3][2] = (-N[3][0] * s[3] + N[3][1] * s[1] - N[3][2] * s[0]) * idet;
	M[3][3] = (N[2][0] * s[3] - N[2][1] * s[1] + N[2][2] * s[0]) * idet;
}
inline void CMatrix::TransformByInverted(LuacObj<CMatrix> n)
{
	CMatrix N;
	N.SetByInverted(n);
	*this *= N;
}
inline std::tuple<float, float, float> CMatrix::PointTransform(float x, float y, float z)
{
	float3 v = { x, y, z };
	v *= *this;
	return { v.x, v.y, v.z };
}
inline std::tuple<float, float, float> CMatrix::VectorTransform(float x, float y, float z)
{
	CMatrix& m = *this;
	return { x * m[0][0] + y * m[1][0] + z * m[2][0],
		 x * m[0][1] + y * m[1][1] + z * m[2][1],
		 x * m[0][2] + y * m[1][2] + z * m[2][2] };
}
inline const CMatrix& CMatrix::operator *= (const CMatrix& n)
{
	Transform(const_cast<CMatrix*>(&n), 4);
	return *this;
}
inline void CMatrix::Perspective(float fovD, float width, float height, float nearZ, float farZ)
{
	CMatrix& M = *this;
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
inline void CMatrix::SetByMatrixToView(LuacObj<CMatrix> N)
{
	CMatrix& m = *this;
	m.SetByTransposed(N);
	float3 p = m.col[3];
	m.col[3] = { 0, 0, 0, 1 };
	p *= m;
	m.SetRow4(-p.x, -p.y, -p.z, 1);
}
inline void CMatrix::CopyFrom(LuacObj<CMatrix> N)
{
	if (this != N)
		*this = *N;
}
inline std::tuple<float, float, float> CScreenToViewPos(float x, float y, float fov, float width, float height)
{
	if (fov == 0 || width == 0 || height == 0)
		return { 0, 0, 0 };
	float t = tanf(fov * M_PI / 180.0f * 0.5f);
	float aspect = width / height;
	float xw = (x / width * 2 - 1) * t * aspect;
	float yw = -(y / height * 2 - 1) * t;
	return { xw, yw, 1 };
}
Lua_global_add_cfunc(CScreenToViewPos)

inline std::tuple<float, float, float> CVec3Normalize(float x, float y, float z)
{
	float d = x * x + y * y + z * z;
	if (d == 0)
		d = 1;
	else if (d > 1)
		d = sqrt(d);
	return { x / d, y / d, z / d };
}
Lua_global_add_cfunc(CVec3Normalize)

inline std::tuple<float, float, float> CVec3NormalizeScale(float x, float y, float z, float scale)
{
	float3 v{};
	v = CVec3Normalize(x, y, z);
	v *= scale;
	return { v.x, v.y, v.z };
}
Lua_global_add_cfunc(CVec3NormalizeScale)

inline std::tuple<float, float, float> CVec3TransformInverted(float x, float y, float z, LuacObj<CMatrix> m)
{
	float3 v = { x, y, z };
	CMatrix M;
	M.SetByInverted(m);
	v *= M;
	return { v.x, v.y, v.z };
}
Lua_global_add_cfunc(CVec3TransformInverted)

inline float CDot3D(float x0, float y0, float z0, float x1, float y1, float z1)
{
	return x0 * x1 + y0 * y1 + z0 * z1;
}
Lua_global_add_cfunc(CDot3D)

inline float CDot2D(float x0, float y0, float x1, float y1)
{
	return x0 * x1 + y0 * y1;
}
Lua_global_add_cfunc(CDot2D)

inline std::tuple<float, float> CNormalize2D(float vx, float vy)
{
	float d = vx * vx + vy * vy;
	if (d == 0)
		d = 1;
	else if (d > 1)
		d = sqrt(d);
	return { vx / d, vy / d };
}
Lua_global_add_cfunc(CNormalize2D)

inline std::tuple<float, float> CGetLineNormal2D(float x0, float y0, float x1, float y1, bool counter_clockwise)
{
	float n = counter_clockwise ? 1 : -1;
	if (x0 == x1)
		return { y0 == y1 ? 0 : (y0 < y1 ? 1 : -1) * n, 0 };
	if (y0 == y1)
		return { 0, (x0 < x1 ? -1 : 1) * n };
	return CNormalize2D((y1 - y0) * n, (x0 - x1) * n);
}
Lua_global_add_cfunc(CGetLineNormal2D)

inline std::tuple<bool, float, float> CIntersect2D(float p0x, float p0y, float p1x, float p1y, bool p1AsDirection,
	float p2x, float p2y, float p3x, float p3y, bool p2Opened, bool p3Opened)
{
	float3 p0 = { p0x, p0y };
	float3 p1 = { p1x, p1y };
	float3 p2 = { p2x, p2y };
	float3 p3 = { p3x, p3y };

	float3 v0 = p1AsDirection ? p1 : p1 - p0;
	if (v0.x == 0 && v0.y == 0)
		return { false, 0, 0 };

	float3 v1 = p3 - p2;
	if (v1.x == 0 && v1.y == 0)
		return { false, 0, 0 };

	if ((v0.x == 0 && v1.x == 0) || (v0.y == 0 && v1.y == 0))
		return { false, 0, 0 };

	float t0 = 0;
	float t1 = 0;
	if (v1.x == 0)
	{
		t0 = (p2.x - p0.x) / v0.x;
		t1 = (p0.y + v0.y * t0 - p2.y) / v1.y;
	}
	else
	{
		float n = v1.y / v1.x;
		float d = v0.y - n * v0.x;
		if (d == 0)
			return { false, 0, 0 };
		float xd = p0.x - p2.x;

		t0 = (p2.y + n * xd - p0.y) / d;
		t1 = (xd + v0.x * t0) / v1.x;
	}

	if (t0 < 0 || (!p1AsDirection && t0 > 1))
		return { false, 0, 0 };

	if (t1 < 0 || (p2Opened && t1 == 0) || t1 > 1 || (p3Opened && t1 == 1))
		return { false, 0, 0 };

	return { true, p0x + v0.x * t0, p0y + v0.y * t0 };
}
Lua_global_add_cfunc(CIntersect2D)
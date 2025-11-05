#pragma once
#include <vector>
#include <stack>
#include <codecvt>
#include "Generic/Types.h"
#include "Generic/Math.h"
#include "Generic/LuaWrapper/LuaUtility.h"
#include "../include/EngineInterface.h"

inline void DebugLog(const wchar_t* szFmt, ...)
{
	wchar_t sz[4096]{};
	va_list argp;
	va_start(argp, szFmt);
	vswprintf_s(sz, 4096, szFmt, argp);
	va_end(argp);
#ifdef WIN32
	OutputDebugString(sz);
	OutputDebugString(L"\n");
#endif
}

#define ptr_deref(type, p, offset) *((type*)p + offset)

#define CHECK(a, ret) if (!(a)) return (ret);

//template<typename T0, typename ...T1>
//static inline void SetValue(void* p, int offset, int n, T1... t)
//{
//	for (int i = 0; i < num; i++)
//		ptr_deref(T0, p, offset + i) = {(0, t)...};
//}
//
//inline void CSetFloat1(void* p, int offset, float x, int wp, int num)
//{
//	SetValue<float>(p, offset, num, x);
//}
//Lua_wrap_cfunc(Lua_void, CSetFloat1, Lua_arg_ptr, Lua_arg_int, Lua_arg_float, Lua_arg_int)
//Lua_global_add_cfunc(CSetFloat1)
//
//inline void CSetFloat2(void* p, int offset, float x, float y, int wp, int num) 
//{
//	SetValue<float2>(p, offset, num, x, y);
//}
//Lua_wrap_cfunc(Lua_void, CSetFloat2, Lua_arg_ptr, Lua_arg_int, Lua_arg_float, Lua_arg_float, Lua_arg_int)
//Lua_global_add_cfunc(CSetFloat2)
//
//inline void CSetFloat3(void* p, int offset, float x, float y, float z, int wp, int num)
//{
//	SetValue<float3>(p, offset, num, x, y, z);
//}
//Lua_wrap_cfunc(Lua_void, CSetFloat3, Lua_arg_ptr, Lua_arg_int, Lua_arg_float, Lua_arg_float, Lua_arg_float, Lua_arg_int)
//Lua_global_add_cfunc(CSetFloat3)
//
//inline void CSetFloat4(void* p, int offset, float x, float y, float z, float w, int wp, int num)
//{
//	SetValue<float4>(p, offset, num, x, y, z, w);
//}
//Lua_wrap_cfunc(Lua_void, CSetFloat4, Lua_arg_ptr, Lua_arg_int, Lua_arg_float, Lua_arg_float, Lua_arg_float, Lua_arg_float, Lua_arg_int)
//Lua_global_add_cfunc(CSetFloat4)
//
//inline void CSetInt1(void* p, int offset, int x, int wp, int num)
//{
//	SetValue<int>(p, offset, num, x);
//}
//Lua_wrap_cfunc(Lua_void, CSetInt1, Lua_arg_ptr, Lua_arg_int, Lua_arg_int, Lua_arg_int)
//Lua_global_add_cfunc(CSetInt1)

//inline float CGetFloat1(void* p, int offset)
//{
//	return ptr_deref(float, p, offset);
//}
//Lua_wrap_cfunc(Lua_ret, CGetFloat1, Lua_arg_ptr, Lua_arg_int)
//Lua_global_add_cfunc(CGetFloat1)
//
//inline Lua_mul_ret(float, float) CGetFloat2(void* p, int offset)
//{
//	float2& v = ptr_deref(float2, p, offset);
//	return { v.x, v.y };
//}
//Lua_wrap_cfunc(Lua_ret, CGetFloat2, Lua_arg_ptr, Lua_arg_int)
//Lua_global_add_cfunc(CGetFloat2)
//
//inline Lua_mul_ret(float, float, float) CGetFloat3(void* p, int offset)
//{
//	float3& v = ptr_deref(float3, p, offset);
//	return { v.x, v.y, v.z };
//}
//Lua_wrap_cfunc(Lua_ret, CGetFloat3, Lua_arg_ptr, Lua_arg_int)
//Lua_global_add_cfunc(CGetFloat3)
//
//inline Lua_mul_ret(float, float, float, float) CGetFloat4(void* p, int offset)
//{
//	float4& v = ptr_deref(float4, p, offset);
//	return { v.x, v.y, v.z, v.w };
//}
//Lua_wrap_cfunc(Lua_ret, CGetFloat4, Lua_arg_ptr, Lua_arg_int)
//Lua_global_add_cfunc(CGetFloat4)
//
//inline int CGetInt1(void* p, int offset)
//{
//	return ptr_deref(int, p, offset);
//}
//Lua_wrap_cfunc(Lua_ret, CGetInt1, Lua_arg_ptr, Lua_arg_int)
//Lua_global_add_cfunc(CGetInt1)

class CBuffer
{
public: 
	virtual ~CBuffer() {};

	size_t GetWritePos()
	{
		return m_writePos;
	}

	void SetWritePos(uint32_t pos)
	{
		m_writePos = pos;
	}

	void* GetData(uint32_t offset)
	{
		return GetSize() > offset ? GetPtr() + offset : nullptr;
	}

	virtual bool Resize(uint32_t size) = 0;

	virtual size_t GetSize() = 0;

	virtual byte* GetPtr() = 0;

	size_t m_writePos = 0;

	Lua_wrap_cpp_class(CBuffer, Lua_abstract, Lua_mf(GetWritePos), Lua_mf(SetWritePos), Lua_mf(Resize));
};
Lua_global_add_cpp_class(CBuffer)

class CMBuffer : public CBuffer
{
public:
	CMBuffer(uint32_t size)
	{
		m_buffer.resize(size);
	}
	Lua_wrap_cpp_class_derived(CBuffer, CMBuffer, Lua_ctor(uint32_t));

	bool Resize(uint32_t size) override
	{
		m_buffer.resize(size);
		return true;
	}

	size_t GetSize() override
	{
		return m_buffer.size();
	}

	byte* GetPtr() override
	{
		return m_buffer.data();
	}

protected:
	std::vector<byte> m_buffer;
};
Lua_global_add_cpp_class(CMBuffer)

template<typename T>
class BufferWriter
{
public:
	BufferWriter(CBuffer& cb, size_t reserve, int wp = -1, bool changeWritePos = true) : m_cb(cb) 
	{
		m_writePos = m_cb.m_writePos;
		if (wp >= 0)
			m_writePos = wp;
		if (m_writePos + sizeof(T) * reserve > m_cb.GetSize())
			m_cb.Resize(m_writePos + sizeof(T) * reserve);
		m_changeWritePos = changeWritePos;
	}
	~BufferWriter()
	{
		SkipUsed();
		if (m_changeWritePos)
			m_cb.m_writePos = m_writePos;
	}
	T& operator [] (size_t n)
	{
		if (m_max < n + 1)
		{
			m_max = n + 1;
			size_t sizeNew = m_writePos + sizeof(T) * m_max;
			if (sizeNew > m_cb.GetSize())
				m_cb.Resize(sizeNew);
		}
		return *(T*)(m_cb.GetPtr() + m_writePos + sizeof(T) * n);
	}
	void SkipUsed()
	{
		m_writePos += sizeof(T) * m_max;
		m_max = 0;
	}

private:
	size_t m_writePos = 0;
	size_t m_max = 0;
	bool m_changeWritePos{};
	CBuffer& m_cb;
};

template<typename T0, typename ...T1>
static inline void AddValue(CBuffer& b, int wp, int n, T1... t)
{
	BufferWriter<T0> bw(b, n, wp);
	for (int i = 0; i < n; i++)
		bw[i] = { (0, t)... };
}

inline void CMulAddFloat1(int num, LuacObj<CBuffer> vb, int wp, float x)
{
	AddValue<float>(*vb, wp, num, x);
}
Lua_global_add_cfunc(CMulAddFloat1)

inline void CMulAddFloat2(int num, LuacObj<CBuffer> vb, int wp, float x, float y)
{
	AddValue<float2>(*vb, wp, num, x, y);
}
Lua_global_add_cfunc(CMulAddFloat2)

inline void CMulAddFloat3(int num, LuacObj<CBuffer> vb, int wp, float x, float y, float z)
{
	AddValue<float3>(*vb, wp, num, x, y, z);
}
Lua_global_add_cfunc(CMulAddFloat3)

inline void CMulAddFloat4(int num, LuacObj<CBuffer> vb, int wp, float x, float y, float z, float w)
{
	AddValue<float4>(*vb, wp, num, x, y, z, w);
}
Lua_global_add_cfunc(CMulAddFloat4)

inline void CMulAddInt1(int num, LuacObj<CBuffer> vb, int wp, int x)
{
	AddValue<int>(*vb, wp, num, x);
}
Lua_global_add_cfunc(CMulAddInt1)

inline void CMulAddUInt1(int num, LuacObj<CBuffer> vb, int wp, uint1 x)
{
	AddValue<uint1>(*vb, wp, num, x);
}
Lua_global_add_cfunc(CMulAddUInt1)

struct uint3
{
	uint1 x;
	uint1 y;
	uint1 z;
};
inline void CMulAddUInt3(int num, LuacObj<CBuffer> vb, int wp, uint1 x, uint1 y, uint1 z)
{
	AddValue<uint3>(*vb, wp, num, x, y, z);
}
Lua_global_add_cfunc(CMulAddUInt3)

inline void CMulAddUShort1(int num, LuacObj<CBuffer> vb, int wp, uint32_t x)
{
	AddValue<uint16_t>(*vb, wp, num, (uint16_t)x);
}
Lua_global_add_cfunc(CMulAddUShort1)

inline void CMulAddUByte4(int num, LuacObj<CBuffer> vb, int wp, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
	AddValue<uint32_t>(*vb, wp, num, r | (g << 8) | (b << 16) | (a << 24));
}
Lua_global_add_cfunc(CMulAddUByte4)

//inline void CAddRectFloat2(LuacObj<CBuffer> vb, int wp, float x, float y, float w, float h)
//{
//	BufferWriter<float2> bw(*vb, 4, wp);
//	bw[0] = { x, y };
//	bw[1] = { x + w, y };
//	bw[2] = { x + w, y + h };
//	bw[3] = { x, y + h };
//}
//Lua_global_add_cfunc(CAddRectFloat2)

inline void CAddRectFloat3(LuacObj<CBuffer> vb, int wp, float x, float y, float w, float h, float z)
{
	BufferWriter<float3> bw(*vb, 4, wp);
	bw[0] = { x, y, z };
	bw[1] = { x + w, y, z };
	bw[2] = { x + w, y + h, z };
	bw[3] = { x, y + h, z };
}
Lua_global_add_cfunc(CAddRectFloat3)

inline size_t CAddLine2D(LuacObj<CBuffer> vb_dst, int vwp, LuacObj<CBuffer> ib_dst, int iwp, int ioffset,
	LuacObj<CBuffer> vb_src, int rp, int count, bool closed, float thickness, bool outer, bool mid)
{
	if (count < 2)
		return 0;

	if (mid)
		thickness *= 0.5;
	closed = closed && count > 2;

	size_t last = count - 1;
	size_t icount = 6 * (closed ? count : last);
	BufferWriter<float3> src(*vb_src, count, rp);
	BufferWriter<float3> dst(*vb_dst, count * 2, vwp);
	BufferWriter<uint1> idst(*ib_dst, icount, iwp);

	float3 n0, n1, nn;
	if (closed)
		n0 = nn = CGetLineNormal2D(src[last].x, src[last].y, src[0].x, src[0].y, outer);

	for (size_t i = 0, j = count * 2 - 1, k = 0, ii = ioffset, jj = j + ioffset; i < count; i++, j--, ii++, jj--, n0 = n1)
	{
		float3 normal = n0;
		float3 p0 = src[i];
		if (i < last)
		{
			float3 p1 = src[i + 1];
			n1 = CGetLineNormal2D(p0.x, p0.y, p1.x, p1.y, outer);
			if (i == 0 && !closed)
				normal = n1;
			else
				normal = CNormalize2D((n0.x + n1.x) * 0.5, (n0.y + n1.y) * 0.5);

			if (outer)
			{
				idst[k++] = jj;
				idst[k++] = jj - 1;
				idst[k++] = ii;
				idst[k++] = ii;
				idst[k++] = jj - 1;
				idst[k++] = ii + 1;
			}
			else
			{
				idst[k++] = ii;
				idst[k++] = ii + 1;
				idst[k++] = jj;
				idst[k++] = jj;
				idst[k++] = ii + 1;
				idst[k++] = jj - 1;
			}
		}
		else if (closed)
		{
			normal = CNormalize2D((n0.x + nn.x) * 0.5, (n0.y + nn.y) * 0.5);

			if (outer)
			{
				idst[k++] = jj;
				idst[k++] = count * 2 - 1 + ioffset;
				idst[k++] = ii;
				idst[k++] = ii;
				idst[k++] = count * 2 - 1 + ioffset;
				idst[k++] = ioffset;
			}
			else
			{
				idst[k++] = ii;
				idst[k++] = ioffset;
				idst[k++] = jj;
				idst[k++] = jj;
				idst[k++] = ioffset;
				idst[k++] = count * 2 - 1 + ioffset;
			}
		}
		normal *= thickness;
		dst[i] = mid ? p0 - normal : p0;
		dst[j] = p0 + normal;
	}
	return icount;
}
Lua_global_add_cfunc(CAddLine2D)

inline size_t AddConvexPolyIndex(BufferWriter<uint1>& ibw, int idx_offset, uint32_t num_vtx)
{
	size_t n = 0;
	for (size_t j = 1; j < num_vtx - 1; j++)
	{
		ibw[n++] = idx_offset;
		ibw[n++] = idx_offset + j;
		ibw[n++] = idx_offset + j + 1;
	}
	return n;
}

inline size_t AddConvexPolyIndex(BufferWriter<uint1>& ibw, std::vector<uint1>& vtx_seq, int idx_offset)
{
	size_t n = 0;
	for (size_t i = 1; i < vtx_seq.size() - 1; i++)
	{
		ibw[n++] = vtx_seq[0] + idx_offset;
		ibw[n++] = vtx_seq[i] + idx_offset;
		ibw[n++] = vtx_seq[i + 1] + idx_offset;
	}
	return n;
}

inline size_t CAddConvexPolyIndex(uint32_t num_vtx, LuacObj<CBuffer> ib, int wp, int idx_offset, int count)
{
	if (num_vtx < 3)
		return 0;
	BufferWriter<uint1> bw(*ib, (num_vtx - 2) * 3 * count, wp);
	size_t n = AddConvexPolyIndex(bw, idx_offset, num_vtx);
	
	size_t o = num_vtx;
	for (size_t i = 1; i < count; i++, o += num_vtx)
		for (size_t j = 0; j < n; j++)
			bw[n * i + j] = bw[j] + o;

	return n * count;
}
Lua_global_add_cfunc(CAddConvexPolyIndex)

inline size_t AddPolyIndex(BufferWriter<float3>& vbw, size_t vnum, BufferWriter<uint1>& ibw, int idx_offset)
{
	std::stack<std::vector<uint1>> stack;
	stack.emplace(std::vector<uint1>(vnum));
	for (uint1 i = 0; i < vnum; i++)
		stack.top()[i] = i;

	size_t count = 0;
	bool pushed = true;
	while (true)
	{
		if (!pushed)
			stack.pop();
		if (stack.empty())
			break;
		pushed = false;

		ibw.SkipUsed();
		std::vector<uint1>& vtx_seq = stack.top();

		vnum = vtx_seq.size();
		if (vnum < 3)
			continue;
		if (vnum == 3)
		{
			ibw[0] = vtx_seq[0] + idx_offset;
			ibw[1] = vtx_seq[1] + idx_offset;
			ibw[2] = vtx_seq[2] + idx_offset;
			count += 3;
			continue;
		}

		float3 v0 = {};
		float3 v1 = {};
		bool firstConvexFound = false;
		bool concaveFound = false;
		size_t i = 0, i0 = 0, i1 = 0, i2 = 1, i3 = 2;
		for (; i < vnum && !firstConvexFound; i++, i0 = i1, i1 = i2, i2 = i3, i3 = ++i3 % vnum)
		{
			float3& p1 = vbw[vtx_seq[i1]];
			float3& p2 = vbw[vtx_seq[i2]];
			float3& p3 = vbw[vtx_seq[i3]];

			v0 = float3::Vector(p1, p2);
			v1 = float3::Vector(p2, p3);

			if (Vec2Cross(v0.x, v0.y, v1.x, v1.y) < 0)
				concaveFound = true;
			else
				firstConvexFound = true;
		}
		if (i == vnum && !concaveFound)
		{
			count += AddConvexPolyIndex(ibw, vtx_seq, idx_offset);
			continue;
		} 

		float3 v = float3::Vector(vbw[vtx_seq[i0]], vbw[vtx_seq[i1]]);
		concaveFound = false;
		for (i = 0; !concaveFound && i < vnum; i++, i1 = i2, i2 = i3, i3 = ++i3 % vnum)
		{
			float3& p1 = vbw[vtx_seq[i1]];
			float3& p2 = vbw[vtx_seq[i2]];
			float3& p3 = vbw[vtx_seq[i3]];

			v0 = float3::Vector(p1, p2);
			v1 = float3::Vector(p2, p3);

			concaveFound = Vec2Cross(v0.x, v0.y, v1.x, v1.y) < 0;
			if (!concaveFound && Vec2Cross(v.x, v.y, v1.x, v1.y) < 0)
			{
				v = v0;
				i0 = i1;
			}
		}
		if (!concaveFound)
		{
			count += AddConvexPolyIndex(ibw, vtx_seq, idx_offset);
			continue;
		}

		bool halfConvex = true;
		for (i = i0; i3 != i; i2 = i3, i3 = ++i3 % vnum)
		{
			float3& p0 = vbw[vtx_seq[i1]];
			float3& p1 = vbw[vtx_seq[i0]];
			float3& p2 = vbw[vtx_seq[i2]];
			float3& p3 = vbw[vtx_seq[i3]];
			if (p1.Same(p2) || p1.Same(p3))
				continue;
			auto t = CIntersect2D(p0.x, p0.y, p1.x, p1.y, false, p2.x, p2.y, p3.x, p3.y, true, false);
			if (std::get<0>(t))
			{
				halfConvex = false;
				i0 = i3;
			}
		}

		std::vector<uint1> vtemp = stack.top();
		stack.pop();

		stack.emplace(std::vector<uint1>());
		std::vector<uint1>& vtx_seq0 = stack.top();
		for (size_t i = i0; ; i = ++i % vnum)
		{
			vtx_seq0.push_back(vtemp[i]);
			if (i == i1)
				break;
		}
		if (halfConvex)
		{
			count += AddConvexPolyIndex(ibw, vtx_seq0, idx_offset);
			stack.pop();
		}

		stack.emplace(std::vector<uint1>());
		std::vector<uint1>& vtx_seq1 = stack.top();
		for (size_t i = i1; ; i = ++i % vnum)
		{
			vtx_seq1.push_back(vtemp[i]);
			if (i == i0)
				break;
		}
		pushed = true;
	}
	return count;
}

inline size_t CAddPolyIndex(LuacObj<CBuffer> vb, int vpos, uint32_t vnum, LuacObj<CBuffer> ib, int iwp, int idx_offset, size_t count)
{
	if (vnum < 3)
		return 0;

	BufferWriter<float3> vbw(*vb, vnum, vpos);
	BufferWriter<uint1> ibw(*ib, (vnum - 2) * 3 * count, iwp);
	
	size_t n = AddPolyIndex(vbw, vnum, ibw, idx_offset);
	vbw[vnum - 1];

	size_t o = vnum;
	for (size_t i = 1; i < count; i++, o += vnum)
		for (size_t j = 0; j < n; j++)
			ibw[n * i + j] = ibw[j] + o;

	return n * count;
}
Lua_global_add_cfunc(CAddPolyIndex)

inline size_t PrepareCopy(CBuffer& src, uint32_t src_pos, uint32_t stride, uint32_t count, CBuffer& dst, int dst_wp)
{
	if (src.GetSize() < src_pos)
		return 0;
	
	size_t size = stride * count;
	if (src.GetSize() - src_pos < size)
	{
		count = (src.GetSize() - src_pos) / stride;
		size = count * stride;
	}
	
	if (size == 0)
		return 0;

	if (dst_wp >= 0)
		dst.m_writePos = dst_wp;
	if (dst.GetSize() < dst.m_writePos + size)
		dst.Resize(dst.m_writePos + size);
	return count;
}

inline void CBufferCopy(LuacObj<CBuffer> dst, int dst_wp, LuacObj<CBuffer> src, int src_pos, int size)
{
	if (size < 0)
		size = src->GetSize();
	size = PrepareCopy(*src, src_pos, 1, size, *dst, dst_wp);
	if (size == 0)
		return;

	memcpy(dst->GetPtr() + dst->m_writePos, src->GetPtr() + src_pos, min(src->GetSize(), size));
	dst->m_writePos += size;
}
Lua_global_add_cfunc(CBufferCopy)

inline void CCopyIndexBuffer(LuacObj<CBuffer> dst, int dst_wp, LuacObj<CBuffer> src, uint32_t src_pos, uint32_t count, uint32_t idx_start)
{
	BufferWriter<uint1> bw_src(*src, count, src_pos);
	BufferWriter<uint1> bw_dst(*dst, count, dst_wp);

	for (size_t i = 0; i < count; i++)
		bw_dst[i] = bw_src[i] + idx_start;
}
Lua_global_add_cfunc(CCopyIndexBuffer)

inline void CReplaceIndex(LuacObj<CBuffer> dst, uint32_t wp, uint32_t count, uint32_t oldIndex, uint32_t newIndex)
{
	BufferWriter<uint1> bw_dst(*dst, count, wp);
	for (size_t i = 0; i < count; i++)
		if (bw_dst[i] == oldIndex)
			bw_dst[i] = newIndex;
}
Lua_global_add_cfunc(CReplaceIndex)

//inline void CMoveFloat2(LuacObj<CBuffer> src, int src_pos, size_t count, float x, float y, LuacObj<CBuffer> dst, int dst_wp)
//{
//	BufferWriter<float2> bw_src(*src, count, src_pos);
//	BufferWriter<float2> bw_dst(*dst, count, dst_wp);
//
//	for (size_t i = 0; i < count; i++)
//		bw_dst[i] = { bw_src[i].x + x, bw_src[i].y + y };
//}
//Lua_global_add_cfunc(CMoveFloat2)

inline void CMoveFloat3(LuacObj<CBuffer> dst, int dst_wp, LuacObj<CBuffer> src, int src_pos, size_t count, float x, float y, float z)
{
	BufferWriter<float3> bw_src(*src, count, src_pos);
	BufferWriter<float3> bw_dst(*dst, count, dst_wp);

	for (size_t i = 0; i < count; i++)
		bw_dst[i] = { bw_src[i].x + x, bw_src[i].y + y, bw_src[i].z + z };
}
Lua_global_add_cfunc(CMoveFloat3)

//inline void CScaleFloat2(LuacObj<CBuffer> src, int src_pos, size_t count, float sx, float sy, LuacObj<CBuffer> dst, int dst_wp)
//{
//	BufferWriter<float2> bw_src(*src, count, src_pos);
//	BufferWriter<float2> bw_dst(*dst, count, dst_wp);
//
//	for (size_t i = 0; i < count; i++)
//		bw_dst[i] = { bw_src[i].x * sx, bw_src[i].y * sy };
//}
//Lua_global_add_cfunc(CScaleFloat2)

inline void CScaleFloat3(LuacObj<CBuffer> dst, int dst_wp, LuacObj<CBuffer> src, int src_pos, size_t count, float sx, float sy, float sz)
{
	BufferWriter<float3> bw_src(*src, count, src_pos);
	BufferWriter<float3> bw_dst(*dst, count, dst_wp);

	for (size_t i = 0; i < count; i++)
		bw_dst[i] = { bw_src[i].x * sx, bw_src[i].y * sy, bw_src[i].z * sz };
}
Lua_global_add_cfunc(CScaleFloat3)

//inline std::tuple<size_t, size_t> CTransformFloat2(LuacObj<CBuffer> src, int src_pos, size_t count, LuacObj<CMatrix2D> m, LuacObj<CBuffer> dst, int dst_wp)
//{
//	BufferWriter<float2> bw_src(*src, count, src_pos);
//	BufferWriter<float2> bw_dst(*dst, count, dst_wp);
//
//	float w = 0, h = 0;
//	for (size_t i = 0; i < count; i++)
//	{
//		bw_dst[i] = bw_src[i] * *m;
//		w = bw_dst[i].x > w ? bw_dst[i].x : w;
//		h = bw_dst[i].y > h ? bw_dst[i].y : h;
//	}
//	return { w, h };
//}
//Lua_global_add_cfunc(CTransformFloat2)

inline std::tuple<float, float, float> CTransformFloat3(LuacObj<CBuffer> dst, int dst_wp, LuacObj<CBuffer> src, int src_pos, size_t count, LuacObj<CMatrix> m)
{
	BufferWriter<float3> bw_src(*src, count, src_pos, false);
	BufferWriter<float3> bw_dst(*dst, count, dst_wp);

	float l = 0, w = 0, h = 0;
	for (size_t i = 0; i < count; i++)
	{
		bw_dst[i] = bw_src[i] * *m;
		l = bw_dst[i].x > l ? bw_dst[i].x : l;
		w = bw_dst[i].y > w ? bw_dst[i].y : w;
		h = bw_dst[i].z > h ? bw_dst[i].z : h;
	}
	return { l, w, h };
}
Lua_global_add_cfunc(CTransformFloat3)

inline void CAddMatrix(LuacObj<CBuffer> vb, int wp, LuacObj<CMatrix> m)
{
	BufferWriter<CMatrix> bw(*vb, 1, wp);
	bw[0] = *m;
}
Lua_global_add_cfunc(CAddMatrix)

inline void CMatrixMultiply(LuacObj<CBuffer> vb, int wp, LuacObj<CMatrix> m1, LuacObj<CMatrix> m2)
{
	BufferWriter<CMatrix> bw(*vb, 1, wp);
	CMatrix& m = bw[0];
	m.SetByMultiplied(m1, m2);
}
Lua_global_add_cfunc(CMatrixMultiply)

inline void CMatrixToView(LuacObj<CBuffer> vb, int wp, LuacObj<CMatrix> m)
{
	BufferWriter<CMatrix> bw(*vb, 1, wp);
	CMatrix& v = bw[0];
	v.SetByTransposed(m);
	float3 p = v.col[3];
	v.col[3] = { 0, 0, 0, 1 };
	p *= v;
	v.SetRow4(-p.x, -p.y, -p.z, 1);
}
Lua_global_add_cfunc(CMatrixToView)

inline void CMatrixToViewMultiply(LuacObj<CBuffer> vb, int wp, LuacObj<CMatrix> m, LuacObj<CMatrix> n)
{
	BufferWriter<CMatrix> bw(*vb, 1, wp);
	CMatrix& v = bw[0];
	v.SetByTransposed(m);
	float3 p = v.col[3];
	v.col[3] = { 0, 0, 0, 1 };
	p *= v;
	v.SetRow4(-p.x, -p.y, -p.z, 1);
	v *= *n;
	float3 z0 = { -1, 0, 0 };
	z0 *= v;
	float3 z1 = { 0, 1, 0 };
	z1 *= v;
	float3 z2 = { 1, 0, 0 };
	z2 *= v;
	float3 zz = z0;
}
Lua_global_add_cfunc(CMatrixToViewMultiply)

class CList
{
public:
	void push_back(void* p)
	{
		list.push_back(p);
	}

	std::list<void*> list;

	Lua_wrap_cpp_class(CList, Lua_ctor(), Lua_mf(push_back));
};
Lua_global_add_cpp_class(CList);

inline void CLuaLoad(LuaReturn& ret, LuacObj<Engine::StreamInput> input)
{
	ret.Push(LuaLoad(input->GetData(), input->GetSize()));
}
Lua_global_add_cfunc(CLuaLoad)


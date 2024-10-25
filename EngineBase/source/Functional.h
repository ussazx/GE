#pragma once
#include <vector>
#include "Generic/Types.h"
#include "Generic/LuaWrapper/LuaGlobalCollect.h"

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

	size_t m_writePos;

	Lua_wrap_cpp_class(CBuffer, Lua_abstract, Lua_mf(GetWritePos), Lua_mf(SetWritePos));
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
	BufferWriter(CBuffer& cb, size_t reserve, int wp = -1) : m_cb(cb) 
	{
		if (wp >= 0)
			m_cb.m_writePos = wp;
		if (m_cb.m_writePos + sizeof(T) * reserve > m_cb.GetSize())
			m_cb.Resize(m_cb.m_writePos + sizeof(T) * reserve);
	}
	~BufferWriter()
	{
		m_cb.m_writePos += sizeof(T) * m_max;
	}
	T& operator [] (size_t n)
	{
		if (m_max < n + 1)
		{
			m_max = n + 1;
			size_t sizeNew = m_cb.m_writePos + sizeof(T) * m_max;
			if (sizeNew > m_cb.GetSize())
				m_cb.Resize(sizeNew);
		}
		return *(T*)(m_cb.GetPtr() + m_cb.m_writePos + sizeof(T) * n);
	}
	void SkipUsed()
	{
		m_cb.m_writePos += sizeof(T) * m_max;
		m_max = 0;
	}

private:
	size_t m_max{};
	CBuffer& m_cb;
};

template<typename T0, typename ...T1>
static inline void AddValue(CBuffer& b, int wp, int n, T1... t)
{
	BufferWriter<T0> bw(b, n, wp);
	for (int i = 0; i < n; i++)
		bw[i] = { (0, t)... };
}

inline void CAddFloat1(LuacObj<CBuffer> vb, int wp, int num, float x)
{
	AddValue<float>(*vb, wp, num, x);
}
Lua_global_add_cfunc(CAddFloat1)

inline void CAddFloat2(LuacObj<CBuffer> vb, int wp, int num, float x, float y)
{
	AddValue<float2>(*vb, wp, num, x, y);
}
Lua_global_add_cfunc(CAddFloat2)

inline void CAddFloat3(LuacObj<CBuffer> vb, int wp, int num, float x, float y, float z)
{
	AddValue<float3>(*vb, wp, num, x, y, z);
}
Lua_global_add_cfunc(CAddFloat3)

inline void CAddFloat4(LuacObj<CBuffer> vb, int wp, int num, float x, float y, float z, float w)
{
	AddValue<float4>(*vb, wp, num, x, y, z, w);
}
Lua_global_add_cfunc(CAddFloat4)

inline void CAddInt1(LuacObj<CBuffer> vb, int wp, int num, int x)
{
	AddValue<int>(*vb, wp, num, x);
}
Lua_global_add_cfunc(CAddInt1)

inline void CAddUInt1(LuacObj<CBuffer> vb, int wp, int num, uint1 x)
{
	AddValue<uint1>(*vb, wp, num, x);
}
Lua_global_add_cfunc(CAddUInt1)

inline void CAddUShort1(LuacObj<CBuffer> vb, int wp, int num, uint32_t x)
{
	AddValue<uint16_t>(*vb, wp, num, (uint16_t)x);
}
Lua_global_add_cfunc(CAddUShort1)

inline void CAddUByte4(LuacObj<CBuffer> vb, int wp, int num, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
	AddValue<uint32_t>(*vb, wp, num, r | (g << 8) | (b << 16) | (a << 24));
}
Lua_global_add_cfunc(CAddUByte4)

inline void CAddRectFloat2(LuacObj<CBuffer> vb, int wp, float x, float y, float w, float h)
{
	BufferWriter<float2> bw(*vb, 4, wp);
	bw[0] = { x, y };
	bw[1] = { x + w, y };
	bw[2] = { x + w, y + h };
	bw[3] = { x, y + h };
}
Lua_global_add_cfunc(CAddRectFloat2)

inline void CAddRectFloat3(LuacObj<CBuffer> vb, int wp, float x, float y, float w, float h)
{
	BufferWriter<float3> bw(*vb, 4, wp);
	bw[0] = { x, y, 1 };
	bw[1] = { x + w, y, 1 };
	bw[2] = { x + w, y + h, 1 };
	bw[3] = { x, y + h, 1 };
}
Lua_global_add_cfunc(CAddRectFloat3)

inline size_t CAddConvexPolyIndex(LuacObj<CBuffer> ib, int wp, int num, int idx_offset, uint32_t num_vtx)
{
	if (num_vtx < 3)
		return 0;
	BufferWriter<uint1> bw(*ib, (num_vtx - 2) * 3 * num, wp);
	size_t n = 0;
	for (size_t i = 0; i < num; i++, idx_offset += num_vtx)
		for (size_t j = 1; j < num_vtx - 1; j++)
		{
			bw[n++] = idx_offset;
			bw[n++] = idx_offset + j;
			bw[n++] = idx_offset + j + 1;
		}
	return n;
}
Lua_global_add_cfunc(CAddConvexPolyIndex)

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

inline void CBufferCopy(LuacObj<CBuffer> src, uint32_t src_pos, uint32_t size, LuacObj<CBuffer> dst, int dst_wp)
{
	size = PrepareCopy(*src, src_pos, 1, size, *dst, dst_wp);
	if (size == 0)
		return;

	memcpy(dst->GetPtr() + dst->m_writePos, src->GetPtr() + src_pos, min(src->GetSize(), size));
	dst->m_writePos += size;
}
Lua_global_add_cfunc(CBufferCopy)

inline void CCopyIndexBuffer(LuacObj<CBuffer> src, uint32_t src_pos, uint32_t count, uint32_t idx_start, LuacObj<CBuffer> dst, int dst_wp)
{
	count = PrepareCopy(*src, src_pos, sizeof(uint1), count, *dst, dst_wp);
	if (count == 0)
		return;

	for (size_t i = 0; i < count; i++)
		*((uint1*)(dst->GetPtr() + dst->m_writePos) + i) = *((uint1*)(src->GetPtr() + src_pos) + i) + idx_start;
	dst->m_writePos += sizeof(uint1) * count;
}
Lua_global_add_cfunc(CCopyIndexBuffer)

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


#include "Internal.h"
#include "../../Generic/Math.h"

std::tuple<size_t, size_t> CAddCube(LuacObj<CBuffer> pos, int vwp, LuacObj<CBuffer> norm, int nwp, 
	LuacObj<CBuffer> uv, int uwp, LuacObj<CBuffer> ib, int iwp)
{
	BufferWriter<float3> pbw(*pos, 24, vwp);
	BufferWriter<float3> nbw(*norm, 24, uwp);
	BufferWriter<float2> ubw(*uv, 24, vwp);
	BufferWriter<uint1> ibw(*ib, 36, vwp);

	pbw[0] = { -1, 1, 1 };
	pbw[1] = { 1, 1, 1 };
	pbw[2] = { 1, 1, -1 };
	pbw[3] = { -1, 1, -1 };

	pbw[4] = pbw[3];
	pbw[5] = pbw[2];
	pbw[6] = { 1, -1, -1 };
	pbw[7] = { -1, -1, -1 };

	pbw[8] = pbw[1];
	pbw[9] = pbw[0];
	pbw[10] = { -1, -1, 1 };
	pbw[11] = { 1, -1, 1 };

	pbw[12] = pbw[7];
	pbw[13] = pbw[6];
	pbw[14] = pbw[11];
	pbw[15] = pbw[10];

	pbw[16] = pbw[0];
	pbw[17] = pbw[3];
	pbw[18] = pbw[7];
	pbw[19] = pbw[10];

	pbw[20] = pbw[2];
	pbw[21] = pbw[1];
	pbw[22] = pbw[11];
	pbw[23] = pbw[6];

	for (size_t i = 0, j = 0, k = 0; i < 24; i = ++k * 4)
	{
		ubw[i] = { 0, 0 };
		ubw[i + 1] = { 1, 0 };
		ubw[i + 2] = { 1, 1 };
		ubw[i + 3] = { 0, 1 };

		ibw[j++] = i;
		ibw[j++] = i + 1;
		ibw[j++] = i + 2;
		ibw[j++] = i;
		ibw[j++] = i + 2;
		ibw[j++] = i + 3;
	}
	return { 24, 36 };
}
Lua_global_add_cfunc(CAddCube);

std::tuple<size_t, size_t> CAddSphere(LuacObj<CBuffer> pb, int pwp, LuacObj<CBuffer> nb, int nwp,
	LuacObj<CBuffer> ub, int uwp, LuacObj<CBuffer> ib, int iwp, float radius, unsigned int levels, unsigned int slices)
{
	if (levels < 2)
		levels = 2;
	if (slices < 1)
		slices = 1;
 
	size_t vtxCount = 2 + (levels - 1) * (slices + 1);
	size_t idxCount = 6 * (levels - 1) * slices;

	BufferWriter<float3> pbw(*pb, vtxCount, pwp);
	BufferWriter<float3> nbw(*nb, vtxCount, nwp);
	BufferWriter<float2> ubw(*ub, vtxCount, uwp);
	BufferWriter<uint1> ibw(*ib, idxCount, iwp);

	float3 pos = { 0.0f, radius, 0.0f };
	float3 norm = { 0.0f, 1.0f, 0.0f };
	float2 uv = { 0.0f, 0.0f };

	pbw[0] = pos;
	nbw[0] = norm;
	ubw[0] = uv;

	float phi = 0.0f, theta = 0.0f;
	float per_phi = M_PI / levels;
	float per_theta = M_2PI / slices;

	unsigned int index = 1;

	for (unsigned int i = 1; i < levels; i++)
	{
		phi = per_phi * i;
		for (unsigned int j = 0; j <= slices; j++)
		{
			theta = per_theta * j;

			pos.x = radius * sinf(phi) * cosf(theta);
			pos.y = radius * cosf(phi);
			pos.z = radius * sinf(phi) * sinf(theta);
			norm = pos;
			norm.Normalize();
			uv = { theta / M_2PI, phi / M_PI };

			pbw[index] = pos;
			nbw[index] = norm;
			ubw[index] = uv;
			index++;
		}
	}
	pos = { 0.0f, -radius, 0.0f };
	norm = { 0.0f, -1.0f, 0.0f };
	uv = { 0.0f, 1.0f };
	pbw[index] = pos;
	nbw[index] = norm;
	ubw[index] = uv;
	index++;

	index = 0;

	for (unsigned int j = 1; j <= slices; j++)
	{
		ibw[index++] = 0;
		ibw[index++] = j % (slices + 1) + 1;
		ibw[index++] = j;
	}

	for (unsigned int i = 1; i < levels - 1; i++)
	{
		for (unsigned int j = 1; j <= slices; j++)
		{
			ibw[index++] = (i - 1) * (slices + 1) + j;
			ibw[index++] = (i - 1) * (slices + 1) + j % (slices + 1) + 1;
			ibw[index++] = i * (slices + 1) + j % (slices + 1) + 1;
			ibw[index++] = i * (slices + 1) + j % (slices + 1) + 1;
			ibw[index++] = i * (slices + 1) + j;
			ibw[index++] = (i - 1) * (slices + 1) + j;
		}
	}

	for (unsigned int j = 1; j <= slices; j++)
	{
		ibw[index++] = (levels - 2) * (slices + 1) + j;
		ibw[index++] = (levels - 2) * (slices + 1) + j % (slices + 1) + 1;
		ibw[index++] = (levels - 1) * (slices + 1) + 1;
	}

	idxCount = index;

	return { vtxCount, idxCount };
}
Lua_global_add_cfunc(CAddSphere);

size_t CAddLineListIndex(size_t num, LuacObj<CBuffer> ib, int wp, int idx_offset)
{
	BufferWriter<uint1> bw(*ib, num, wp);
	for (size_t i = 0; i < num; i++)
		bw[i] = i + idx_offset;
	return num;
}
Lua_global_add_cfunc(CAddLineListIndex);

void CAddLine(float x0, float y0, float z0, float x1, float y1, float z1, LuacObj<CBuffer> vb, int wp)
{
	BufferWriter<float3> bw(*vb, 2, wp);
	bw[0] = { x0, y0, z0 };
	bw[1] = { x1, y1, z1 };
}
Lua_global_add_cfunc(CAddLine);

std::tuple<size_t, size_t> CGetIndicesSegment(LuacObj<CBuffer> ib, int wp, size_t offset, size_t count)
{
	BufferWriter<uint1> bw(*ib, offset + count, wp);
	size_t min = bw[offset], max = bw[offset];
	for (size_t i = offset; i < offset + count; i++)
		if (bw[i] < min)
			min = bw[i];
		else if (bw[i] > max)
			max = bw[i];
	return { min, max };
}
Lua_global_add_cfunc(CGetIndicesSegment);

class CSkinning {};

class CTransformer
{
public:
	typedef std::tuple<bool, BufferWriter<float3>, BufferWriter<float3>> Factors;
	void AddFactors(LuacObj<CBuffer> src, int wpSrc, size_t start, size_t count, LuacObj<CBuffer> dst, int wDst, bool isNormal)
	{
		auto& f = m_factors[start][count];
		f.reserve(m_factors.size() + 1);
		m_hasNormal = m_hasNormal || isNormal;
		f.emplace_back(Factors(isNormal, 
			BufferWriter<float3>(*dst, count, wDst), BufferWriter<float3>(*src, start + count, wpSrc)));
	}
	void MatrixTransform(LuacObj<CMatrix> m)
	{
		CMatrix mn;
		if (m_hasNormal)
			;
		for (auto& i : m_factors)
			for (auto& j : i.second)
			{
				if (j.second.size() == 0)
					continue;
				for (size_t n = 0; n < j.first; n++)
					for (auto& k : j.second)
						std::get<1>(k)[i.first + n] = std::get<2>(k)[n] * (std::get<0>(k) ? mn : *m);
				j.second.clear();
			}
		m_hasNormal = false;
	}
	//void SkinningTransform(CSkinning& skn, )
	bool m_hasNormal;
	std::unordered_map<size_t, std::unordered_map<size_t, std::vector<Factors>>> m_factors;
	Lua_wrap_cpp_class(CTransformer, Lua_ctor_void, Lua_mf(AddFactors), Lua_mf(MatrixTransform))
};
Lua_global_add_cpp_class(CTransformer);
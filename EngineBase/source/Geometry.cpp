#include "Internal.h"

std::tuple<size_t, size_t> CAddCube(LuacObj<CBuffer> pos, int vwp, LuacObj<CBuffer> uv, int uwp, LuacObj<CBuffer> ib, int iwp)
{
	BufferWriter<float3> pwb(*pos, 24, vwp);
	BufferWriter<float2> uwb(*uv, 24, vwp);
	BufferWriter<uint1> iwb(*ib, 36, vwp);

	pwb[0] = { -1, 1, 1 };
	pwb[1] = { 1, 1, 1 };
	pwb[2] = { 1, 1, -1 };
	pwb[3] = { -1, 1, -1 };

	pwb[4] = pwb[3];
	pwb[5] = pwb[2];
	pwb[6] = { 1, -1, -1 };
	pwb[7] = { -1, -1, -1 };

	pwb[8] = pwb[1];
	pwb[9] = pwb[0];
	pwb[10] = { -1, -1, 1 };
	pwb[11] = { 1, -1, 1 };

	pwb[12] = pwb[7];
	pwb[13] = pwb[6];
	pwb[14] = pwb[11];
	pwb[15] = pwb[10];

	pwb[16] = pwb[0];
	pwb[17] = pwb[3];
	pwb[18] = pwb[7];
	pwb[19] = pwb[10];

	pwb[20] = pwb[2];
	pwb[21] = pwb[1];
	pwb[22] = pwb[11];
	pwb[23] = pwb[6];

	for (size_t i = 0, j = 0, k = 0; i < 24; i = ++k * 4)
	{
		uwb[i] = { 0, 0 };
		uwb[i + 1] = { 1, 0 };
		uwb[i + 2] = { 1, 1 };
		uwb[i + 3] = { 0, 1 };

		iwb[j++] = i;
		iwb[j++] = i + 1;
		iwb[j++] = i + 2;
		iwb[j++] = i;
		iwb[j++] = i + 2;
		iwb[j++] = i + 3;
	}
	return { 24, 36 };
}
Lua_global_add_cfunc(CAddCube);

std::tuple<size_t, size_t> CGetIndicesSegment(LuacObj<CBuffer> ib, int wp, size_t offset, size_t count)
{
	BufferWriter<uint1> bw(*ib, offset + count, wp);
	size_t min = bw[0], max = bw[0];
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
	void MatrixTransform(LuacObj<CMatrix3D> m)
	{
		CMatrix3D mn;
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
	Lua_wrap_cpp_class(CTransformer, Lua_ctor(), Lua_mf(AddFactors), Lua_mf(MatrixTransform))
};
Lua_global_add_cpp_class(CTransformer);
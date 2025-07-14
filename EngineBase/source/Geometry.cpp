#include "Functional.h"

std::tuple<size_t, size_t> CAddCube(LuacObj<CBuffer> pos, int vwp, LuacObj<CBuffer> uv, int uwp, LuacObj<CBuffer> ib, int iwp)
{
	BufferWriter<float3> pwb(*pos, 24, vwp);
	BufferWriter<float2> uwb(*pos, 24, vwp);
	BufferWriter<uint1> iwb(*ib, 36, vwp);

	pwb[0] = { -1, -1, 1 };
	pwb[1] = { 1, -1, 1 };
	pwb[2] = { 1, -1, -1 };
	pwb[3] = { -1, -1, -1 };

	pwb[4] = pwb[3];
	pwb[5] = pwb[2];
	pwb[6] = { 1, 1, -1 };
	pwb[7] = { -1, 1, -1 };

	pwb[8] = pwb[1];
	pwb[9] = pwb[0];
	pwb[10] = { -1, 1, 1 };
	pwb[11] = { 1, 1, 1 };

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

	for (size_t i = 0, j = 0; i < 24; i *= 4)
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
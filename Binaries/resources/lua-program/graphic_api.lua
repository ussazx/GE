-----graphic_api-----
require 'class'



Mesh = class()

function Mesh:ctor(num_vertices, combined_locations)
	CreateMesh(self)
end

m = Mesh(8, x)
	
m

rp0 = CreateRP(...)

	rtv0 = CreateRTV(...)

	fb0 = CreateFB(rp0, ...)
	
	m0 = Create
	
	rf0 = CreateRF

-----matrix-----
Matrix = class()

function Matrix:ctor()
	self.m = CreateMatrix()
end

function Matrix:set_val(row, col, val)
	SetMatrixVal(self.m, col, val, val)
end

function Matrix:set_row_val(col, ...)
	SetMatrixRow(self.m, col, ...)
end

-----skeleton-----
Skeleton = class()

function Skeleton:ctor()
	self.bone_array = {}
	self.trans_array = {}
end

function Skeleton:update_transform()
	for i, b in pairs(self.bone_array) do
		b:update(self.trans_array[i])
	end
end

-----variable type-----
FLOAT1 = 1
FLOAT2 = 2
FLOAT3 = 3
FLOAT4 = 4
INT1 = 5

-----vb element location-----
VB_FLOAT1 = 0x00000001
VB_FLOAT2 = 0x00000010
VB_FLOAT3 = 0x00000100
VB_FLOAT4 = 0x00001000
VB_INT1 = 0x00010000

-----skeleton_mesh-----
SkMesh = class()

function SkMesh:ctor()
	
end

-----skinning array-----
SkinningArray = class()

function SkinningArray:ctor()
	self.o = CreateSkinningArray()
end

function SkinningArray:add_bone_info(vtx_idx, bone_idx, weight)
	AddBoneInfo(self.o, vtx_idx, bone_idx, weight)
end

ui_mesh_desc = {}
ui_mesh_desc.num_vertices = 100
ui_mesh_desc.num_float2 = 2
ui_mesh_desc.num_float4 = 1

-----Mesh-----
function Mesh:ctor(desc)
	self.co = CreateMesh(desc, self)
end

function Mesh:move(offset)
	if (offset > self.offset_max) then
		self.offset_max = offset
	end
	self.offset = offset
end

function Mesh:skip_moved()    
	self.current = self.current + self.offset_max
	self.offset_max = 0
end

function Mesh:set_float2(idx, x, y)
	SetFloat2(self.float2[idx], self.current + self.offset, x, y)
end

function Mem:set_copy_op(element_idx, operator, param)

function Mem:copy_to(dst_mem, dst_offset)
	MemCopyTo(self.mem, dst_mem, element_idx)
end
-----vbo-----
VBO = class()

function VBO:ctor(element_type_counts, vertex_count, is_gpu_side)
	if (is_gpu_side) then
		self.vb = CreateVB()
	end
	self.float2 = {}
	self.float2[0] = CreateBuf(SIZE_FLOAT * vertex_count)
	if (is_gpu_side)
		self.vb:Add(self.float2[0])
	self.float3 = {}
	self.float3[0] = CreateBuf(SIZE_FLOAT3 * vertex_count)
	self.vbo, mem = VBOCreate(element_type_counts, vertex_count, is_gpu_side)
	self.mem = Mem(mem)
	self.is_gpu_side = is_gpu_side
	float3.SetValue(ptr, offset, x, y, z)

	--self.current = 0
	--self.offset_max = 0
end

function VBO:set_val(element, idx, method, ...)
	method(self.element[idx], ...)
end

-----ibo-----
local IBO = class()

function IBO:ctor(index_count)
	self.ib = CreateIB(index_count)
	self.current = 0
	self.offset_max = 0
end

function IBO:ctor()
	RemoveIB(self.ib)
end

function IBO:skip_used()
	self.current = self.current + self.offset_max
	self.offset_max = 0
end

function IBO:set_val(offset, val)
	if (offset > self.offset_max)
		self.offset_max = offset
	SetIBVal(ib, self.current + offset, val)
end

-----SPO------

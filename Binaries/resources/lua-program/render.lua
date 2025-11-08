-----render.lua-----
require 'graphic'
require 'geometry'

---Material---
function Material(mtl)
	return setmetatable({}, {__index = mtl})
end

---InstancePos---
InstancePos = class()

function InstancePos:ctor(inst, pos)
	self.inst = inst
	self.pos = pos
end

function InstancePos:dtor()
	self.inst:RecyclePos(self.pos)
end

---PerObjectInstance---
PerObjectInstance = class()
local function AddMatrix(n, vb, wp, m)
	CAddMatrix(vb, wp, m)
end
function PerObjectInstance:ctor(...)
	self.type = PerObjectInstance
	self.objPos = setmetatable({inst = self}, {__mode = 'k'})
	self.recycledPos = {}
	self.pos = 1
	self.func = {}
	self.defMtx = CMatrix()
	local i = 0
	for k, v in pairs({...}) do
		if (v == CAddMatrix) then
			v = AddMatrix
		end
		self.func[k] = {func = v, i = i}
		i = i + g_funcSize[v]
	end
	self.vbId = InstanceBuffer(SIZE_WRITE_ID)
	self.vbMtx = InstanceBuffer(CMatrix._size)
	if (i > 0) then
		self.vbExtra = InstanceBuffer(i)
	end
	self.perSize = i
end

function PerObjectInstance:SetDefaultValue(idx, ...)
	local o = self.func[idx]
	o.func(1, self.vbExtra(), o.i, ...)
end

function PerObjectInstance:RecyclePos(pos)
	self.pos[pos] = true
end

function PerObjectInstance:AddObject(obj)
	local pos = next(self.recycledPos)
	if (pos) then
		self.recycledPos[pos] = nil
	else
		pos = self.pos
		self.pos = self.pos + 1
	end
	local o = InstancePos(self, pos)
	self.objPos[obj] = o
	CMulAddUInt1(1, self.vbId(), pos * SIZE_WRITE_ID, obj.id)
	CAddMatrix(self.vbMtx(), pos * CMatrix._size, self.defMtx)
	if (self.vbExtra) then
		local vb = self.vbExtra()
		CBufferCopy(vb, self.perSize * pos, vb, 0, self.perSize)
	end
	return o
end

function PerObjectInstance:SetObjectMatrix(obj)
	local pos = self.objPos[obj] or self:AddObject(obj)
	CAddMatrix(self.vbMtx(), pos.pos * CMatrix._size, obj.mModel)
end
		
function PerObjectInstance:SetObjectValue(idx, obj, ...)
	local pos = self.objPos[obj] or self:AddObject(obj)
	local o = self.func[idx]
	o.func(1, self.vbExtra(), self.perSize * pos.pos + o.i, ...)
end

g_defaultMeshInst = PerObjectInstance()

---Geometry---
Geometry = class()
Geometry.TRANS_NONE = 0
Geometry.TRANS_DEFAULT = 1
Geometry.TRANS_NORMAL = 2
function Geometry:ctor(o)
	self.layout = o.layout
	self.vb = o.vb
	self.ib = o.ib
	self.meshes = o.meshes
	local s = 'return function(model, scene) local m = model.meshes'
	for k, m in pairs(o.meshes) do
		if (o.ib) then
			m.vbStart, m.vbEnd = CGetIndicesSegment(o.ib, 0, m[1], m[2])
		end
		s = string.format('%s m[%q].renderer:Render(scene)', s, k)
	end
	self.renderFunc = load(s .. ' end', 'renderer', 't')()
	
	self.trans = CTransformer()
	local write = 'return function(mesh, vbStart, vbCount, idxOffset, idxCount'
	local write_d = 'return function(mesh, vbCount'
	for k, v in pairs(o.vbInfo) do
		local s = string.format(', vbDst%q, wp%q', k, k)
		write = write .. s
		write_d = write_d .. s
	end
	s = ', ib, iwp, ibStart) '
	write = write .. s .. 'local vb '
	write_d = write_d .. s
	for k, v in pairs(o.vbInfo) do
		write = write .. string.format('vb = mesh.vb[%q] ', k)
		if (v[1] == Geometry.TRANS_DEFAULT or v[1] == Geometry.TRANS_NORMAL) then
			local isNormal = v[1] == Geometry.TRANS_NORMAL
			write = write .. string.format('mesh:Trans(vb, 0, vbStart, vbCount, vbDst%q, wp%q, %q) ', k, k, isNormal)
			write_d = write_d .. string.format('mesh:Trans(vbDst%q, wp%q, 0, vbCount, vbDst%q, wp%q, %q) ', k, k, k, k, isNormal)
		else
			write = write .. 
			string.format(' CBufferCopy(vbDst%q, wp%q, vb, vbStart * %q, vbCount * %q) ', k, k, v[2], v[2])
		end
	end
	write = write .. 'CCopyIndexBuffer(ib, iwp, mesh.ib, idxOffset, idxCount, ibStart - vbStart) end'
	write_d = write_d .. 'end'
	self.WriteStatic = load(write, 'write', 't')()
	self.WriteDynamic = load(write_d, 'write_d', 't')()
end

---Mesh---
Mesh = class()

function Mesh:ctor(id, geom, vbStart, vbEnd, idxOffset, idxCount)
	self.id = id
	self.trans = geom.trans
	self.WriteStatic = geom.WriteStatic
	self.WriteDynamic = geom.WriteDynamic
	self.vb = geom.vb
	self.ib = geom.ib
	self.vbStart = vbStart or 0
	self.vbEnd = vbEnd or 0
	self.idxOffset = idxOffset or 0
	self.idxCount = idxCount or 0
	self.instArgs = {}
	self.renderer = Renderer(self, geom.layout, self.Write)
end

function Mesh:CPUTrans(...)
	self.trans:AddFactors(...)
end

function Mesh:GPUTrans(src, wpSrc, start, count, dst, wpDst)
	CBufferCopy(dst, wpDst, src, start * SIZE_FLOAT3, count * SIZE_FLOAT3)
end

function Mesh:SetInstArgs(inst, start, count)
	local o = self.instArgs[inst] or {}
	o[1], o[2] = start, count
	self.instArgs[inst] = o
end

function Mesh:Write(...)
	local vbCount = self.vbEndNew - self.vbStartNew + 1
	if (vbCount > 1) then
		self:WriteStatic(self.vbStartNew, vbCount, self.idxOffset, self.idxCountNew, ...)
	end
	return vbCount, self.idxCount
end

function Mesh:WriteDynamic1(...)
	local vbCount, idxCount = self.dynamicFunc(self.dynamicData, ...)
	self:WriteDynamic(vbCount, ...)
	return vbCount, idxCount
end

function Mesh:WriteDynamic2(...)
	local vbCount, idxCount = self.dynamicFunc(...)
	self:TransDynamic(vbCount, ...)
	return vbCount, idxCount
end

---Model---
Model = class(SceneObject)
Model.writeId = true
function Model:ctor(geom)
	self.dropId = WeakTable()
	self.meshes = {}
	self.trans = geom.trans
	for _, m in pairs(geom.meshes) do
		local mesh = Mesh(self.id, geom, m.vbStart, m.vbEnd, m[1], m[2])
		table.insert(self.meshes, mesh)
		mesh.renderer:SetMaterial(m[3])
		local inst = m[3].inst
		if (inst.type == PerObjectInstance) then
			local pos = inst.objPos[self] or inst:AddObject(self)
			mesh.Trans = Mesh.GPUTrans
			mesh:SetInstArgs(inst, pos.pos, 1)
		else
			mesh.Trans = Mesh.CPUTrans
			if (inst.args) then
				mesh:SetInstArgs(inst, inst.args[1], inst.args[2])
			end
		end
	end
	self.RenderMeshes = geom.renderFunc
	self.pickable = {}
	self:Reschedule()
	if (EDITOR) then
		self:ShowPicked(false)
	end
end

function Model:SetMaterial(idx, mtl)
	local m = self.meshes[idx]
	if (m) then
		local inst = mtl.inst
		if (inst.type == PerObjectInstance) then
			local pos = inst.objPos[self] or inst:AddObject(self)
			m.Trans = Mesh.GPUTrans
			m:SetInstArgs(inst, pos.pos, 1)
		else
			m.Trans = Mesh.CPUTrans
			if (inst.args) then
				m:SetInstArgs(inst, inst.args[1], inst.args[2])
			end
		end
		m.renderer:SetMaterial(mtl)
		m.renderer:EnableWriteId(self.writeId)
		self:Reschedule()
	end	
end

function Model:SetCustomMesh(idx, func, data)
	local m = self.meshes[idx] 
	if (m) then
		local b = not func == not m.dynamicFunc
		m.dynamicFunc = func
		m.dynamicData = data
		if (data) then
			m.renderer.reader = Mesh.WriteDynamic1
		else
			m.renderer.reader = Mesh.WriteDynamic2
		end
		if (b) then
			self:Reschedule()
		end
	end
end

function Model:SetMeshInstArgs(idx, inst, start, count)
	local m = self.meshes[idx]
	if (m and inst.type ~= PerObjectInstance) then
		m:SetInstArgs(inst, start, count)
	end	
end

function Model:EnableWriteId(flag)
	if (self.writeId ~= flag) then
		for _, m in pairs(self.meshes) do
			m.renderer:EnableWriteId(flag)
		end
		self.writeId = flag
	end
end

function Model:EnableSubpass(spId, flag, meshIdx)
	if (meshIdx) then
		local m = self.meshes[meshIdx] 
		if (m) then
			m.renderer:EnableSubpass(spId, flag)
		end
	else
		for _, m in pairs(self.meshes) do
			m.renderer:EnableSubpass(spId, flag)
		end
	end
end

function Model:ShowPicked(flag)
	if (self.picked ~= flag) then
		self:EnableSubpass(g_rp0[3], flag)
		self:EnableSubpass(g_rp1[1], flag)
		self.picked = flag
	end
end
	
function Model:Update()
	SceneObject.Update(self)
	for _, v in pairs(self.pmInst) do
		v:SetObjectMatrix(self)
	end
end

function Model:Render(scene)
	self:RenderMeshes(scene)
	self.trans:MatrixTransform(self.mModel)
end

function Model:Reschedule()
	self.pmInst = {}
	local k, m = next(self.meshes)
	m.vbStartNew = m.vbStart
	m.vbEndNew = m.vbEnd
	m.idxCountNew = m.idxCount
	local mtl1 = m.renderer.mtl
	local inst = mtl1.inst
	if (inst.type == PerObjectInstance) then
		table.insert(self.pmInst, inst)
	end
	while (m) do
		k, n = next(self.meshes, k)
		if (not k) then
			break
		end
		local mtl2 = n.renderer.mtl
		if (mt12.inst ~= inst1 and mtl2.inst.type == PerObjectInstance) then
			table.insert(self.pmInst, mtl2.inst)
			inst = mtl2.inst
		end
		if (not m.dynamicFunc) then
			if (mtl1.vbLayout == mtl2.vbLayout and mtl1.inst.type == mtl2.inst.type) then
				m.vbStartNew = math.min(m.vbStartNew, n.vbStart)
				m.vbEndNew = math.max(m.vbEndNew, n.vbEnd)
				m.idxCountNew = m.idxCountNew + n.idxCount
				n.vbStartNew = 0
				n.vbEndNew = 0
				n.idxCountNew = 0
			else
				m = n
				m.vbStartNew = m.vbStart
				m.vbEndNew = m.vbEnd
				m.idxCountNew = m.idxCount
			end
		end
		mtl1 = mtl2
	end
end

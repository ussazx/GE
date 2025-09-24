---graphic.lua---
require 'defines'
require 'class'

---SubpassId---
local subpassId = 0
local passTable = {}
function SubpassId(pass, subIdx)
	local p = passTable[pass] or {}
	passTable[pass] = p
	local id = p[subIdx] or subpassId
	p[subIdx] = id
	subpassId = subpassId + 1
	return id
end

---VBLayout---
g_vbNull = CMBuffer(16)
local g_vbLayouts = {}
function NewVBLayout(fields, ...)
	local o = {...}
	local strides = {}
	local i = 1
	local j = nil
	while (i <= fields and i ~= 0) do
		if (i & fields ~= 0) then
			j, strides[i] = next(o, j)
		end
		i = i << 1
	end
	local same
	local layout
	for _, v in pairs(g_vbLayouts) do
		if (fields == v.fields) then
			layout = v
			i = 1
			while (i <= fields and i ~= 0) do
				if (i & fields ~= 0) then
					if (strides[i] == v.strides[i]) then
						same = true
					else
						same = false
						break
					end
				end
				i = i << 1
			end
			if (same) then
				return layout 
			end
		end
	end
	o = {fields = fields, strides = strides}
	table.insert(g_vbLayouts, o)
	return o
end

---RenderCommand---
local g_renderCmdPool = {}
local g_nRenderCmdPool = 0

RenderCommand = class()

local function GetIndBuf(input)
	input.indUsed = input.indUsed + 1
	if (input.indUsed <= input.indCap) then
		return input.indBuf[input.indUsed]
	end
	local indBuf = cGI:NewDrawIndirectCmd(1)
	input.indCap = input.indCap + 1
	input.indBuf[input.indCap] = indBuf
	return indBuf
end

function Command.NewRenderCmd()
	local cmd = g_renderCmdPool[1]
	if (cmd) then
		table.remove(g_renderCmdPool, 1)
		g_nRenderCmdPool = g_nRenderCmdPool - 1
		return cmd
	end
	cmd = RenderCommand()
	cmd.rIdx = 0
	cmd.cIdx = 0
	cmd.rbPos = 0
	
	cmd.input = {[0] = {vtx = {}, idxCount = 0, ib = cGI:NewBuffer(SIZE_INDEX), indUsed = 0, indCap = 0, indBuf = {}, GetIndBuf = GetIndBuf},
				 [1] = {vtx = {}, idxCount = 0, ib = cGI:NewBuffer(SIZE_INDEX), indUsed = 0, indCap = 0, indBuf = {}, GetIndBuf = GetIndBuf}}
	
	
	cmd[0] = cGI:NewCommand(false)
	cmd[0].gb = cGI:NewBuffer(128)
	cmd[0].cIdx = 0
	
	cmd[1] = cGI:NewCommand(false)
	cmd[1].gb = cGI:NewBuffer(128)
	cmd[1].cIdx = 1
	
	cmd.cmd = cmd[0]
	cmd.gb = cmd.cmd.gb

	for _, layout in pairs(g_vbLayouts) do
		local wp = {idxAddOn = 0}
		local o0 = {wp = wp, vb = {}, vbSet = cGI:NewBufferSet(nil, 0)}
		local o1 = {wp = wp, vb = {}, vbSet = cGI:NewBufferSet(nil, 0)}
		for k, s in pairs(layout.strides) do
			wp[k] = 0
		
			local vb = cGI:NewBuffer(s)
			o0.vb[k] = vb
			o0.vbSet:Add(vb)
			
			vb = cGI:NewBuffer(s)
			o1.vb[k] = vb
			o1.vbSet:Add(vb)
		end
		cmd.input[0].vtx[layout] = o0
		cmd.input[1].vtx[layout] = o1
	end
	return cmd
end

function RenderCommand:Reset()
	if (self.rendered) then
		self.rIdx = ~self.rIdx & 1
		self.rendered = false
		
	end
	local input = self.input[self.rIdx]
	input.idxCount = 0
	input.indUsed = 0
	for _, v in pairs(input.indBuf) do
		v:Reset()
	end
	for _, v in pairs(input.vtx) do
		for k, _ in pairs(v.wp) do
			v.wp[k] = 0
		end
	end
	return input
end

function RenderCommand:Execute()
	local cmd0 = self[self.cIdx]
	self.cIdx = ~self.cIdx & 1
	local cmd1 = self[self.cIdx]
	cmd0:Execute()
	cmd1:Wait()
	if (cmd0.gb.updated) then
		CBufferCopy(cmd0.gb, 0, self.rbPos, cmd1.gb, 0)
		cmd0.gb.updated = false
	end
	self.cmd = cmd1
	self.gb = self.cmd.gb
	self.rendered = true
end

function RenderCommand.Recycle(cmd)
	if (g_nRenderCmdPool < 10) then
		table.insert(g_renderCmdPool, cmd)
		g_nRenderCmdPool = g_nRenderCmdPool + 1
	end
end

---ResBuffer---
local ResBufferMT = {__call = 
function(b)
	local gb = b.cmd.gb
	gb.updated = true
	return gb
end}

function ResBuffer(cmd, ...)
	local b = setmetatable({}, ResBufferMT)
	b.cmd = cmd
	b.size = 0
	b.offset = cmd.rbPos
	for k, v in pairs({...}) do
		b[k] = b.offset + b.size
		b.size = b.size + v
	end
	cmd.rbPos = cmd.rbPos + (b.size + 0xFF) // 0x100 * 0x100
	cmd[0].gb:Resize(cmd.rbPos)
	cmd[1].gb:Resize(cmd.rbPos)
	return b
end

---ResourceSetNew---
ResourceSetNew = class()

function ResourceSetNew:ctor(rl)
	self[0] = rl:NewResourceSet()
	self[1] = rl:NewResourceSet()
end

function ResourceSetNew:BindResBuffer(b, binding)
	self[0]:BindBuffer(b.cmd[0].gb, b.offset, b.size, binding, cGI.RESOURCE_TYPE_UNIFORM_BUFFER)
	self[1]:BindBuffer(b.cmd[1].gb, b.offset, b.size, binding, cGI.RESOURCE_TYPE_UNIFORM_BUFFER)
end

function ResourceSetNew:BindTexelView(v, binding)
	self[0]:BindBuffer(v, 0, cGI.WHOLE_SIZE, 0, cGI.RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER)
	self[1]:BindBuffer(v, 0, cGI.WHOLE_SIZE, 0, cGI.RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER)
end

---DrawcallList---
local function SetViewport(op, cmd)
	cmd:SetViewport(op.x, op.y, op.w, op.h, 0, 1)
end

local function SetScissor(op, cmd)
	cmd:SetScissor(op.x, op.y, op.w, op.h)
end

local function SetVertexBuffers(op, cmd)
	cmd:SetVertexBuffers(op.vtxInput, op.slot)
end

local function SetResourceSet(op, cmd)
	cmd:SetResourceSet(op.res[cmd.cIdx], op.idx)
end

local function SetupSubList(op, cmd)
	op.list:SetupDrawcalls(cmd)
end

local function DrawIndexed(op, cmd)
	cmd:DrawIndexed(op.pl, op.ib, 0, op.idxStart,op.idxCount, op.instStart, op.instCount)
end

local function DrawIndexedIndirect(op, cmd)
	cmd:DrawIndexedIndirect(op.pl, op.ib, op.indBuf, op.indStart, op.indCount)
end

local function ClearSwapchain(op, cmd)
	cmd:ClearSwapchain(op.x, op.y, op.w, op.h, op.r, op.g, op.b, op.a)
end

local function ClearViewFloat4(op, cmd)
	cmd:ClearViewFloat4(op.idx, op.x, op.y, op.w, op.h, op.r, op.g, op.b, op.a)
end

local function ClearViewUint4(op, cmd)
	cmd:ClearViewUint4(op.idx, op.x, op.y, op.w, op.h, op.r, op.g, op.b, op.a)
end

local function ClearDepthStencil(op, cmd)
	cmd:ClearDepthStencil(op.idx, op.x, op.y, op.w, op.h, op.d, op.s)
end

DrawcallList = class()

function DrawcallList:ctor()
	self.d_rs = {}
	self.d_res = {}
	self.d_insVbSets = {}
	self.opList = {}
end

function DrawcallList:Reset(input)
	self.indBuf = input:GetIndBuf()
	self.input = input
	self.idxStart = 0
	self.idxCount = 0
	self.indStart = 0 
	self.indCount = 0
	self.opIdx = 0
	self.resIdx = 0

	self.rs = self.d_rs
	self.rs.vp = nil
	self.rs.sc = nil 
	self.rs.pl = nil
	self.rs.vbMain = nil
	self.res = self.d_res
	for k, _ in pairs(self.res) do
		self.res[k] = nil
	end
	self.insVbSets = self.d_insVbSets
	for k, _ in pairs(self.insVbSets) do
		self.insVbSets[k] = nil
	end
end

function DrawcallList:NewOP()
	self.opIdx = self.opIdx + 1
	local op = self.opList[self.opIdx]
	if (not op) then
		op = {}
		self.opList[self.opIdx] = op
	end
	return op
end

function DrawcallList:ClearSwapchain(x, y, w, h, r, g, b, a)
	self:CommitDrawcall()
	op = self:NewOP()
	op.func = ClearSwapchain
	op.x = x
	op.y = y
	op.w = w
	op.h = h
	op.r = r
	op.g = g
	op.b = b
	op.a = a
end

function DrawcallList:ClearViewFloat4(idx, x, y, w, h, r, g, b, a)
	self:CommitDrawcall()
	op = self:NewOP()
	op.func = ClearViewFloat4
	op.idx = idx
	op.x = x
	op.y = y
	op.w = w
	op.h = h
	op.r = r
	op.g = g
	op.b = b
	op.a = a
end

function DrawcallList:ClearViewUint4(idx, x, y, w, h, r, g, b, a)
	self:CommitDrawcall()
	op = self:NewOP()
	op.func = ClearViewUint4
	op.idx = idx
	op.x = x
	op.y = y
	op.w = w
	op.h = h
	op.r = r
	op.g = g
	op.b = b
	op.a = a
end

function DrawcallList:ClearDepthStencil(idx, x, y, w, h, d, s)
	self:CommitDrawcall()
	op = self:NewOP()
	op.func = ClearDepthStencil
	op.idx = idx
	op.x = x
	op.y = y
	op.w = w
	op.h = h
	op.d = d
	op.s = s
end

function DrawcallList:SetViewport(x, y, w, h)
	local rs = self.rs
	local op = rs.vp
	if (not op or op.x ~= x or op.y ~= y or op.w ~= w or op.h ~= h) then
		self:CommitDrawcall()
		op = self:NewOP()
		op.func = SetViewport
		op.x = x
		op.y = y
		op.w = w
		op.h = h
		rs.vp = op
	end
end

function DrawcallList:SetScissor(x, y, w, h)
	local rs = self.rs
	local op = rs.sc
	if (not op or op.x ~= x or op.y ~= y or op.w ~= w or op.h ~= h) then
		self:CommitDrawcall()
		op = self:NewOP()
		op.func = SetScissor
		op.x = x
		op.y = y
		op.w = w
		op.h = h
		rs.sc = op
	end
end

function DrawcallList:SetPipeline(pl, vbLayout, slot)
	local rs = self.rs
	if (pl ~= self.rs.pl) then
		self:CommitDrawcall()
	end
	rs.pl = pl
	
	local vtxInput = self.input.vtx[vbLayout].vbSet
	local op = rs.vbMain
	if (not op or op.vtxInput ~= vtxInput or op.slot ~= slot) then
		self:CommitDrawcall()
		op = self:NewOP()
		op.func = SetVertexBuffers
		op.vtxInput = vtxInput
		op.slot = slot
		rs.vbMain = op
	end
end

function DrawcallList:SetInsVB(vbSet, slot)
	local insVbSets = self.insVbSets
	local op = insVbSets[slot]
	if (not op or op.vtxInput ~= vbSet) then
		self:CommitDrawcall()
		op = self:NewOP()
		op.func = SetVertexBuffers
		op.vtxInput = vbSet
		op.slot = slot
		insVbSets[slot] = op
	end
end

function DrawcallList:AddResourceSet(res)
	local resIdx = self.resIdx
	self.resIdx = self.resIdx + 1
	local resSet = self.res
	local op = resSet[resIdx]
	if (not op or op.res ~= res) then
		self:CommitDrawcall()
		op = self:NewOP()
		op.func = SetResourceSet
		op.res = res
		op.idx = resIdx
		resSet[resIdx] = op
	end
end

function DrawcallList:AddSubList(dcList)
	self:CommitDrawcall()
	self.rs = dcList.rs
	self.res = dcList.res
	self.insVbSets = dcList.insVbSets
	local op = self:NewOP()
	op.func = SetupSubList
	op.list = dcList
end

function DrawcallList:SetupDrawcalls(cmd)
	self:CommitDrawcall()
	for i = 1, self.opIdx do
		local op = self.opList[i]
		op:func(cmd)
	end
end

function DrawcallList:CommitDrawcall()
	local op
	if (self.indCount > 0) then
		self:CommitIndBuffer()
		op = self:NewOP()
		op.func = DrawIndexedIndirect
		op.indStart = self.indStart
		op.indCount = self.indCount
		op.indBuf = self.indBuf
		
		self.indStart = self.indStart + self.indCount
		self.indCount = 0
	elseif (self.idxCount > 0) then
		op = self:NewOP()
		op.func = DrawIndexed
		op.idxStart = self.idxStart
		op.idxCount = self.idxCount
		op.instStart = self.instStart
		op.instCount = self.instCount
		op.func = DrawIndexed
		
		self.idxCount = 0
	else
	return end
	op.pl = self.rs.pl
	op.ib = self.input.ib
end

function DrawcallList:CommitIndBuffer()
	if (self.idxCount > 0) then
		self.indBuf:AddDrawIndexed(0, self.idxStart, self.idxCount, self.instStart, self.instCount)
		self.indCount = self.indCount + 1
		self.idxCount = 0
	end
end

function DrawcallList:Draw(idxStart, idxCount, instStart, instCount)
	if (self.idxStart + self.idxCount ~= idxStart or self.instStart ~= instStart or self.instCount ~= instCount) then
		self:CommitIndBuffer()
	end
	if (self.idxCount == 0) then
		self.idxStart = idxStart
	end
	self.instStart = instStart
	self.instCount = instCount
	self.idxCount = self.idxCount + idxCount
	
	self.resIdx = 0
end

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
	self.trans = CTransformer()
	local write = 'return function(self, vbStart, vbCount, idxOffset, idxCount'
	for k, v in pairs(o.vb) do
		write = write .. string.format(', vbDst%q, wp%q', k, k)
	end
	write = write .. ', ib, iwp, ibStart) local vbSrc = self.vb '
	for k, v in pairs(o.vb) do
		if (v[2] == Geometry.TRANS_DEFAULT or v[2] == Geometry.TRANS_NORMAL) then
			local isNormal = v[2] == Geometry.TRANS_NORMAL
			write = write .. string.format('self.trans:AddFactors(vbSrc[%q][1], 0, vbStart, vbCount, vbDst%q, wp%q, %q) ', k, k, k, isNormal)
		else
			write = write .. 
			string.format('CBufferCopy(vbSrc[%q][1], vbStart * SIZE_FLOAT3, vbCount * SIZE_FLOAT3, vbDst%q, wp%q) ', k, k, k)
		end
	end
	write = write .. 'CCopyIndexBuffer(self.ib, idxOffset, idxCount, ibStart - vbStart, ib, iwp) end '
	self.Write = load(write, 'Write', 't')()
end

---Mesh---
Mesh = class()

function Mesh:ctor(geom, idxOffset, idxCount)
	self.geom = geom
	self.vbStart, self.vbEnd = CGetIndicesSegment(geom.ib, 0, idxOffset, idxCount)
	self.idxOffset = idxOffset
	self.idxCount = idxCount
	self.renderer = Renderer(self, self.geom.layout, self.Write)
end

function Mesh:Write(...)
	local vbCount = self.vbEndNew - self.vbStartNew
	if (vbCount > 0) then
		vbCount = vbCount + 1
		self.geom:Write(self.vbStartNew, vbCount, self.idxOffset, self.idxCountNew, ...)
	end
	return vbCount, self.idxCount
end

---Model---
Model = class()
Model.Renderer = {}
function Model:ctor(geom)
	self.geom = geom
	self.matrix = CMatrix3D()
	self.meshes = {}
	for _, v in pairs(geom.meshes) do
		local mesh = Mesh(geom, v[1], v[2])
		table.insert(self.meshes, mesh)
		mesh.renderer:SetMaterial(v[3])
	end
	self.RenderMeshes = Model.Renderer[geom]
	if (not self.RenderMeshes) then
		local renderer = 'return function(self, scene) local m = self.meshes'
		for k, v in pairs(self.meshes) do
			renderer = string.format('%s m[%q].renderer:Render(scene)', renderer, k)
		end
		renderer = load(renderer .. ' end', 'renderer', 't')()
		Model.Renderer[geom] = renderer
		self.RenderMeshes = renderer
	end
	self:Reschedule()
end

function Model:SetMaterial(idx, ...)
	local m = self.meshes[idx]
	if (m) then
		m.renderer:SetMaterial(...)
		self:Reschedule()
	end
end

function Model:Render(scene)
	self:RenderMeshes(scene)
	self.geom.trans:MatrixTransform(self.matrix)
end

function Model:Reschedule()
	local k, m = next(self.meshes)
	m.vbStartNew = m.vbStart
	m.vbEndNew = m.vbEnd
	m.idxCountNew = m.idxCount
	while (m) do
		k, n = next(self.meshes, k)
		if (not k) then
			break
		end
		if (m.renderer.mtl.vbLayout == n.renderer.mtl.vbLayout) then
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
end

---Renderer---
Renderer = class()
Renderer.UpdateCached = {}
Renderer.VtxHandler = {}
Renderer.VtxHandlerCached = {}

function Renderer:ctor(mesh, fields, reader, doCache)
	self.mesh = mesh
	self.reader = reader
	self.fields = fields
	self.doCache = doCache
	self.insArgs = {}
	self.n_vtx = 0
	self.n_idx = 0
	
	if (doCache) then
		self.copy = CBufferCopy
		Renderer.VtxHandlerCached[fields] = Renderer.VtxHandlerCached[fields] or {}
		self.vb = {}
		local c = 'local vb = o.vb o.n_vtx, o.n_idx = o.reader(o.mesh, '
		local f = Renderer.UpdateCached[fields]
		local i = 1
		while (i <= fields and i ~= 0) do
			if (i & fields ~= 0) then
				self.vb[i] = CMBuffer(8)
				if (f == nil) then
					c = c .. 'vb[' .. i ..'], 0, '
				end
			end
			i = i << 1
		end
		c = c .. 'o.ib, 0, 0)'
		self.ib = CMBuffer(SIZE_INDEX)
		self.Render = self.RenderCached
		if (f == nil) then
			f = load(c, '', 't', Renderer)
			Renderer.UpdateCached[fields] = f
		end
		self.updateCached = f
	else
		Renderer.VtxHandler[fields] = Renderer.VtxHandler[fields] or {}
	end
	self.update = true
end

function Renderer:Render(scene, disables)
	self.vwp = g_input.vtx[self.mtl.vbLayout].wp
	disables = disables or {}
	Renderer.o = self
	local mtl = self.mtl
	self.vbDst = g_input.vtx[mtl.vbLayout].vb
	self.ibDst = g_input.ib
	self.iwp = g_input.idxCount * SIZE_INDEX
	self.ibStart = self.vwp.idxAddOn
	local draw = true
	local iwp = g_input.idxCount * SIZE_INDEX
	for spId, func in pairs(mtl.func) do
		local dcList = scene:GetDrawcall(spId, func.mergeType, func.order)
		if (not disables[spId] and dcList) then
			func.func(dcList)
			if (draw) then
				self.vtxHandler()
				draw = false
			end
			local insArgs = self.insArgs[mtl.insSlot[spId]]
			dcList:Draw(g_input.idxCount, self.n_idx, insArgs[1], insArgs[2])
		end
	end
	self.vwp.idxAddOn = self.vwp.idxAddOn + self.n_vtx
	g_input.idxCount = g_input.idxCount + self.n_idx
	self.n_vtx = 0
	self.n_idx = 0
end

function Renderer:RenderCached(scene, disables)
	self.vwp = g_input.vtx[self.mtl.vbLayout].wp
	disables = disables or {}
	Renderer.o = self
	if (self.update) then
		self.updateCached()
		self.update = false
	end
	local mtl = self.mtl
	self.vbDst = g_input.vtx[mtl.vbLayout].vb
	local draw = false
	local iwp = g_input.idxCount * SIZE_INDEX
	for spId, func in pairs(mtl.func) do
		local dcList = scene:GetDrawcall(spId, func.mergeType, func.order)
		if (not disables[spId] and dcList) then
			func.func(dcList)
			local insArgs = self.insArgs[mtl.insSlot[spId]]
			dcList:Draw(g_input.idxCount, self.n_idx, insArgs[1], insArgs[2])
			draw = true
		end
	end
	if (draw) then
		self.vtxHandler()
		CCopyIndexBuffer(self.ib, 0, self.n_idx, self.vwp.idxAddOn, g_input.ib, iwp)
		self.vwp.idxAddOn = self.vwp.idxAddOn + self.n_vtx
		g_input.idxCount = g_input.idxCount + self.n_idx
	end
end

function Renderer:SetMaterial(mtl, ...)
	if (self.mtl == mtl) then
		return
	end
	self.mtl = mtl
	self.strides = mtl.vbLayout.strides
	local strides = self.strides
	self.insArgs = {...}
	for _, v in pairs(mtl.insSlot) do
		self.insArgs[v] = self.insArgs[v] or {0, 1}
	end
	
	local srcFields = self.fields
	local dstFields = mtl.vbLayout.fields
	local f
	if (self.doCache) then
		f = Renderer.VtxHandlerCached[srcFields][dstFields]
	else
		f = Renderer.VtxHandler[srcFields][dstFields]
	end
	if (f == nil) then
		local c = [[local copy = o.copy local vb = o.vb local n_vtx = o.n_vtx 
			local vbDst = o.vbDst local vwp = o.vwp local strides = o.strides ]]
		local c1 = ''
		local i = 1
		if (self.vb) then
			while (i <= srcFields and i ~= 0) do
				if (i & srcFields ~= 0) then
					if (strides[i]) then
						c = c .. 'local n_vtx'..i..' = n_vtx * strides['..i..']'
						c = c .. 'copy(vb['..i..'], 0, n_vtx'..i..', vbDst['..i..'], vwp['..i..'])'
						c1 = c1 .. 'vwp['..i..'] = vwp['..i..'] + n_vtx'..i..' '
					end
				end
				i = i << 1
			end
			i = 1
			local n = ~(srcFields & dstFields) & dstFields
			while (i <= n and i ~= 0) do
				if (i & n ~= 0) then
					local slot = mtl.slot[i]
					c1 = c1 .. string.format('vwp[%q] = vwp[%q] + n_vtx * strides[%q] ', slot, slot, slot)
				end
				i = i << 1
			end
			f = load(c..c1, '', 't', Renderer)
			Renderer.VtxHandlerCached[srcFields][dstFields] = f
		else
			c = [[local vbDst = o.vbDst local vwp = o.vwp local strides = o.strides
				o.n_vtx, o.n_idx = o.reader(o.mesh, ]]
			c1 = 'local n_vtx = o.n_vtx '
			while (i <= srcFields and i ~= 0) do
				if (i & srcFields ~= 0) then
					if (strides[i]) then
						c1 = c1 .. 'vwp['..i..'] = vwp['..i..'] + n_vtx * strides['..i..'] '
						c = c .. 'vbDst['.. i ..'], vwp['..i..'], '
					else
						c = c .. 'g_vbNull, 0, '
					end
				end
				i = i << 1
			end
			i = 1
			local n = ~(srcFields & dstFields) & dstFields
			while (i <= n and i ~= 0) do
				if (i & n ~= 0) then
					c1 = c1 .. string.format('vwp[%q] = vwp[%q] + n_vtx * strides[%q] ', i, i, i)
				end
				i = i << 1
			end
			c = c .. 'o.ibDst, o.iwp, o.ibStart) '
			f = load(c..c1, '', 't', Renderer)
			Renderer.VtxHandler[srcFields][dstFields] = f
		end
	end
	self.vtxHandler = f
end

---FramePipeline---
FramePipeline = class()

function FramePipeline:ctor()
	self.cmdList = {}
	self.dcLists = {}
	self.surfaces = {}
	self.subLists = {}
	self.subListUsed = 0
end

function FramePipeline:NewSubList()
	self.subListUsed = self.subListUsed + 1
	local list = self.subLists[self.subListUsed]
	if (not list) then
		list = DrawcallList()
		self.subLists[self.subListUsed] = list
	end
	return list
end

function FramePipeline:AddFrameOutput(fb)
	table.insert(self.cmdList, {type = 0, fb = fb})
end

function FramePipeline:AddCopyImage(params)
	table.insert(self.cmdList, {type = 1, params = params})
end

function FramePipeline:UpdateSurface(input)
	self.subListUsed = 0
	for surface, dcLists in pairs(self.foParams) do		
		for _, dcList in pairs(dcLists) do
			dcList:Reset(input)
		end
		g_surface = surface
		g_dcLists = dcLists
		g_input = input
		g_fp = self
		surface:Update(nil, surface.rect)
	end
end

function FramePipeline:FillCommand(cmd)
	self.cmd = cmd[cmd.cIdx]
	self.code()
end

function FramePipeline:SetSurface(s, spId)
	if (self.dcLists[spId] and self.surfaces[spId] ~= s) then
		local o = self.surfaces[spId]
		self.surfaces[spId] = s
		if (o) then
			local remove = true
			for _, v in pairs(self.surfaces) do
				if (o == v) then
					remove = false
					break
				end
			end
			if (remove) then
				self.foParams[o] = nil
			end
		end
		
		o = self.foParams[s] or {}
		o[spId] = self.dcLists[spId]
		self.foParams[s] = o
	end
end

function FramePipeline:Bake()
	self.foParams = {}
	
	local code = 'local c local o\n'
	for k, c in pairs(self.cmdList) do
		code = code .. 'c = cmdList[' .. k .. ']\n'
		if (c.type == 0) then
			code = code .. 'cmd:RenderBegin(c.fb, false)\n'
			
			local n = #c.fb.rp
			for i = 1, n do
				local spId = c.fb.rp[i]
				code = code .. 'dcLists['..spId..']:SetupDrawcalls(cmd)\n'
				if (i ~= n) then
					code = code..'cmd:NextSubpass(false)\n'
				end
				 self.dcLists[spId] = self.dcLists[spId] or DrawcallList()
			end
			code = code..'cmd:RenderEnd()\n'
		elseif (c.type == 1) then
			code = code..[[o = c.params cmd:CopyImage(o.srcView, o.srcLayer, o.src_x, o.src_y, 
			o.dstView, o.dstLayer, o.dst_x, o.dst_y, o.numLayers, o.w, o.h)]]
		end
	end
	self.code = load(code, '', 't', self)
end
			
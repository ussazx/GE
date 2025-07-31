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

---VtxLayout---
g_vbNull = CMBuffer(16)
local g_vtxLayout = {}
function NewVtxLayout(...)
	local o = {...}
	local n
	local layout
	local same = 0
	for _, v in pairs(g_vtxLayout) do
		n = #v
		layout = v
		if (#o == n) then
			for i = 1, n do
				if (o[i] == v[i]) then
					same = i
				else
					break
				end
			end
		end
	end
	if (same == n) then
		return layout 
	end
	table.insert(g_vtxLayout, o)
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

	for _, layout in pairs(g_vtxLayout) do
		local wp = {idxAddOn = 0}
		local o0 = {wp = wp, vb = {}, vbSet = cGI:NewBufferSet(nil, 0)}
		local o1 = {wp = wp, vb = {}, vbSet = cGI:NewBufferSet(nil, 0)}
		for k, s in pairs(layout) do
			wp[k] = 0
		
			local vb = cGI:NewBuffer(s)
			table.insert(o0.vb, vb)
			o0.vbSet:Add(vb)
			
			vb = cGI:NewBuffer(s)
			table.insert(o1.vb, vb)
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
		b[k] = cmd.rbPos
		cmd.rbPos = cmd.rbPos + v
		b.size = b.size + v
	end
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
DrawcallList = class()

function DrawcallList:ctor()
	self.d = {}
	self.c = {}
	self.p = {}
	self.res = {}
	self.dcList = {}
	self.dcCap = 0
	self.insVbSets = {}
	self.dcCount = 0
	self.idxCount = 0
end

function DrawcallList:Reset(input)
	self.indBuf = input:GetIndBuf()
	self.input = input
	self.idxStart = 0
	self.idxCount = 0
	self.indStart = 0 
	self.indCount = 0
	self.dcIdx = 0
	self.dcCount = 0
	self.dc = self:NewDrawcall()
	self.p.vp = nil
	self.p.cr = nil
	self.p.pl = nil
	self.p.vtxInput = nil
	self.p.mainSlot = nil
	self.mtl = nil
	self.resIdx = 0
	for k, _ in pairs(self.res) do
		self.res[k] = nil
	end
	for k, _ in pairs(self.insVbSets) do
		self.insVbSets[k] = nil
	end
	self.reset = true
end

function DrawcallList:NewDrawcall()
	self.dcIdx = self.dcIdx + 1
	local dc
	if (self.dcIdx <= self.dcCap) then
		dc = self.dcList[self.dcIdx]
		dc.vp = nil
		dc.cr = nil
		dc.mainSlot = nil
		for k, _ in pairs(dc.resList) do
			dc.resList[k] = nil
		end
		for k, _ in pairs(dc.vbSets) do
			dc.vbSets[k] = nil
		end
	else
		dc = {resList = {}, vbSets = {}, vtxOffsets = {}}
		self.dcCap = self.dcCap + 1
		self.dcList[self.dcCap] = dc
	end
	dc.direct = false
	dc.nOffsets = 0
	dc.resCount = 0
	dc.indStart = self.indStart
	dc.indCount = 0
	return dc
end

function DrawcallList:SetupDrawcall(cmd, dc)
	if (dc.vp) then
		cmd:SetViewport(dc.vp.x, dc.vp.y, dc.vp.w, dc.vp.h, 0, 1)
	end
	if (dc.cr) then
		cmd:SetClipRect(dc.cr.x, dc.cr.y, dc.cr.w, dc.cr.h)
	end
	for k, v in pairs(dc.resList) do
		cmd:SetResourceSet(v[cmd.cIdx], k)
	end
	for k, v in pairs(dc.vbSets) do
		cmd:SetVertexBuffers(v, k)
	end
	if (dc.mainSlot) then
		cmd:SetVertexBuffers(dc.vtxInput, dc.mainSlot)
	end
	if (dc.direct) then
		cmd:DrawIndexed(dc.pl, self.input.ib, 0, dc.idxStart, dc.idxCount, dc.instStart, dc.instCount)
	else
		cmd:DrawIndexedIndirect(dc.pl, self.input.ib, self.indBuf, dc.indStart, dc.indCount)
	end
end

function DrawcallList:SetupDrawcalls(cmd)
	self:CommitCurrent()
	for i = 1, self.dcCount do
		local dc = self.dcList[i]
		if (self.subList) then
			self.subList:SetupDrawcalls(cmd)
			self.subList = nil
		else
			self:SetupDrawcall(cmd, dc)
		end
	end
	self.reset = false
	--Print('---draw call---', self.dcCount)
end

function DrawcallList:SetPipeline(pl, vtxLayout, slot)
	self.c.pl = pl
	self.c.vtxInput = self.input.vtx[vtxLayout].vbSet
	self.c.mainSlot = slot
end

function DrawcallList:SetInsVB(vbSet, slot)
	if (self.insVbSets[slot] ~= vbSet) then
		self:CommitCurrent()
		self.insVbSets[slot] = vbSet
		self.dc.vbSets[slot] = vbSet
	end
end

function DrawcallList:AddResourceSet(res)
	if (self.res[self.resIdx] ~= res) then
		self:CommitCurrent()
		self.res[self.resIdx] = res
		self.dc.resList[self.resIdx] = res
	end
	self.resIdx = self.resIdx + 1
end

function DrawcallList:CommitStates()
	if (self.c.pl ~= self.p.pl) then
		self:CommitCurrent()
		self.p.pl = self.c.pl
	end
	self.dc.pl = self.c.pl
	
	if (self.c.vtxInput ~= self.p.vtxInput or self.c.mainSlot ~= self.p.mainSlot) then
		self:CommitCurrent()
		self.p.vtxInput = self.c.vtxInput
		self.p.mainSlot = self.c.mainSlot
		self.dc.vtxInput = self.c.vtxInput
		self.dc.mainSlot = self.c.mainSlot
	end
	
	if (DrawcallList.vp ~= self.p.vp) then
		self:CommitCurrent()
		self.p.vp = DrawcallList.vp
		self.dc.vp = DrawcallList.vp
	end
	
	if (DrawcallList.cr ~= self.p.cr) then
		self:CommitCurrent()
		self.p.cr = DrawcallList.cr
		self.dc.cr = DrawcallList.cr
	end
	
	self.resIdx = 0
end

function DrawcallList:CommitCurrent()
	if (not self.dc) then 
	return end
	if (self.indCount > 0) then
		self:CommitIndBuffer()
		self.dc.indCount = self.indCount
		self.indStart = self.indStart + self.indCount
		self.indCount = 0
	elseif (self.idxCount > 0) then
		self.dc.direct = true
		self.dc.idxStart = self.idxStart
		self.dc.idxCount = self.idxCount
		self.dc.instStart = self.instStart
		self.dc.instCount = self.instCount
		self.idxCount = 0
	elseif (not self.subList) then 
	return end
	self.dc.pl = self.c.pl
	self.dcCount = self.dcCount + 1
	self.dc = self:NewDrawcall()
end

function DrawcallList:CommitIndBuffer()
	if (self.idxCount > 0) then
		self.indBuf:AddDrawIndexed(0, self.idxStart, self.idxCount, self.instStart, self.instCount)
		self.indCount = self.indCount + 1
		self.idxCount = 0
	end
end

function DrawcallList:Draw(idxStart, idxCount, instStart, instCount)
	self:CommitStates()
	if (self.idxStart + self.idxCount ~= idxStart or self.instStart ~= instStart or self.instCount ~= instCount) then
		self:CommitIndBuffer()
	end
	if (self.idxCount == 0) then
		self.idxStart = idxStart
	end
	self.instStart = instStart
	self.instCount = instCount
	self.idxCount = self.idxCount + idxCount
end

function DrawcallList:AddSubList(dcList)
	self:CommitCurrent()
	dc.subList = dcList
	self:CommitCurrent()
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
	local write = 'return function(self, vbStart, vbCount, idxOffset, idxCount '
	for k, v in pairs(o.vb) do
		write = write .. string.format(', vbDst%q, wp%q', k, k)
	end
	write = write .. ', ib, iwp, ibStart) local vbSrc = self.vb '
	for k, v in pairs(o.vb) do
		if (v[2] == Geometry.TRANS_DEFAULT or v[2] == Geometry.TRANS_NORMAL) then
			local isNormal = v[2] == Geometry.TRANS_NORMAL
			write = write .. 
			string.format('self.trans:AddFactors(vbStart, vbCount, vbSrc[%q], 0, vbDst%q, wp%q, %q) ', k, k, k, isNormal)
		else
			write = write .. 
			string.format('CBufferCopy(vbSrc[%q], vbStart * SIZE_FLOAT3, vbCount * SIZE_FLOAT3, vbDst%q, wp%q) ', k, k, k)
		end
	end
	write = write .. 'CCopyIndexBuffer(self.ib, idxOffset, idxCount, ibStart - vbStart, ib, iwp) end '
	self.Write = load(write, '', 't')
end

local geoInfo = {}
geoInfo.layout = 1|2
geoInfo.vb = {}
geoInfo.vb[1] = {CMBuffer(1), Geometry.TRANS_DEFAULT}
geoInfo.vb[2] = {CMBuffer(1), Geometry.TRANS_NORMAL}
geoInfo.vb[3] = {CMBuffer(1), Geometry.TRANS_NONE}
geoInfo.ib = CMBuffer(1)
geoInfo.meshes = {}
geoInfo.meshes[1] = {0, 24, defMtl}
local z = Geometry(geoInfo)


--function Geometry:Write(vbSrc0, vbSrc1, idxStart, count, skinning, skeleton, vbDst, vwb)

---Mesh---
Mesh = class()

function Mesh:ctor(geom, idxOffset, idxCount)
	self.geom = geom
	self.vbStart, self.vbEnd = CGetIndicesSegment(geom.ib, 0, idxOffset, idxCount)
	self.idxOffset = idxOffset
	self.idxCount = idxCount
	self.renderer = MeshRenderer(self, self.geom.layout, self.Write)
	self.vbCount = self.vbEnd - self.vbCount + 1
end

function Mesh:Write(...)
	local vbCount = self.vbEndNew - self.vbStartNew + 1
	if (vbCount > 1) then
		self.geom:Write(self.vbEndNew, self.vbCount, self.idxOffset, self.idxCountNew, ...)
	end
	return vbCount, self.idxCountNew
end

---Model---
Model = class()
Model.Renderer = {}
function Model:ctor(geom)
	self.matrix = CMatrix3D()
	self.meshes = {}
	for _, v in pairs(geom) do
		table.insert(self.meshes, Mesh(geom, v[1], v[2]))
		Mesh.renderer:SetMaterial(v[3])
	end
	self.RenderMeshes = Model.Renderer[geom]
	if (not self.Renderer) then
		local renderer = 'return function(self, scene) local m = self.meshes '
		for k, v in pairs(self.meshes) do
			renderer = renderer .. string.format('m[%q].renderer:Render(scene) ', k)
		end
		renderer = load(renderer .. 'end', '', 't')
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
	self.geom:MatrixTransform(self.matrix)
end

function Model:Reschedule()
	local k, m = next(self.meshes)
	while (m) do
		m.vbStartNew = m.vbStart
		m.vbEndNew = m.vbEnd
		m.idxCountNew = m.idxCount
		k, n = next(self.meshes, k)
		if (not k) then
			return
		end
		if (m.renderer.mtl.layout == n.renderer.mtl.layout) then
			m.vbStartNew = math.min(m.vbStartNew, n.vbStart)
			m.vbEndNew = math.max(m.vbEndNew, n.vbEnd)
			m.idxCountNew = n.idxCount
			n.vbStartNew = 0
			n.vbEndNew = 0
		else
			m = n
		end
	end
end

---MeshRenderer---
MeshRenderer = class()
MeshRenderer.UpdateCached = {}
MeshRenderer.VtxHandler = {}
MeshRenderer.VtxHandlerCached = {}

function MeshRenderer:ctor(mesh, layout, reader, doCache)
	self.mesh = mesh
	self.reader = reader
	self.layout = layout
	self.doCache = doCache
	self.insArgs = {}
	
	if (doCache) then
		self.copy = CBufferCopy
		MeshRenderer.VtxHandlerCached[layout] = MeshRenderer.VtxHandlerCached[layout] or {}
		self.vb = {}
		local c = 'local vb = o.vb o.n_vtx, o.n_idx = o.reader(o.mesh, '
		local f = MeshRenderer.UpdateCached[layout]
		local i = 1
		while (i <= layout and i ~= 0) do
			if (i & layout ~= 0) then
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
			f = load(c, '', 't', MeshRenderer)
			MeshRenderer.UpdateCached[layout] = f
		end
		self.updateCached = f
	else
		MeshRenderer.VtxHandler[layout] = MeshRenderer.VtxHandler[layout] or {}
	end
	self.update = true
end

function MeshRenderer:Render(scene, disables)
	self.vwp = g_input.vtx[self.mtl.vtxLayout].wp
	disables = disables or {}
	MeshRenderer.o = self
	local mtl = self.mtl
	self.vbDst = g_input.vtx[mtl.vtxLayout].vb
	self.ibDst = g_input.ib
	self.iwp = g_input.idxCount * SIZE_INDEX
	self.ibStart = self.vwp.idxAddOn
	local draw = true
	local iwp = g_input.idxCount * SIZE_INDEX
	for spId, func in pairs(mtl.func) do
		local dcList = scene:GetDrawcall(spId, func.mergeType, func.order)
		if (not disables[spId] and dcList) then
			if (dcList.mtl ~= mtl) then
				func.func(dcList)
				dcList.mtl = mtl
			end
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

function MeshRenderer:RenderCached(scene, disables)
	self.vwp = g_input.vtx[self.mtl.vtxLayout].wp
	disables = disables or {}
	MeshRenderer.o = self
	if (self.update) then
		self.updateCached()
		self.update = false
	end
	local mtl = self.mtl
	self.vbDst = g_input.vtx[mtl.vtxLayout].vb
	local draw = false
	local iwp = g_input.idxCount * SIZE_INDEX
	for spId, func in pairs(mtl.func) do
		local dcList = scene:GetDrawcall(spId, func.mergeType, func.order)
		if (not disables[spId] and dcList) then
			if (dcList.mtl ~= mtl) then
				func.func(dcList)
				dcList.mtl = mtl
			end
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

function MeshRenderer:SetMaterial(mtl, ...)
	if (self.mtl == mtl) then
		return
	end
	self.mtl = mtl
	self.stride = mtl.vtxLayout
	self.insArgs = {...}
	for _, v in pairs(mtl.insSlot) do
		self.insArgs[v] = self.insArgs[v] or {0, 1}
	end
	
	local layout = self.layout
	local f
	if (self.doCache) then
		f = MeshRenderer.VtxHandlerCached[layout][mtl.vtxLayout]
	else
		f = MeshRenderer.VtxHandler[layout][mtl.vtxLayout]
	end
	if (f == nil) then
		local c = [[local copy = o.copy local vb = o.vb local n_vtx = o.n_vtx 
			local vbDst = o.vbDst local vwp = o.vwp local stride = o.stride ]]
		local c1 = ''
		local i = 1
		if (self.vb) then
			while (i <= layout and i ~= 0) do
				if (i & layout ~= 0) then
					local slot = mtl.slot[i]
					if (slot) then
						c = c .. 'local n_vtx'..i..' = n_vtx * stride['..slot..']'
						c = c .. 'copy(vb['..i..'], 0, n_vtx'..i..', vbDst['..slot..'], vwp['..slot..'])'
						c1 = c1 .. 'vwp['..slot..'] = vwp['..slot..'] + n_vtx'..i..' '
					end
				end
				i = i << 1
			end
			f = load(c..c1, '', 't', MeshRenderer)
			MeshRenderer.VtxHandlerCached[layout][mtl.vtxLayout] = f
		else
			c = [[local vbDst = o.vbDst local vwp = o.vwp local stride = o.stride
				o.n_vtx, o.n_idx = o.reader(o.mesh, ]]
			c1 = 'local n_vtx = o.n_vtx '
			while (i <= self.layout and i ~= 0) do
				if (i & layout ~= 0) then
					local slot = mtl.slot[i]
					if (slot) then
						c1 = c1 .. 'vwp['..slot..'] = vwp['..slot..'] + n_vtx * stride['..slot..'] '
						c = c .. 'vbDst[' .. slot ..'], vwp[' .. slot ..'], '
					else
						c = c .. 'g_vbNull, 0, '
					end
				end
				i = i << 1
			end
			c = c .. 'o.ibDst, o.iwp, o.ibStart) '
			f = load(c..c1, '', 't', MeshRenderer)
			MeshRenderer.VtxHandler[layout][mtl.vtxLayout] = f
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
	if (not list.reset) then
		list:Reset(g_input)
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
		DrawcallList.vp = surface.rect
		DrawcallList.cr = surface.rect
		
		for _, dcList in pairs(dcLists) do
			dcList:Reset(input)
		end
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
			
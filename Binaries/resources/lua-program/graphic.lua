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

---VtxInput---
g_vbNull = CMBuffer(16)
local g_vtxInput = {}
function NewVtxInput(...)
	local o = {...}
	local n
	local input
	local same = 0
	for _, v in pairs(g_vtxInput) do
		local s = v.stride
		n = #s
		input = v
		if (#o == n) then
			for i = 1, n do
				if (o[i] == s[i]) then
					same = i
				else
					break
				end
			end
		end
	end
	if (same == n) then
		return input 
	end
	input = {stride = o}
	table.insert(g_vtxInput, input)
	return input
end

---RenderCommand---
local g_renderCmdPool = {}
local g_nRenderCmdPool = 0

function Command.New()
	local cmd, o = next(g_renderCmdPool)
	if (o) then
		for k, v in pairs(o) do
			g_vtxInput[k][cmd] = v
		end
		g_renderCmdPool[cmd] = nil
		g_nRenderCmdPool = g_nRenderCmdPool - 1
		return cmd
	end
	cmd = cGI:NewCommand(false)
	cmd.rb = {}
	cmd.rb.mb = CMBuffer(128)
	cmd.rb.gb = cGI:NewBuffer(128)
	cmd.rb.pos = 0
	cmd.rIdx = 0
	for _, v in pairs(g_vtxInput) do
		local o = {wp = {idxAddOn = 0},
		[0] = {vb = {}, vbSet = cGI:NewBufferSet(nil, 0)}, 
		[1] = {vb = {}, vbSet = cGI:NewBufferSet(nil, 0)}}
		for k, s in pairs(v.stride) do
			o.wp[k] = 0
		
			local vb = cGI:NewBuffer(s)
			table.insert(o[0].vb, vb)
			o[0].vbSet:Add(vb)
			
			vb = cGI:NewBuffer(s)
			table.insert(o[1].vb, vb)
			o[1].vbSet:Add(vb)
		end
		v[cmd] = o
	end
	return cmd
end

function Command:Prepare()
	for _, v in pairs(g_vtxInput) do
		local o = v[self]
		for k, _ in pairs(o.wp) do
			o.wp[k] = 0
		end
	end
end

function Command:WaitFinish()
	self:Wait()
	if (self.rb.updated) then
		CBufferCopy(self.rb.mb, 0, self.rb.pos, self.rb.gb, 0)
		self.updated = false
	end
end

function Command:Flip()
	self.rIdx = ~self.rIdx & 1 
end

function Command.Recycle(cmd)
	local rec = false
	local o
	if (g_renderCmdPool < 10) then
		rec = true
		o = {}
		g_renderCmdPool[cmd] = o
		g_nRenderCmdPool = g_nRenderCmdPool + 1
	end
	for k, v in pairs(g_vtxInput) do
		if (rec) then
			o[k] = v[cmd]
		end
		v[cmd] = nil
	end
end

---ResBuffer---
ResBuffer = class()

function ResBuffer:ctor(cmd, ...)
	self.func = {}
	self.n = 0
	for k, v in pairs({...}) do
		self.func[k] = v
		self.n = self.n + g_sizeFunc[v]
	end
	self.buf = cmd.rb
	self.pos = cmd.rb.pos
	cmd.rb.pos = cmd.rb.pos + self.n
	cmd.rb.gb:Resize(cmd.rb.pos)
end

function ResBuffer:Set(idx, ...)
	self.func[idx](self.buf.mb, self.pos, 1, ...)
	self.buf.updated = true
end

---ResourceSet---
function ResourceSet:ResBuffer(b, binding)
	self:BindBuffer(b.buf.gb, b.pos, b.n, binding, cGI.RESOURCE_TYPE_UNIFORM_BUFFER)
end

function ResourceSet:BindTexelView(v, binding)
	self:BindBuffer(v, 0, cGI.WHOLE_SIZE, 0, cGI.RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER)
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
	self.vbArgs = {}
	self.indBuf2 = {}
	self.indBuf2[0] = cGI:NewDrawIndirectCmd(1)
	self.indBuf2[1] = cGI:NewDrawIndirectCmd(1)
	self.ib2 = {}
	self.ib2[0] = cGI:NewBuffer(SIZE_INDEX)
	self.ib2[1] = cGI:NewBuffer(SIZE_INDEX)
	self.dcCount = 0
	self.idxCount = 0
end

function DrawcallList:Reset()
	self.cmd = g_cmd
	self.dcIdx = 0
	self.dcCount = 0
	self.dc = nil
	self.dc = self:NewDrawcall()
	self.indBuf = self.indBuf2[g_cmd.rIdx]
	self.ib = self.ib2[g_cmd.rIdx]
	self.iwp = 0
	self.indBuf:Reset()
	self.ib:SetWritePos(0)
	self.idxCount = 0
	self.resIdx = 0
	self.p.vp = nil
	self.p.cr = nil
	self.p.pl = nil
	self.p.mainVbSet = nil
	self.p.mainSlot = nil
	self.spId = nil
	for _, v in pairs(self.vbArgs) do
		v.idxStart = 0
		v.idxAddOn = 0
	end
	for k, _ in pairs(self.insVbSets) do
		self.insVbSets[k] = nil
	end
end

function DrawcallList:NewDrawcall()
	self.dcIdx = self.dcIdx + 1
	local dc
	if (self.dcIdx <= self.dcCap) then
		dc = self.dcList[self.dcIdx]
		dc.vp = nil
		dc.cr = nil
		dc.mainSlot = nil
		for k, _ in pairs(dc.vbSets) do
			dc.vbSets[k] = nil
		end
	else
		dc = {resList = {}, vbSets = {}, vtxOffsets = {}}
		self.dcCap = self.dcCap + 1
		self.dcList[self.dcCap] = dc
	end
	dc.nOffsets = 0
	dc.resCount = 0
	dc.indCount = 0
	if (self.dc) then
		dc.indStart = self.dc.indStart + self.dc.indCount
	else
		dc.indStart = 0
	end
	return dc
end

function DrawcallList:SetupDrawcalls()
	self:CommitCurrent()
	local cmd = self.cmd
	for i = 1, self.dcCount do
		local dc = self.dcList[i]
		if (dc.vp) then
			cmd:SetViewport(dc.vp.x, dc.vp.y, dc.vp.w, dc.vp.h, 0, 1)
		end
		if (dc.cr) then
			cmd:SetClipRect(dc.cr.x, dc.cr.y, dc.cr.w, dc.cr.h)
		end
		for k, v in pairs(dc.resList) do
			cmd:SetResourceSet(v, k)
			dc.resList[k] = nil
		end
		for k, v in pairs(dc.vbSets) do
			cmd:SetVertexBuffers(v, k)
		end
		if (dc.mainSlot) then
			cmd:SetVertexBuffers(dc.mainVbSet, dc.mainSlot)
		end
		cmd:DrawIndexedIndirect(dc.pl, self.ib, self.indBuf, dc.indStart, dc.indCount)
	end
	--Print('---draw call---', self.dcCount)
end

function DrawcallList:SetPipeline(pl, vtxInput, slot)
	self.c.pl = pl
	self.c.mainVbSet = vtxInput[self.cmd][self.cmd.rIdx].vbSet
	self.c.mainSlot = slot
	self.vbArg = self.vbArgs[vtxInput] or {idxStart = 0, idxAddOn = 0}
	self.vbArgs[vtxInput] = self.vbArg
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
		--self.cmd:AddResourceSet(res)
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
	
	if (self.c.mainVbSet ~= self.p.mainVbSet or self.c.mainSlot ~= self.p.mainSlot) then
		self:CommitCurrent()
		self.p.mainVbSet = self.c.mainVbSet
		self.p.mainSlot = self.c.mainSlot
		self.dc.mainVbSet = self.c.mainVbSet
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
	self:CommitIndBuffer()
	if (self.dc and self.dc.indCount > 0) then
		self.dcCount = self.dcCount + 1
		self.dc = self:NewDrawcall()
	end
end

function DrawcallList:CommitIndBuffer()
	if (self.idxCount > 0) then
		self.indBuf:AddDrawIndexed(0, self.vbArg.idxStart, self.idxCount, self.instStart, self.instCount)
		
		self.vbArg.idxStart = self.vbArg.idxStart + self.idxCount
		self.idxCount = 0
		
		self.dc.indCount = self.dc.indCount + 1
	end
end

function DrawcallList:Draw(idxAddOn, vtxCount, idxCount, instStart, instCount)
	self:CommitStates()
	if (self.vbArg.idxAddOn ~= idxAddOn or self.instStart ~= instStart or self.instCount ~= instCount) then
		self:CommitIndBuffer()
	end
	self.vbArg.idxAddOn = idxAddOn + vtxCount
	self.instStart = instStart
	self.instCount = instCount
	self.idxCount = self.idxCount + idxCount
end

---Mesh---
Mesh = class()
Mesh.UpdateCached = {}
Mesh.VtxHandler = {}
Mesh.VtxHandlerCached = {}

local function RenderMesh(mesh, disables)
	Mesh.mesh = mesh
	local mtl = mesh.mtl
	mesh.vbDst = mtl.vtxInput[g_cmd][g_cmd.rIdx].vb
	local draw = true
	for spId, func in pairs(mtl.func) do
		if (disables[spId] ~= true and g_dcLists[spId]) then
			local dcList = g_dcLists[spId]
			if (dcList.spId ~= spId) then
				func(dcList)
				dcList.spId = spId
			end
			if (draw) then
				mesh.vtxHandler()
				draw = false
			end
			local n_idx = mesh.idxFunc(mesh.funcData, dcList.ib, mesh.vwp.idxAddOn, dcList.iwp)
			dcList.iwp = dcList.iwp + n_idx * SIZE_INDEX
			local insArgs = mesh.insArgs[mtl.insSlot[spId]]
			dcList:Draw(mesh.vwp.idxAddOn, mesh.n_vtx, n_idx, insArgs[1], insArgs[2])
		end
	end
	mesh.vwp.idxAddOn = mesh.vwp.idxAddOn + mesh.n_vtx
	mesh.n_vtx = 0
end

local function RenderMeshCached(mesh, disables)
	Mesh.mesh = mesh
	if (mesh.update) then
		mesh.updateCached()
		mesh.n_idx = mesh.idxFunc(mesh.funcData, mesh.ib, 0, 0)
		mesh.update = false
	end
	local mtl = mesh.mtl
	mesh.vbDst = mtl.vtxInput[g_cmd][g_cmd.rIdx].vb
	local draw = false
	for spId, func in pairs(mtl.func) do
		if (disables[spId] ~= true and g_dcLists[spId]) then
			local dcList = g_dcLists[spId]
			if (dcList.spId ~= spId) then
				func(dcList)
				dcList.spId = spId
			end
			CCopyIndexBuffer(mesh.ib, 0, mesh.n_idx, mesh.vwp.idxAddOn, dcList.ib, dcList.iwp)
			dcList.iwp = dcList.iwp + mesh.n_idx * SIZE_INDEX
			local insArgs = mesh.insArgs[mtl.insSlot[spId]]
			dcList:Draw(mesh.vwp.idxAddOn, mesh.n_vtx, mesh.n_idx, insArgs[1], insArgs[2])
			draw = true
		end
	end
	if (draw) then
		mesh.vtxHandler()
		mesh.vwp.idxAddOn = mesh.vwp.idxAddOn + mesh.n_vtx
	end
end

function Mesh:ctor(vtxFunc, idxFunc, funcData, doCache, layout)
	self.vtxFunc = vtxFunc
	self.idxFunc = idxFunc
	self.funcData = funcData
	self.doCache = doCache
	self.layout = layout
	self.insArgs = {}
	
	if (doCache) then
		self.copy = CBufferCopy
		Mesh.VtxHandlerCached[layout] = Mesh.VtxHandlerCached[layout] or {}
		self.vb = {}
		local c = 'local vb = mesh.vb mesh.n_vtx = mesh.vtxFunc(mesh.funcData, '
		local f = Mesh.UpdateCached[layout]
		local i = 1
		while (i <= layout and i ~= 0) do
			if (i & layout ~= 0) then
				self.vb[i] = CMBuffer(8)
				if (f == nil) then
					c = c .. 'vb[' .. i ..'], 0'
					if (i << 1 < layout and i << 1 ~= 0) then
						c = c .. ', '
					else
						c = c .. ')'
					end
				end
			end
			i = i << 1
		end
		self.ib = CMBuffer(SIZE_INDEX)
		self.render = RenderMeshCached
		if (f == nil) then
			f = load(c, '', 't', Mesh)
			Mesh.UpdateCached[layout] = f
		end
		self.updateCached = f
	else
		Mesh.VtxHandler[layout] = Mesh.VtxHandler[layout] or {}
		self.render = RenderMesh
	end
	self.update = true
	self.param = {}
end

function Mesh:SetMaterial(mtl, ...)
	if (self.mtl == mtl) then
		return
	end
	self.mtl = mtl
	self.stride = mtl.vtxInput.stride
	self.insArgs = {...}
	
	local layout = self.layout
	local f
	if (self.doCache) then
		f = Mesh.VtxHandlerCached[layout][mtl.vtxInput]
	else
		f = Mesh.VtxHandler[layout][mtl.vtxInput]
	end
	if (f == nil) then
		local c = [[local copy = mesh.copy local vb = mesh.vb local n_vtx = mesh.n_vtx 
		local vbDst = mesh.vbDst local vwp = mesh.vwp local stride = mesh.stride ]]
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
			f = load(c..c1, '', 't', Mesh)
			Mesh.VtxHandlerCached[layout][mtl.vtxInput] = f
		else
			c = 'local vbDst = mesh.vbDst local vwp = mesh.vwp local stride = mesh.stride mesh.n_vtx = mesh.vtxFunc(mesh.funcData, '
			c1 = 'local n_vtx = mesh.n_vtx '
			while (i <= self.layout and i ~= 0) do
				if (i & layout ~= 0) then
					local slot = mtl.slot[i]
					if (slot) then
						c1 = c1 .. 'vwp['..slot..'] = vwp['..slot..'] + n_vtx * stride['..slot..'] '
						c = c .. 'vbDst[' .. slot ..'], vwp[' .. slot ..']'
					else
						c = c .. 'g_vbNull, 0'
					end
					if (i << 1 < layout and i << 1 ~= 0) then
						c = c .. ', '
					else
						c = c .. ')'
					end
				end
				i = i << 1
			end
			f = load(c..c1, '', 't', Mesh)
			Mesh.VtxHandler[layout][mtl.vtxInput] = f
		end
	end
	self.vtxHandler = f
end

function Mesh:Render(disables)
	self.vwp = self.mtl.vtxInput[g_cmd].wp
	self:render(disables or {})
end

---FramePipeline---
FramePipeline = class()

function FramePipeline:ctor()
	self.cmdList = {}
end

function FramePipeline:AddFrameOutput(fb, ...)
	table.insert(self.cmdList, {type = 0, fb = fb, dcLists = {}, params = {...}})
end

function FramePipeline:AddCopyImage(params)
	table.insert(self.cmdList, {type = 1, params = params})
end

function FramePipeline:UpdateLayouts()
	for param, dcLists in pairs(self.foParams) do
		local layout = param.layout
		DrawcallList.vp = layout.rect
		DrawcallList.cr = layout.rect
		
		for _, dcList in pairs(dcLists) do
			dcList:Reset()
		end
		g_dcLists = dcLists
		layout:Update(nil, layout.rect)
	end
end

function FramePipeline:FillCommand(cmd)
	self.cmd = cmd
	self.code()
end

function FramePipeline:Bake()
	self.foParams = {}

	local code = 'local c local o\n'
	for k, c in pairs(self.cmdList) do
		code = code .. 'c = cmdList[' .. k .. ']\n'
		if (c.type == 0) then
			code = code .. 'cmd:RenderBegin(c.fb, false)\n'
			
			for k, param in pairs(c.params) do				
				code = code .. 'c.dcLists['..k..']:SetupDrawcalls()\n'
				if (k ~= #c.params) then
					code = code..'cmd:NextSubpass(false)\n'
				end
				
				c.dcLists[k] = c.dcLists[k] or DrawcallList()
				local dcLists = self.foParams[param] or {}
				dcLists[param.spId] = c.dcLists[k]
				self.foParams[param] = dcLists
			end
			code = code..'cmd:RenderEnd()\n'
		elseif (c.type == 1) then
			code = code..[[o = c.params cmd:CopyImage(o.srcView, o.srcLayer, o.src_x, o.src_y, 
			o.dstView, o.dstLayer, o.dst_x, o.dst_y, o.numLayers, o.w, o.h)]]
		end
	end
	self.code = load(code, '', 't', self)
end
			
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
	input = {stride = o, wp = {}, vb = {}, vbSet = {}}
	for i = 1, #o do
		table.insert(input.wp, 0)
	end
	table.insert(g_vtxInput, input)
	return input
end

---RenderCommand---
local g_renderCmdPool = {}
local g_nRenderCmdPool = 0

function Command.New()
	local i, cmd = next(g_renderCmdPool)
	if (cmd) then
		for k, v in pairs(cmd.vtxInputs) do
			g_vtxInput[k].vb[cmd] = v.vb
			g_vtxInput[k].vbSet[cmd] = v.vbSet
		end
		g_renderCmdPool[i] = nil
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
		local vbt = {[0] = {}, [1] = {}}
		v.vb[cmd] = vbt
		local vbSet = {[0] = cGI:NewBufferSet(nil, 0), [1] = cGI:NewBufferSet(nil, 0)}
		v.vbSet[cmd] = vbSet
		local vb
		for _, s in pairs(v.stride) do
			vb = cGI:NewBuffer(s)
			table.insert(vbt[0], vb)
			vbSet[0]:Add(vb)
	
			vb = cGI:NewBuffer(s)
			table.insert(vbt[1], vb)
			vbSet[1]:Add(vb)
		end
	end
	return cmd
end

function Command:Flip()
	self.rIdx = ~self.rIdx & 1
	for _, v in pairs(g_vtxInput) do
		v.vbSet[self][self.rIdx]:SetWritePos(0)
		for k, _ in pairs(v.wp) do
			v.wp[k] = 0
		end
	end
end

function Command:PrepareRender()
	self:Wait()
	if (self.rb.updated) then
		CBufferCopy(self.rb.mb, 0, self.rb.pos, self.rb.gb, 0)
		self.updated = false
	end
end

function Command.Recycle(cmd)
	local rec = false
	local o
	if (g_renderCmdPool < 10) then
		o = {vtxInputs = {}}
		table.insert(g_renderCmdPool, o)
		g_nRenderCmdPool = g_nRenderCmdPool + 1
		rec = true
		o.cmd = cmd
	end
	for k, v in pairs(g_vtxInput) do
		if (rec) then
			o.vtxInputs[k].vb = v.vb[cmd]
			o.vtxInputs[k].vbSet = v.vbSet[cmd]
		end
		v.vb[cmd] = nil
		v.vbSet[cmd] = nil
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
	self.p.mainSlot = nil
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
	for i = 1, self.dcCount do
		local dc = self.dcList[i]
		if (dc.vp) then
			g_cmd:SetViewport(dc.vp.x, dc.vp.y, dc.vp.w, dc.vp.h, 0, 1)
		end
		if (dc.cr) then
			g_cmd:SetClipRect(dc.cr.x, dc.cr.y, dc.cr.w, dc.cr.h)
		end
		for k, v in pairs(dc.resList) do
			g_cmd:SetResourceSet(v, k)
			dc.resList[k] = nil
		end
		for k, v in pairs(dc.vbSets) do
			g_cmd:SetVertexBuffers(v, k)
		end
		if (dc.mainSlot) then
			g_cmd:SetVertexBuffers(dc.mainVbSet, dc.mainSlot)
		end
		g_cmd:DrawIndexedIndirect(dc.pl, self.ib, self.indBuf, dc.indStart, dc.indCount)
	end
	Print('---draw call---', self.dcCount)
end

function DrawcallList:SetPipeline(pl, vtxInput, slot)
	self.c.pl = pl
	self.c.mainVbSet = vtxInput.vbSet[g_cmd][g_cmd.rIdx]
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
		--self.cmd:AddResourceSet(res)
		self.res[self.resIdx] = res
		self.dc.resList[self.resIdx] = res
	end
	self.resIdx = self.resIdx + 1
end

function DrawcallList:CommitStates()
	if (self.c.pl ~= self.p.pl or self.c.mainVbSet ~= self.p.mainVbSet) then
		self:CommitCurrent()
		self.p.pl = self.c.pl
		self.p.mainVbSet = self.c.mainVbSet
		self.vbArg = self.vbArgs[self.c.mainVbSet] or {idxStart = 0, idxAddOn = 0}
	end
	self.dc.pl = self.c.pl
	self.dc.mainVbSet = self.c.mainVbSet
	
	if (self.c.mainSlot ~= self.p.mainSlot) then
		self:CommitCurrent()
		self.p.mainSlot = self.c.mainSlot
		self.dc.mainSlot = self.c.mainSlot
	end
	
	if (DrawcallList.vp ~= self.p.vp) then
		self:CommitCurrent()
		self.dc.vp = DrawcallList.vp
		self.p.vp = DrawcallList.vp
		DrawcallList.vp = DrawcallList.dvp
	end
	
	if (DrawcallList.cr ~= self.p.cr) then
		self:CommitCurrent()
		self.dc.cr = DrawcallList.cr
		self.p.cr = DrawcallList.cr
		DrawcallList.cr = DrawcallList.dcr
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

function DrawcallList:Draw(vtxCount, idxCount, instStart, instCount)
	self.vbArg.idxAddOn = self.vbArg.idxAddOn + vtxCount
	if (self.instStart ~= instStart or self.instCount ~= instCount) then
		self:CommitIndBuffer()
	end
	self.instStart = instStart
	self.instCount = instCount
	self.idxCount = self.idxCount + idxCount
end

function DrawcallList:Skip(vtxInput, vtxCount)
	self:CommitIndBuffer()
	local n = vtxInput.vbSet[g_cmd][g_cmd.rIdx]
	local vbArg = self.vbArgs[n] or {idxStart = 0, idxAddOn = 0}
	vbArg.idxAddOn = vbArg.idxAddOn + vtxCount
	self.vbArgs[n] = vbArg
end

---Mesh---
Mesh = class()
Mesh.UpdateCached = {}
Mesh.VtxHandler = {}
Mesh.VtxHandlerCached = {}

local function RenderMesh(mesh, disables)
	Mesh.mesh = mesh
	local mtl = mesh.mtl
	mesh.vbDst = mtl.vtxInput.vb[g_cmd][g_cmd.rIdx]
	mesh.vtxHandler()
	for spId, dcList in pairs(g_dcLists) do
		skip = true
		if (disables[spId] ~= true and mtl[spId]) then
			mtl[spId](dcList)
			local n_idx = mesh.idxFunc(mesh.funcData, dcList.ib, dcList.vbArg.idxAddOn, dcList.iwp)
			dcList.iwp = dcList.iwp + n_idx * SIZE_INDEX
			local insArgs = mesh.insArgs[mtl.insSlot[spId]]
			dcList:Draw(mesh.n_vtx, n_idx, insArgs[1], insArgs[2])
			skip = false
		end
		if (skip) then
			dcList:Skip(mtl.vtxInput, mesh.n_vtx)
		end
	end
end

local function RenderMeshCached(mesh, disables)
	Mesh.mesh = mesh
	if (mesh.update) then
		mesh.updateCached()
		mesh.n_idx = mesh.idxFunc(mesh.funcData, mesh.ib, 0, 0)
		mesh.update = false
	end
	local mtl = mesh.mtl
	mesh.vbDst = mtl.vtxInput.vb[g_cmd][g_cmd.rIdx]
	mesh.vtxHandler()
	for spId, dcList in pairs(g_dcLists) do
		skip = true
		if (disables[spId] ~= true and mtl[spId]) then
			mtl[spId](dcList)
			CCopyIndexBuffer(mesh.ib, 0, mesh.n_idx, dcList.vbArg.idxAddOn, dcList.ib, dcList.iwp)
			dcList.iwp = dcList.iwp + mesh.n_idx * SIZE_INDEX
			local insArgs = mesh.insArgs[mtl.insSlot[spId]]
			dcList:Draw(mesh.n_vtx, mesh.n_idx, insArgs[1], insArgs[2])
			skip = false
		end
		if (skip) then
			dcList:Skip(mtl.vtxInput, mesh.n_vtx)
		end
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
	self.vwp = mtl.vtxInput.wp
	self.insArgs = {...}
	
	local layout = self.layout
	local f
	if (self.doCache) then
		f = Mesh.VtxHandlerCached[layout][mtl.vtxInput]
	else
		f = Mesh.VtxHandler[layout][mtl.vtxInput]
	end
	if (f == nil) then
		local c = 'local copy = mesh.copy local vb = mesh.vb local n_vtx = mesh.n_vtx local vbDst = mesh.vbDst local vwp = mesh.vwp local stride = mesh.stride '
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
	self:render(disables or {})
end

---FramePipeline---
FramePipeline = class()

function FramePipeline:ctor()
	self.cmdList = {}
	self.rIdx = 0
	self.wIdx = 0
end

function FramePipeline:AddFrameOutput(fb, ...)
	table.insert(p.cmdList, {type = 0, fb = fb, inputs = {...}})
end

function FramePipeline:AddCopyImage(params)
	table.insert(p.cmdList, {type = 1, params = params})
end

function FramePipeline:UpdateLayouts()
	local layout
	local draw
	for input, param in pairs(self.fpParams) do
		layout = input.layout
		g_pass = data.pass
		for k, v in g_pass do
			g_pass[k] = layout[k]
			draw = true
			--g_drawMgr[k]:Reset(
		end
		if (draw) then
			g_drawMgr = data.drawMgr
			layout:Update()
		end
		draw = false
	end
end

function FramePipeline:FillCommand(cmd)
	self.cmd = cmd
	self.code()
end

function FramePipeline:Bake()
	self.fpParams = {}

	local code = ''
	for i = 1, #self.cmdList do
		local o = cmdList[i]
		
		code = code .. 'local o = cmdList[' .. i .. '] '
		if (o.type == 0) then
			code = code .. 'cmd:RenderBegin(o.fb, false) '
			for j = 1, #o.inputs do
				local fpParam = self.fpParams[o.inputs[j]] or {}
				fpParam.dcList = fpParam.dcList or {}
				fpParam.dcList[o.fb[j]] = DrawcallList()
				fpParam.pass = fpParam.pass or {}
				fpParam.pass[o.fb[j]] = o.fb[j]
				self.fpParams[o.inputs[j]] = fpParam

				code = code..'local layout = o.inputs['..j..'].layout local vsInput = layout.vsInput[rIdx] '
				code = code..'vsInput.vbSet:SetDrawOffset(0) '
				code = code..'layout.dcList[o.fb]['..j..']:SetupDrawcalls(vsInput, cmd) '
				if (j ~= #o.refLayouts) then
					code = code..'cmd:NextSubpass(false) '
				else
					code = code..'cmd:RenderEnd() '
				end
			end
		elseif (type == 1) then
			code = code..[[o = 
			o.params cmd:CopyImage(o.srcView, o.srcLayer, o.src_x, o.src_y, o.dstView, o.dstLayer, o.dst_x, o.dst_y, o.numLayers, w, h) ]]
		end
	end
	self.code = load(code, '', self)
end
			
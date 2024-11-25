---graphic.lua---
require 'defines'
require 'class'

function NewVSInput()
	return {vbSet = cGI:NewBufferSet(nil, 0), nElems = 0}
end

function NewVSInput2()
	return {[0] = NewVSInput(), [1] = NewVSInput()}
end	

function ResizeVbSet(vsInput, nElems)
	if (vsInput.nElems < nElems) then
		local b
		for i = 1, nElems - vsInput.nElems do
			b = cGI:NewBuffer(1)
			vsInput.vbSet:Add(b)
			vsInput[vsInput.nElems] = b
			vsInput.nElems = vsInput.nElems + 1
		end
	end
end

---ResBuffer---
ResBuffer = class()

function ResBuffer:ctor(n)
	self.mb = CMBuffer(n)
	self[0] = cGI:NewBuffer(n)
	self.size = n
end

function ResBuffer:Set(offset, func, ...)
	func(self.mb, offset, 1, ...)
	g_dcList.rbList[self] = self
end

function ResBuffer:Update()
	CBufferCopy(self.mb, 0, self.size, self[0], 0)
end	

---ResourceSet---
ResSet = class()

function ResSet:BindUniformBuffer(b, binding)
	self.buffers[binding] = b
	self[0]:BindBuffer(b[0], binding, cGI.RESOURCE_UNIFORM_BUFFER)
end

function ResSet:BindTexelView(v, binding)
	self[0]:BindBuffer(v, binding, cGI.RESOURCE_UNIFORM_TEXEL_BUFFER)
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
	self.rbList = {}
	self.indBuf2 = {}
	self.indBuf2[0] = cGI:NewDrawIndirectCmd(1)
	self.indBuf2[1] = cGI:NewDrawIndirectCmd(1)
	self.ib2 = {}
	self.ib2[0] = cGI:NewBuffer(SIZE_INDEX)
	self.ib2[1] = cGI:NewBuffer(SIZE_INDEX)
	self.vtxOffsets = {}
	self:Reset(nil, nil, 0)
end

function DrawcallList:Reset(vp, cr, rIdx)
	self.rIdx = rIdx
	self.dcIdx = 0
	self.dcCount = 0
	self.dc = nil
	self.dc = self:NewDrawcall()
	self.IdxStart = 0
	self.indBuf = self.indBuf2[rIdx]
	self.ib = self.ib2[rIdx]
	self.indBuf:Reset()
	self.ib:SetWritePos(0)
	self.idxCount = 0
	self.resIdx = 0
	self.d.vp = vp
	self.d.cr = cr
	self.c.vp = vp
	self.c.cr = cr
	self.p.vp = nil
	self.p.cr = nil
	self.p.pl = nil
	self.p.pl = nil
	for k, _ in pairs(self.vtxOffsets) do
		self.vtxOffsets[k] = 0
	end
end

function DrawcallList:NewDrawcall()
	self.dcIdx = self.dcIdx + 1
	local dc
	if (self.dcIdx <= self.dcCap) then
		dc = self.dcList[self.dcIdx]
		dc.vp = nil
		dc.cr = nil
		dc.pl = nil
	else
		dc = {resList = {}, resIdx = {}, vtxOffsets = {}}
		self.dcCap = self.dcCap + 1
		self.dcList[self.dcCap] = dc
	end
	
	dc.nOffsets = 0
	if (self.dcIdx > 1) then
		local stride = self.p.pl.stride
		local n
		for i = 0, self.p.pl.nElems - 1 do
			n = self.vtxOffsets[i] or 0
			n = n + stride[i] * self.idxAddOn
			self.vtxOffsets[i] = n
			dc.vtxOffsets[i] = n
		end
		dc.nOffsets = self.p.pl.nElems
	end
	self.idxAddOn = 0
	
	if (self.dc) then
		dc.instStart = self.dc.instStart + self.dc.instCount	
	else
		dc.instStart = 0
	end
	dc.resCount = 0
	dc.instCount = 0
	return dc
end

function DrawcallList:SetupDrawcalls(cmd)
	self:CommitCurrent()
	for k, rb in pairs(self.rbList) do
		rb:Update()
		self.rbList[k] = nil
	end
	for i = 1, self.dcCount do
		local dc = self.dcList[i]
		if (dc.vp) then
			cmd:SetViewport(dc.vp.x, dc.vp.y, dc.vp.w, dc.vp.h, 0, 1)
		end
		if (dc.cr) then
			cmd:SetClipRect(dc.cr.x, dc.cr.y, dc.cr.w, dc.cr.h)
		end
		for i = 1, dc.resCount do
			cmd:SetResourceSet(dc.resList[i], dc.resIdx[i])
		end
		cmd:DrawIndexedIndirect(dc.pl.pl, g_vsInput.vbSet, self.ib, self.indBuf, dc.instStart, dc.instCount, dc.nOffsets, dc.vtxOffsets)
	end
	Print('---draw call---', self.dcCount)
end

function DrawcallList:SetViewport(vp)
	self.c.vp = vp
end

function DrawcallList:SetClipRect(cr)
	self.c.cr = cr
end

function DrawcallList:SetPipeline(pl)
	self.c.pl = pl
end

function DrawcallList:AddResourceSet(res)
	if (self.res[self.resIdx] ~= res) then
		self:CommitCurrent()
		--self.cmd:AddResourceSet(res)
		self.dc.resCount = self.dc.resCount + 1
		self.dc.resList[self.dc.resCount] = res
		self.dc.resIdx[self.dc.resCount] = self.resIdx

		self.res[self.resIdx] = res
	end
	self.resIdx = self.resIdx + 1
end

function DrawcallList:CommitStates()
	if (self.c.pl ~= self.p.pl) then
		self:CommitCurrent()
		self.p.pl = self.c.pl
	end

	if (self.c.vp ~= self.p.vp) then
		self:CommitCurrent()
		self.dc.vp = self.c.vp
		self.p.vp = self.c.vp
		self.c.vp = self.d.vp
	end
	
	if (self.c.cr ~= self.p.cr) then
		self:CommitCurrent()
		self.dc.cr = self.c.cr
		self.p.cr = self.c.cr
		self.c.cr = self.d.cr
	end
	self.dc.pl = self.c.pl
	
	ResizeVbSet(g_vsInput, self.dc.pl.nElems)
	
	self.resIdx = 0
end

function DrawcallList:CommitCurrent()
	self:CommitIndBuffer()
	if (self.dc.instCount > 0) then
		self.dcCount = self.dcCount + 1
		self.dc = self:NewDrawcall()
	end
end

function DrawcallList:CommitIndBuffer()
	if (self.idxCount > 0) then
		self.indBuf:AddDrawIndexed(0, self.IdxStart, self.idxCount, self.instStart, self.instCount)
		
		self.IdxStart = self.IdxStart + self.idxCount
		self.idxCount = 0
		
		self.dc.instCount = self.dc.instCount + 1
	end
end

function DrawcallList:Draw(vtxCount, idxCount, instStart, instCount)
	self.idxAddOn = self.idxAddOn + vtxCount
	
	if (self.instStart ~= instStart or self.instCount ~= instCount) then
		self:CommitIndBuffer()
	end
	self.instStart = instStart
	self.instCount = instCount
	self.idxCount = self.idxCount + idxCount
end

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

RenderUnit = class()

function RenderUnit:Render()
	CopyVtx(self.vb, self.vtxCount)
	for _, pass in g_pass do
		if (self.res[pass]) then
			g_dcList[pass]:draw(self.vtxCount)
		else
			g_dcList[pass]:Skip(self.vtxCount)
		end
	end
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
			code = code..'o = o.params cmd:CopyImage(o.srcView, o.srcLayer, o.src_x, o.src_y, o.dstView, o.dstLayer, o.dst_x, o.dst_y, o.numLayers, w, h) '  
		end
	end
	self.code = load(code, '', self)
end
			
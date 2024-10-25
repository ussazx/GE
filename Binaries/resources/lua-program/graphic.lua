---graphic.lua---
require 'defines'
require 'class'

function NewVSInput2D(n)
	n = n or 1024
	local o = {}
	o[VB_ELEM_FLOAT2_0] = cGI:NewBuffer(n * SIZE_FLOAT2)
	o[VB_ELEM_FLOAT3_0] = cGI:NewBuffer(n * SIZE_FLOAT3)
	o[VB_ELEM_FLOAT4_0] = cGI:NewBuffer(n * SIZE_UINT1)
	o[VB_ELEM_ID] = g_idVb
	o.vbSet = cGI:NewBufferSet(nil, 0)
	o.vbSet:Add(o[VB_ELEM_FLOAT2_0])
	o.vbSet:Add(o[VB_ELEM_FLOAT3_0])
	o.vbSet:Add(o[VB_ELEM_FLOAT4_0])
	o.vbSet:Add(o[VB_ELEM_ID])
	o.ib = cGI:NewBuffer(n * SIZE_UINT1)
	ResetVSInput2D(o)
	return o
end

function ResetVSInput2D(o)
	o.vbSet:SetWritePos(0)
	o.vbSet:SetDrawOffset(0)
	o.ib:SetWritePos(0)
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
	g_drawCmdMgr.rbList[self] = self
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

---DrawCmdMgr---
DrawCmdMgr = class()

function DrawCmdMgr:ctor()
	self.d = {}
	self.c = {}
	self.p = {}
	self.res = {}
	self.dcList = {}
	self.dcCap = 0
	self.idDcList = {}
	self.idDcCap = 0
	self.rbList = {}
	self:Reset()
end

function DrawCmdMgr:Reset(pl, vp, cr, idIndBuf)
	self.dcIdx = 0
	self.dcCount = 0
	self.dc = self:NewDrawcall()
	self.idxCount = 0
	self.vtxOffset = 0
	self.vtxOffsetAcc = 0
	self.idxPos = 0
	self.resIdx = 0
	self.idDcIdx = 0
	self.idDcCount = 0
	self.idIndBuf = idIndBuf
	self.indInstCount = 0
	self.indIdxPos = 0
	self.indIdxCount = 0
	self.indVtxOffset = 0
	self.idDc = self:NewIdDrawcall()
	self.d.pl = pl
	self.d.vp = vp
	self.d.cr = cr
	self.c.vp = vp
	self.c.cr = cr
	self.p.cr = nil
	self.p.vp = nil
	self.p.pl = nil
	self.c_id = nil
	--self.drawCall = 0
end

function DrawCmdMgr:NewDrawcall()
	self.dcIdx = self.dcIdx + 1
	local dc
	if (self.dcIdx <= self.dcCap) then
		dc = self.dcList[self.dcIdx]
		dc.vp = nil
		dc.cr = nil
		dc.pl = nil
	else
		dc = {resList = {}, resIdx = {}}
		self.dcCap = self.dcCap + 1
		self.dcList[self.dcCap] = dc
	end
	dc.resCount = 0
	dc.pl = self.d.pl
	return dc
end

function DrawCmdMgr:NewIdDrawcall()
	self.idDcIdx = self.idDcIdx + 1
	local idDc
	if (self.idDcIdx <= self.idDcCap) then
		idDc = self.idDcList[self.idDcIdx]
	else
		idDc = {}
		self.idDcCap = self.idDcCap + 1
		self.idDcList[self.idDcCap] = idDc
	end
	idDc.vp = nil
	idDc.cr = nil
	idDc.instStart = self.indInstCount
	idDc.instCount = 0
	return idDc
end

function DrawCmdMgr:CommitCurrent(isLast)
	if (self.idxCount > 0) then
		local dc = self.dc
		dc.idxPos = self.idxPos
		dc.idxCount = self.idxCount
		dc.vtxOffset = self.vtxOffset
		self.dcCount = self.dcCount + 1
		
		self.dc = self:NewDrawcall()
		self.idxPos = self.idxPos + self.idxCount
		if (isLast ~= true) then
			self.vtxOffsetAcc = self.vtxOffsetAcc + self.vtxOffset
		end
		self.vtxOffset = 0
		self.idxCount = 0
		self.resIdx = 0
	end
end

function DrawCmdMgr:SetupDrawcalls(vsInput, cmd)
	self:CommitCurrent(true)
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
		cmd:DrawIndexed(self.dc.pl, vsInput.vbSet, vsInput.ib, 0, dc.idxPos, dc.idxCount, dc.vtxOffset)
	end
	Print('---draw call---', self.dcCount)
end

function DrawCmdMgr:CommitIdCurrent()
	self:AddIdIndBuffer()
	if (self.idDc.instCount > 0) then
		self.idDcCount = self.idDcCount + 1
		self.idDc = self:NewIdDrawcall()
	end
end

function DrawCmdMgr:SetupIdDrawcalls(plId, vsInput, cmd)
	self:CommitIdCurrent()
	for i = 1, self.idDcCount do
		local idDc = self.idDcList[i]
		if (idDc.vp) then
			cmd:SetViewport(idDc.vp.x, idDc.vp.y, idDc.vp.w, idDc.vp.h, 0, 1)
		end
		if (idDc.cr) then
			cmd:SetClipRect(idDc.cr.x, idDc.cr.y, idDc.cr.w, idDc.cr.h)
		end
		cmd:DrawIndexedIndirect(plId, vsInput.vbSet, vsInput.ib, self.idIndBuf, idDc.instStart, idDc.instCount)
	end
end

function DrawCmdMgr:SetViewport(vp)
	self.c.vp = vp
end

function DrawCmdMgr:SetClipRect(cr)
	self.c.cr = cr
end

function DrawCmdMgr:SetPipeline(pl)
	self.dc.pl = pl
end

function DrawCmdMgr:AddResourceSet(res)
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

function DrawCmdMgr:CommitStates()
	if (self.dc.pl ~= self.p.pl) then
		self:CommitCurrent()
		self.p.pl = self.dc.pl
	end

	if (self.c.vp ~= self.p.vp) then
		self:CommitIdCurrent()
		self:CommitCurrent()
		self.dc.vp = self.c.vp
		self.idDc.vp = self.c.vp
		self.p.vp = self.c.vp
		self.c.vp = self.d.vp
	end
	
	if (self.c.cr ~= self.p.cr) then
		self:CommitIdCurrent()
		self:CommitCurrent()
		self.dc.cr = self.c.cr
		self.idDc.cr = self.c.cr
		self.p.cr = self.c.cr
		self.c.cr = self.d.cr
	end
	
	self.resIdx = 0
end

function DrawCmdMgr:AddIdIndBuffer()
	if (self.indIdxCount > 0) then
		self.idIndBuf:AddDrawIndexed(self.vtxOffsetAcc, self.indIdxPos, self.indIdxCount, self.c_id, 1)
		self.idDc.instCount = self.idDc.instCount + 1
		self.indInstCount = self.indInstCount + 1
		self.indIdxPos = self.indIdxPos + self.indIdxCount
		self.indIdxCount = 0
	end
end

function DrawCmdMgr:Draw(vtxCount, idxCount, id)		
	self.vtxOffset = self.vtxOffset + vtxCount
	self.idxCount = self.idxCount + idxCount
	
	if (id == nil) then
		self:AddIdIndBuffer()
		self.indIdxPos = self.indIdxPos + idxCount
		self.c_id = id
		return 
	end
	
	id = id & ID_NUM_MAX
	if (id ~= self.c_id) then
		self:AddIdIndBuffer()
		self.c_id = id
	end
	self.indIdxCount = self.indIdxCount + idxCount
end
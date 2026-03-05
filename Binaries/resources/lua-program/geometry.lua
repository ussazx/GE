-----geometry.lua-----
require 'class'
require 'utility'

degToArc = math.pi / 180

function Normalize2D(vx, vy)
	local d = vx * vx + vy * vy
	if (d == 0) then
		d = 1
	elseif (d > 1) then
		d = math.sqrt(d)
	end
	return vx / d, vy / d
end

function Dot2D(x0, y0, x1, y1)
	return x0 * x1 + y0 * y1
end

function GetLineNormal2D(x0, y0, x1, y1, clockwise)
	if (x0 == x1) then
		if (y0 == y1) then
			return 0, 0
		end
		local nx = y1 - y0
		if (clockwise) then
			return -nx / math.abs(nx), 0
		end
		return nx / math.abs(nx), 0
	end
	if (y0 == y1) then
		local ny = x0 - x1
		if (clockwise) then
			return 0, -ny / math.abs(ny)
		end
		return 0, ny / math.abs(ny)
	end
	local nx = y1 - y0
	local ny = x0 - x1
	if (clockwise) then
		nx = -nx
		ny = -ny
	end
	return Normalize2D(nx, ny)
end

local function _ExtrudeLines2D(pt, k, vol, clockwise, n0x, n0y, nnx, nny)
	local p0
	k, p0 = next(pt, k)
	local x0, y0 = p0[1], p0[2]
	local n1x, n1y = nnx, nny
	local _, p1 = next(pt, k)
	if (p1) then
		n1x, n1y = GetLineNormal2D(x0, y0, p1[1], p1[2], clockwise)
	end
	local nx, ny = n0x, n0y
	if (nx == 0 and ny == 0) then
		nx, ny = n1x, n1y
	elseif (n1x ~= 0 or n1y ~= 0) then
		nx, ny = Normalize2D((nx + n1x) * 0.5, (ny + n1y) * 0.5)
	end
	p0.normal = {-nx, -ny}
	
	if (p1) then
		return {x0 + nx * vol, y0 + ny * vol, normal = {nx, ny}}, _ExtrudeLines2D(pt, k, vol, clockwise, n1x, n1y, nnx, nny)
	end
	return {x0 + nx * vol, y0 + ny * vol, normal = {nx, ny}}
end
	

-----ExtrudeLines-----
function ExtrudeLines2D(v, vol, clockwise, closed)
	local n = #v
	if (n < 2) then
		return
	end
	local o = {}
	local n0x, n0y, nnx, nny = 0, 0, 0, 0
	if (closed and n > 2) then
		n0x, n0y = GetLineNormal2D(v[n][1], v[n][2], v[1][1], v[1][2], clockwise)
		nnx, nny = n0x, n0y
	end
	for i = 1, n do
		local p0 = v[i]
		local n1x, n1y
		local normal
		if (i < n) then
			local p1 = v[i + 1]
			n1x, n1y = GetLineNormal2D(p0[1], p0[2], p1[1], p1[2], clockwise)
			if (i == 1 and not closed) then
				normal = {n1x, n1y}
			else
				normal = {Normalize2D((n0x + n1x) * 0.5, (n0y + n1y) * 0.5)}
			end
		elseif (closed) then
			normal = {Normalize2D((n0x + nnx) * 0.5, (n0y + nny) * 0.5)}
		else
			normal = {n0x, n0y}
		end
		table.insert(o, {p0[1] + normal[1] * vol, p0[2] + normal[2] * vol, normal = normal})
		n0x, n0y = n1x, n1y
	end
	return o
end

-----DrawLine-----
function DrawLine(thickness, mid, closed, ...)
	local n0 = {...}
	local n = #n0
	if (n < 2) then
		n0 = n0[1]
		n = #n0
	end
	closed = closed and n > 2
	local pn0 = n0[n]
	if (mid) then
		thickness = thickness * 0.5
	end
	local n1 = ExtrudeLines2D(n0, thickness, true, closed)
	local pn1 = n1[n]
	for i = n, 1, -1 do
		local normal = n1[i].normal
		local n0i = n0[i]
		if (mid) then
			n0i[1], n0i[2] = n0i[1] - thickness * normal[1], n0i[2] - thickness * normal[2]
		end
		n0i.normal = {-normal[1], -normal[2]}
		table.insert(n0, n1[i])
	end
	if (closed) then
		local n2 = n * 2
		pn1.combined = {n + 1, n2 + 1}
		table.insert(n0, pn1)
		pn0.combined = {n, n2 + 2}
		table.insert(n0, pn0)
	else
		pn0, pn1 = n0[n * 2], n0[1]
		for i = 1, 2 do
			local n0x, n0y = pn0.normal[1], pn0.normal[2]
			local n1x, n1y = GetLineNormal2D(pn0[1], pn0[2], pn1[1], pn1[2], false)
			pn0.normal[1], pn0.normal[2] = Normalize2D((n0x + n1x) * 0.5, (n0y + n1y) * 0.5)
			pn1.normal[1], pn1.normal[2] = Normalize2D((-n0x + n1x) * 0.5, (-n0y + n1y) * 0.5)	
			pn0, pn1 = n0[n], n0[n + 1]
		end
	end
		
	n0.has_normal = true
	return n0
end

math.d_pi = 2 * math.pi
math.h_pi = math.pi * 0.5

------MakeRect-----
function MakeRect(x, y, w, h)
	return { {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h} }
end

------MakeCircle-----
function MakeCircle(x, y, radius, segNum)
	if (radius < 2) then
		segNum = 3
	else
		local maxSegNum = math.ceil(math.pi / (math.h_pi - math.acos(0.5 / radius)))
		if (segNum) then
			segNum = math.max(3, math.min(segNum, maxSegNum))
		else
			segNum = math.max(3, math.ceil(maxSegNum) / 4)
		end
	end
	local o = {}
	local a = math.d_pi / segNum
	x, y = x + radius, y + radius
	for i = 1, segNum do
		local aa = a * i
		table.insert(o, {x + radius * math.sin(aa), y - radius * math.cos(aa)})
	end
	return o
end

-----BakePolyNormals-----
function BakePolyNormals2D(v)
	local num = #v
	if (num < 3) then
		return
	end
	local combined = {}
	local p0, p1 = v[num], v[1]
	local nx, ny = GetLineNormal2D(p0[1], p0[2], p1[1], p1[2], false)
	local n0x, n0y = nx, ny
	for i = 1, num do
		local n1x, n1y = nx, ny
		
		p0 = v[i]
		if (i < num) then
			p1 = v[i + 1]
			n1x, n1y = GetLineNormal2D(p0[1], p0[2], p1[1], p1[2], false)
		else
			n1x, n1y = nx, ny
		end
		if (combined[p0]) then
			p0.normal[1] = p0.normal[1] + n0x + n1x
			p0.normal[2] = p0.normal[2] + n0y + n1y
		elseif (p0.combined) then
			p0.normal = {n0x + n1x, n0y + n1y}
			combined[p0] = p0
		else
			p0.normal = {Normalize2D((n0x + n1x) * 0.5, (n0y + n1y) * 0.5)}
		end
		n0x, n0y = n1x, n1y
	end
	for _, v in pairs(combined) do
		v.normal[1], v.normal[2] = Normalize2D(v.normal[1] * 0.5, v.normal[2] * 0.5)
	end
	v.has_normal = true
end

-----DrawPolyOutline-----
function DrawPolyOutline(vertices, width, gap)
	width = width or 0
	gap = gap or 0
	local outer = width + gap
	local v = vertices
	
	local num = #v
	
	local t = {}
	local i = 1
	while (i) do
		local n0 = {}
		local j = i
		i = nil
		local n = 0
		local w = false
		while (j <= num) do
			local vp = v[j]
			if (n > 0 and vp.combined) then
				if (vp.combined[2] == j) then
					break
				end
				i = j + 1
				j = vp.combined[2] + 1
			else
				j = j + 1
			end
			n = n + 1
			local vn = vp.normal
			table.insert(n0, {vp[1] + vn[1] * outer, vp[2] + vn[2] * outer, normal = {vn[1], vn[2]}})
		end
		if (n > 0) then
			for k = n, 1, -1 do
				local vp = n0[k]
				local vn = vp.normal
				table.insert(n0, {vp[1] - vn[1] * width, vp[2] - vn[2] * width, normal = {-vn[1], -vn[2]}})
			end
			local pn0 = n0[n]
			local pn1 = n0[n + 1]
			local n2 = n * 2
			pn1.combined = {n + 1, n2 + 1}
			table.insert(n0, pn1)
			pn0.combined = {n, n2 + 2}
			table.insert(n0, pn0)
			n0.has_normal = true
			
			table.insert(t, n0)
		end
	end
	return t
end

---ObjectScene---
ObjectScene = class(Object)
function ObjectScene:ctor()
	self.update = true
	self.scene = self
	self.children = ObjectArray()
end

function ObjectScene:Update()
	for _, c in self.children:pairs() do
		c:Update()
	end
	self.update = false
end

function ObjectScene:Handle(func)
	if (func(self)) then
		for _, c in self.children:pairs() do
			if (not c:Handle(func)) then
				return false
			end
		end
		return true
	end
	return false
end

---SceneObject---
SceneObject = class(ObjectScene)
for k, _ in pairs(CMatrix) do
	if (k ~= '_class' and k ~= CMatrix) then
		local s = 'return function(self, ...) self.scene.update = true return self.mRoot:'..k..'(...) end'
		SceneObject[k] = load(s)()
	end
end
SceneObject.srlzClass = {}
SceneObject.srlzClass['SceneObject'] = SceneObject

--SceneObject.ATTACH_RELATIVE
SceneObject.ATTACH_SNAP = 1
SceneObject.ATTACH_WORLD = 2

--SceneObject.ATTACH_ROT_AFFECT_POS_ROT
SceneObject.ATTACH_ROT_AFFECT_POS = 1
SceneObject.ATTACH_ROT_IGNORE = 2

local mId = CMatrix[CMatrix]
function SceneObject:ctor(parent)
	self.mRoot = CMatrix()
	self[mId] = self.mRoot[mId]
	self.mCache = CMatrix()
	self.mModel = CMatrix()
	self.mWorld = self.mRoot
	self.sx = 1
	self.sy = 1
	self.sz = 1
	self.msx = 1
	self.msy = 1
	self.msz = 1
	if (parent) then
		self.parent = parent
		self.scene = parent.scene
		parent.children:insert(self)
		if (parent[SceneObject]) then
			self.attached = parent
			self.mWorld = self.mCache
		end
	else
		self.scene = self
	end
end

function SceneObject:Serialize()
	local o = {}
	o.class = 'SceneObject'
	o.mRow1 = {self.mRoot:GetRow1()}
	o.mRow2 = {self.mRoot:GetRow2()}
	o.mRow3 = {self.mRoot:GetRow3()}
	o.mRow4 = {self.mRoot:GetRow4()}
	o.sx, o.sy, o.sz = self.sx, self.sy, self.sz	
	return o
end

function SceneObject.NewSerialized(o)
	local c = SceneObject()
	c:LoadSerialized(o)
	return c
end

function SceneObject:LoadSerialized(o)
	self.mRoot:SetRow1(o.mRow1[1], o.mRow1[2], o.mRow1[3], o.mRow1[4])
	self.mRoot:SetRow2(o.mRow2[1], o.mRow2[2], o.mRow2[3], o.mRow2[4])
	self.mRoot:SetRow3(o.mRow3[1], o.mRow3[2], o.mRow3[3], o.mRow3[4])
	self.mRoot:SetRow4(o.mRow4[1], o.mRow4[2], o.mRow4[3], o.mRow4[4])
	self:SetScale(o.sx, o.sy, o.sz)
end	

function SceneObject:SetScale(x, y, z)
	self.sx, self.sy, self.sz = x, y, z
	self.scene.update = true
end

function SceneObject:Attach(parent, initial, rotRule, followScale)
	local o = parent
	while (o) do
		if (o == self) then
			return false 
		end
		o = o.parent
	end
	if (self.parent) then
		self.parent.children:remove_obj(self)
	end
	self.parent = parent
	self.scene = parent.scene
	self.scene.update = true
	parent.children:insert(self)
	if (not parent[SceneObject]) then
		self.attached = nil
		return
	end
	self.attached = parent
	self.rotRule = rotRule
	self.followScale = followScale
	if (initial == SceneObject.ATTACH_SNAP) then
		if (rotRule == SceneObject.ATTACH_ROT_AFFECT_POS_ROT) then
			self.mRoot:Identity()
		else
			self.mRoot:SetPosition(0, 0, 0)
		end
	elseif (initial == SceneObject.ATTACH_WORLD) then
		if (rotRule == SceneObject.ATTACH_ROT_IGNORE) then
			local x, y, z = self.mWorld:GetPosition()
			local xd, yd, zd = parent.mWorld:GetPosition()
			self.mRoot:SetPosition(x - xd, y - yd, z - zd)
		elseif (rotRule == SceneObject.ATTACH_ROT_AFFECT_POS) then
			local x, y, z = self.mWorld:GetPosition()
			x, y, z = parent.mWorld:PointTransformInv(x, y, z)
			self.mRoot:SetPosition(x, y, z)
		else --ATTACH_ROT_AFFECT_POS_ROT
			if (self.mRoot ~= self.mWorld) then
				self.mRoot:CopyFrom(self.mWorld)
			end
			self.mRoot:TransformByInverted(parent.mWorld)

		end
	end
	self.mWorld = self.mCache
end

function SceneObject:Detach(detachFromScene)
	local parent = self.parent
	local scene = self.scene
	self.followScale = false
	if (parent and (detachFromScene or parent ~= scene)) then
		parent.children:remove_obj(self)
		self.attached = nil
		if (not detachFromScene) then
			self.parent = self.scene
			self.parent.children:insert(self)
			self.mRoot:CopyFrom(self.mWorld)
			self.scene.update = true
		else
			self.parent = nil
			self.scene = self
		end
		self.mWorld = self.mRoot
	end
end

function SceneObject:Update()
	self:UpdateBegin()
	local parent = self.attached
	if (parent) then
		if (self.rotRule == SceneObject.ATTACH_ROT_IGNORE) then
			self.mWorld:CopyFrom(self.mRoot)
			local x, y, z = self.mWorld:GetPosition()
			local x1, y1, z1 = parent.mWorld:GetPosition()
			self.mWorld:SetPosition(x + x1, y + y1, z + z1)
		elseif (self.rotRule == SceneObject.ATTACH_ROT_AFFECT_POS) then
			self.mWorld:CopyFrom(self.mRoot)
			local x, y, z = self.mWorld:GetPosition()
			x, y, z = parent.mWorld:PointTransform(x, y, z)
			self.mWorld:SetPosition(x, y, z)
		else --ATTACH_ROT_AFFECT_POS_ROT
			self.mWorld:SetByMultiplied(self.mRoot, parent.mWorld)
		end
	end
	self:RenderBegin()
	self.msx, self.msy, self.msz = self.sx, self.sy, self.sz
	if (self.followScale) then
		self.msx, self.msy, self.msz = self.msx * parent.msx, self.msy * parent.msy, self.msz * parent.msz
	end
	self.mModel:SetByScaled(self.msx, self.msy, self.msz, self.mWorld)
	for _, c in self.children:pairs() do
		c:Update()
	end
	self.update = false
end

function SceneObject:UpdateBegin() end

function SceneObject:RenderBegin() end

-- local function CacGridLinePos(len, front, back, left, right, xLineZ, zLineX)
	-- if (back > xLineZ) then
		-- local d = back - xLineZ
		-- xLineZ = front - (d - d // len * len)
	-- elseif (front < xLineZ) then
		-- local d = xLineZ - front
		-- xLineZ = back + d - d // len * len
	-- end
	-- if (left > zLineX) then
		-- local d = left - zLineX
		-- zLineX = right - (d - d // len * len)
	-- elseif (right < zLineX) then
		-- local d = zLineX - right
		-- zLineX = left + d - d // len * len
	-- end
	-- return xLineZ, zLineX
-- end

-- local function GetLineFade(noneFadeCount, fadeCount, fade, pos, space)
	-- local i = math.abs(pos) // space
	-- if (fadeCount == 0 or i % noneFadeCount == 0) then
		-- return 1
	-- elseif (i % fadeCount == 0) then
		-- return fade
	-- end
	-- return 0
-- end
	
-- local function GridFunc(grid, vb, vwp, cb, cwp, ib, iwp, ibStart)
	-- local x, y, z = grid.camera.mWorld:GetPosition()
	-- y = math.abs(y)
	-- local count = math.ceil(gridLineLen / 2 / lineSpace)
	-- local lenH = lineSpace * count + lineSpace / 2
	-- local len = lenH * 2
	
	-- local front = z + lenH
	-- local back = z - lenH
	-- local left = x - lenH
	-- local right = x + lenH

	-- local t = math.tan(fov * 0.5 * degToArc)
	-- local n = math.ceil(y * t * 2 / lineSpace)
	-- local fadeLevel = math.floor(math.log(n, fadePerCount))
	-- local noneFadeCount = fadePerCount ^ fadeLevel
	-- local fadeCount = 0
	-- local fade = 0
	-- if (fadeLevel > 0) then
		-- fadeCount = noneFadeCount / fadePerCount
		-- local y0 = noneFadeCount * 0.5 / t
		-- local y1 = math.max(y, y0 * fadePerCount * lineSpace * 0.25)
		-- fade = 1 - (y - y0) / (y1 - y0)
	-- end
	
	-- --0, 0 lines
	-- local xLineZ, zLineX = CacGridLinePos(len, front, back, left, right, 0, 0)
	-- CAddLine(left, 0, xLineZ, right, 0, xLineZ, vb, vwp)
	-- vwp = APPEND
	-- CAddLine(zLineX, 0, front, zLineX, 0, back, vb, vwp)
	-- local f = GetLineFade(noneFadeCount, fadeCount, fade, xLineZ, lineSpace)
	-- CAddUByte4(150, 150, 150, 80 * f, cb, cwp, 2)
	-- cwp = APPEND
	-- f = GetLineFade(noneFadeCount, fadeCount, fade, zLineX, lineSpace)
	-- CAddUByte4(150, 150, 150, 80 * f, cb, cwp, 2)
	
	-- local d = lineSpace
	-- for i = 1, count do
		-- local xLineZ1, zLineX1 = CacGridLinePos(len, front, back, left, right, d, d)
		-- local xLineZ2, zLineX2 = CacGridLinePos(len, front, back, left, right, -d, -d)
		
		-- CAddLine(left, 0, xLineZ1, right, 0, xLineZ1, vb, vwp)
		-- CAddLine(zLineX1, 0, front, zLineX1, 0, back, vb, vwp)
		-- CAddLine(left, 0, xLineZ2, right, 0, xLineZ2, vb, vwp)
		-- CAddLine(zLineX2, 0, front, zLineX2, 0, back, vb, vwp)
		
		-- f = GetLineFade(noneFadeCount, fadeCount, fade, xLineZ1, lineSpace)
		-- CAddUByte4(150, 150, 150, 80 * f, cb, cwp, 2)
		-- f = GetLineFade(noneFadeCount, fadeCount, fade, zLineX1, lineSpace)
		-- CAddUByte4(150, 150, 150, 80 * f, cb, cwp, 2)
		-- f = GetLineFade(noneFadeCount, fadeCount, fade, xLineZ2, lineSpace)
		-- CAddUByte4(150, 150, 150, 80 * f, cb, cwp, 2)
		-- f = GetLineFade(noneFadeCount, fadeCount, fade, zLineX2, lineSpace)
		-- CAddUByte4(150, 150, 150, 80 * f, cb, cwp, 2)
		-- d = d + lineSpace
	-- end
	
	-- count = (count * 4 + 2) * 2
	-- CAddLineListIndex(count, ib, iwp, ibStart)
	-- return count, count
-- end
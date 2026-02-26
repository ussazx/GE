-----presets.lua-----
require 'global'

g_iconFolder = AddPoly2D(true, { {5, 0}, {50, 0}, {55, 5},
						{110, 5}, {110, 10}, {0, 10}, {0, 5} },
						{ {0, 12}, {110, 12}, {110, 62}, {0, 62} })

local s = 12
local h = s / 2
g_iconArrowHeadL = AddPoly2D(true, { {0, h}, {s, 0}, {s, s} })
g_iconTriangleR = AddPoly2D(true, { {0, 0}, {s, h}, {0, s} })
g_iconTriangleU = AddPoly2D(true, { {h, 0}, {s, s}, {0, s} })
g_iconTriangleD = AddPoly2D(true, { {0, 0}, {s, 0}, {h, s} })
g_iconTriangleDR = AddPoly2D(true, { {0, s}, {s, 0}, {s, s} })

local r = 10
g_iconMagnifier = AddPoly2D(true, DrawLine(3, false, true, MakeCircle(0, 0, r, 16)), 
	DrawLine(4, true, false, {r + r - 4, r + r - 4}, {r + 10, r + 10}))

g_iconLine = AddPoly2D(true, DrawLine(5, false, false, {0, 0}, {100, 0}, {50, 100}, {366, 210}, {500, 710}))

local o = DrawLine(4, false, true, MakeCircle(0, 0, 10, 6))
o.color = Color(255, 255, 255, 255)
g_iconPreset = AddPoly2D(true, o)

g_iconLine1 = AddPoly2D(false, DrawLine(10, false, false, {100, 100}, {100, 300}))

r = 5
o = MakeCircle(0, 0, r, 8)
o.color = Color(255, 200, 0, 255)
g_iconUnsaved = AddPoly2D(true, o)

local hx = 10
local hy = 5
local hgt = 12
local gap = 1
local hgap = gap / 2
local d = hy / hx
g_iconGeom1 = AddPoly2D(true, {{0, hy}, {hx, 0}, {hx * 2, hy}, {hx, hy * 2}},
	{{0, hy + gap}, {hx - hgap, hy * 2 + gap - hgap * d}, {hx - hgap, hy * 2 + gap - hgap * d + hgt}, {0, hy + gap + hgt}},
	{{hx + hgap, hy * 2 + gap - hgap * d}, {hx * 2, hy + gap}, {hx * 2, hy + gap + hgt}, {hx + hgap, hy * 2 + gap - hgap * d + hgt}})
	
hx = 20
hy = 10
hgt = 22
gap = 2
hgap = gap / 2
d = hy / hx
g_iconGeom2 = AddPoly2D(true, {{0, hy}, {hx, 0}, {hx * 2, hy}, {hx, hy * 2}},
	{{0, hy + gap}, {hx - hgap, hy * 2 + gap - hgap * d}, {hx - hgap, hy * 2 + gap - hgap * d + hgt}, {0, hy + gap + hgt}},
	{{hx + hgap, hy * 2 + gap - hgap * d}, {hx * 2, hy + gap}, {hx * 2, hy + gap + hgt}, {hx + hgap, hy * 2 + gap - hgap * d + hgt}})

local file = { {0, 0}, {40, 0}, {50, 10}, {50, 60}, {0, 60} }
file.color = Color(150, 150, 150, 255)
local o1 = MakeRect(10, 15, 30, 10)
o1.color = Color(255, 0, 0, 180)
local o2 = MakeRect(10, 25, 30, 10)
o2.color = Color(0, 255, 0, 180)
local o3 = MakeRect(10, 35, 30, 10)
o3.color = Color(0, 0, 255, 180)
g_iconFB = AddPoly2D(true, file, o1, o2, o3)

local n = 10
g_iconLNavi = AddPoly2D(true, DrawLine(7, true, false, {n, 0}, {0, n}, {n, n * 2}))
g_iconRNavi = AddPoly2D(true, DrawLine(7, true, false, {0, 0}, {n, n}, {0, n * 2}))

-----Camera-----
Camera = class(SceneObject)
function Camera:ctor()
	self.mView = CMatrix()
end

function Camera:RenderBegin()
	self.mView:SetByMatrixToView(self.mWorld)
end

ObjectCoord = class(Model, g_assets.Geometry['baseCube'])

function ObjectCoord:ctor(camera)
	self.camera = camera
	
	self.arrowY = Model(g_assets.Geometry['arrowY'])
	self.arrowY:Attach(self, nil, nil, true)
	
	self.arrowX = Model(g_assets.Geometry['arrowX'])
	self.arrowX:Attach(self, nil, nil, true)
	
	self.arrowZ = Model(g_assets.Geometry['arrowZ'])
	self.arrowZ:Attach(self, nil, nil, true)
	
	g_perObjInst1:SetObjectValue(1, self.arrowY, 255, 0, 0, 180)
	g_perObjInst1:SetObjectValue(1, self.arrowX, 0, 255, 0, 180)
	g_perObjInst1:SetObjectValue(1, self.arrowZ, 0, 0, 255, 180)
end

function ObjectCoord:SetColorY(r, g, b, a)
	g_perObjInst1:SetObjectValue(1, self.arrowY, r, g, b, a)
end

function ObjectCoord:SetColorX(r, g, b, a)
	g_perObjInst1:SetObjectValue(1, self.arrowX, r, g, b, a)
end

function ObjectCoord:SetColorZ(r, g, b, a)
	g_perObjInst1:SetObjectValue(1, self.arrowZ, r, g, b, a)
end

function ObjectCoord:RestoreColors(x, y, z)
	if (x) then
		g_perObjInst1:SetObjectValue(1, self.arrowY, 255, 0, 0, 180)
	end
	if (y) then
		g_perObjInst1:SetObjectValue(1, self.arrowX, 0, 255, 0, 180)
	end
	if (z) then
		g_perObjInst1:SetObjectValue(1, self.arrowZ, 0, 0, 255, 180)
	end
end	
	
function ObjectCoord:RenderBegin()
	local x, y, z = self.mWorld:GetPosition()
	local _, _, z = self.camera.mView:PointTransform(x, y, z)
	local n = math.abs(z) / 10
	self:SetScale(n, n, n)
end
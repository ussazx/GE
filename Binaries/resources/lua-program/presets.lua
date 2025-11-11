-----presets.lua-----
require 'geometry'

-----Camera-----
Camera = class(SceneObject)
function Camera:ctor()
	self.mView = CMatrix()
end

function Camera:RenderBegin()
	self.mView:SetByMatrixToView(self.mWorld)
end

ObjectCoord = class(Model, g_baseCube)

function ObjectCoord:ctor(camera)
	self.camera = camera
	
	self.arrowY = Model(g_arrowY)
	self.arrowY:Attach(self, nil, nil, true)
	
	self.arrowX = Model(g_arrowX)
	self.arrowX:Attach(self, nil, nil, true)
	
	self.arrowZ = Model(g_arrowZ)
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
	_, _, z = self.camera.mView:PointTransform(x, y, z)
	local n = math.abs(z) / 10
	self:SetScale(n, n, n)
end
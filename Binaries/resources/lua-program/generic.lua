---generic.lua---
require 'class'

function Print(...)
	print(...)
	if (Flush) then
		Flush()
	end
end

-----point-----
Point = class()

function Point:ctor(o, x, y)
	if (o) then
		self.x = o.x or 0
		self.y = o.y or 0
	else
		self.x = x or 0
		self.y = y or 0
	end
end

function Point:move_to(x, y)
	self.x = x
	self.y = y
end

function Point:move(x, y)
	self.x = self.x + x
	self.y = self.y + y
end

-----bound-----
Bound = class()

function Bound:ctor(o, left, right, top, bottom)
	if (o) then
		self.left = o.left or 0
		self.right = o.right or 0
		self.top = o.top or 0
		self.bottom = o.bottom or 0
	else
		self.left = left or 0
		self.right = right or 0
		self.top = top or 0
		self.bottom = bottom or 0
	end
end

-----rect-----
Rect = class(Point)

function Rect:ctor(o, x, y, width, height)
	if (o) then
		self.x = o.x or 0
		self.y = o.y or 0
		self.width = o.width or 0
		self.height = o.height or 0
	else
		self.x = x or 0
		self.y = y or 0
		self.width = width or 0
		self.height = height or 0
	end
end

function Rect:ctor(o, x, y, width, height)
	if (o) then
		self.x = o.x or 0
		self.y = o.y or 0
		self.width = o.width or 0
		self.height = o.height or 0
	else
		self.x = x or 0
		self.y = y or 0
		self.width = width or 0
		self.height = height or 0
	end
end

function Rect:intersect(bound)
	if (self.x >= bound.right or self.y >= bound.bottom) then
		return nil, nil, nil
	end

	local right = self.x + self.width
	if (right <= bound.left) then
		return nil, nil, nil
	end

	local bottom = self.y + self.height
	if (bottom <= bound.top) then
		return nil, nil, nil
	end

	local r = Rect(self)
	local u0 = 0
	local v0 = 0
	
	if (r.x < bound.left) then
		u0 = bound.left - r.x
		r.x = bound.left
		r.width = r.width - u0
	end
	if (right > bound.right) then
		r.width = r.width - (right - bound.right)
	end
	if (r.y < bound.top) then
		v0 = bound.top - r.y
		r.y = bound.top
		r.width = r.width - v0
	end
	if (bottom > bound.bottom) then
		r.height = r.height - (bottom - bound.bottom)
	end

	return r, u0, v0
end
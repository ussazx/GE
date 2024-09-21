---class---
local function dtor(o)
	local c = o
	while (c ~= nil) do
		if (c._base == nil or c.dtor ~= c._base.dtor) then
			c.dtor(o)
		end
		c = c._base
	end
end

local function instantiate(c, ...)
	local o = setmetatable({}, getmetatable(c).class)
	
	if (o.ctor) then
		o:ctor(...)
	end
	return o
end

local function instantiate_d(c, base, ...)
	local o
	local b = getmetatable(base)
	if (b and b.__index == c._base) then
		o = setmetatable(base, getmetatable(c).class)
		if (o.ctor and o.ctor ~= o._base.ctor) then
			o:ctor(...)
		end
	else
		o = {}
		local bases = getmetatable(c).bases
		for _, b in pairs(bases) do
			o = setmetatable(o, b.class)
		end
		o = setmetatable(o, getmetatable(c).class)
		
		for _, b in pairs(bases) do
			if (b.ctor) then
				b.ctor(o)
			end
		end
		
		if (o.ctor and o.ctor ~= o._base.ctor) then
			o:ctor(base, ...)
		end
	end
	
	return o
end

local function _clone(o)
	local c = getmetatable(o).__index()
	for k, v in pairs(o) do
		c[k] = v
	end
	return c
end

function class(base_class)
	local bases = {}
	_call = instantiate
	if (base_class) then
		_call = instantiate_d
		for _, b in pairs(getmetatable(base_class).bases) do
			table.insert(bases, b)
		end
		local b_ctor
		if (base_class.ctor and (base_class._base == nil or base_class.ctor ~= base_class._base.ctor)) then
			b_ctor = base_class.ctor
		end
		table.insert(bases, {class = getmetatable(base_class).class, ctor = b_ctor})
	end
	local c = setmetatable({_base = base_class, clone = _clone}, {__index = base_class, __call = _call})
	c._class = c
	local m = getmetatable(c)
	m.class = {__index = c, __gc = dtor}
	m.bases = bases
	return c
end
---class---
local function assign(o, c)
	if (c._base) then
		o = assign(o, c._base)
	end
	return setmetatable(o, c._class)
end

local function ctor(o, c, ...)
	if (c._base) then
		ctor(o, c._base)
	end
	if (c.ctor and (c._base == nil or c.ctor ~= c._base.ctor)) then
		c.ctor(o, ...)
	end
end

local function dtor(o)
	local c = o
	while (c ~= nil) do
		if (c._base == nil or c.dtor ~= c._base.dtor) then
			c.dtor(o)
		end
		c = c._base
	end
end

local function instantiate(c, base, ...)
	local o
	local b = getmetatable(base)
	if (b and c._base and b == c._base._class) then
		o = setmetatable(base, c._class)
		if (o.ctor and o.ctor ~= base.ctor) then
			o:ctor(...)
		end
	else
		o = assign({}, c)
		ctor(o, c, base, ...)
	end
	return o
end

function class(base_class)
	local c = setmetatable({_base = base_class}, {__index = base_class, __call = instantiate})
	c._class = {__index = c, __gc = dtor}
	return c
end

-- local function assign(o, c)
	-- if (c._base) then
		-- assign(o, c._base)
	-- end
	-- for k, v in pairs(c) do
		-- o[k] = v
	-- end
-- end

-- local function ctor(o, c, ...)
	-- if (c._base) then
		-- ctor(o, c._base)
	-- end
	-- if (c.ctor) then
		-- c.ctor(o, ...)
	-- end
-- end

-- local function dtor(o)
	-- if (o.dtor) then
		-- o.dtor(o)
	-- end
	-- if (o._base) then
		-- dtor(o._base)
	-- end
-- end

-- local function instantiate(c, base, ...)
	-- local m = getmetatable(base)
	-- if (m and m == c._base) then
		-- for k, v in pairs(c) do
			-- base[k] = v
		-- end
		-- if (c.ctor) then
			-- c.ctor(base, ...)
		-- end
		-- return base
	-- end
	-- local o = setmetatable({}, c)
	-- assign(o, c)
	-- ctor(o, c, base, ...)
	-- return o
-- end		

-- function class(base_class)
	-- return setmetatable({_base = base_class, __gc = dtor}, {__call = instantiate})
-- end
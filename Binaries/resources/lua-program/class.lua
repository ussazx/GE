---class---
local function assign(o, c)
	if (c._base) then
		o = assign(o, c._base)
	end
	return setmetatable(o, c._class)
end

local function ctor(o, c, ...)
	if (c._base) then
		if (c._pcb) then
			c._bctor(o, c._base, ...)
		else
			c._bctor(o, c._base, c._bargs)
		end
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

local function instantiate(c, ...)
	local o = assign({}, c)
	ctor(o, c, ...)
	return o
end

local function new_class(base_class)
	local c = setmetatable({_ctor = ctor, _base = base_class}, {__index = base_class, __call = instantiate})
	c._class = {__index = c, __gc = dtor}
	c._bctor = ctor
	c[c] = c
	return c
end

function class(base_class, ...)
	local c = new_class(base_class)
	local args = {...}
	local n = #args
	if (n > 0) then
		c._bargs = args
		local s = [[return function (o, c, args)
			if (c._base) then
				c._ctor(o, c._base)
			end
			c._ctor(o, c]]
		for i = 1, n do
			s = s .. string.format(', args[%q]', i)
		end
		s = s .. ') end'
		c._bctor = load(s, '', 't')()
	end
	return c
end

function class2(base_class)
	local c = new_class(base_class)
	c._pcb = true
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
--require 'a'
require 'object'
--require 'utility'
require 'class'

a = class(Object)

b = class(a)

c = class(b)


aa = a()
cc = c()

aa:delist()
aa = nil

bb = b()

collectgarbage('collect')

print(bb.id, cc.id)

local env =
{
	i = 0,
	Inc = function ()
		local n = i
		i = i + 1
		return n
	end,
	
	j = 1,
	Inc2 = function ()
		local n = j
		j = j * 2
		return n
	end
}

local function Func(name, num)
	s = ''
	for i = 0, num do
		s = s..name..'_'..i..' = {loc = Inc(), flag = Inc2()} '
	end
	load(s, s, 'bt', env)()
end

--Func('aa', 5)

--print(env.aa_5.loc)

t = setmetatable({}, {__mode = 'kv'})

a = {a = 1}

b = {a = 2}

c = {a = 3}

d = {a = 4}

e = {a = 5}

table.insert(t, a) --1

a = nil
collectgarbage('collect')

table.insert(t, b) --1

table.insert(t, c) --2

b = nil
collectgarbage('collect')

table.insert(t, d) --1


table.insert(t, e) --3


print('\n')

for k, v in pairs(t) do
	print(k, v.a)
end
print('\n')

a = {}
table.insert(a, 1)
a[2] = 2
a[5] = 5
a['a'] = 1

table.insert(a, 3, 3)

table.insert(a, 4, 4)

for k, v in pairs(a) do
	print(k, v)
end

print(z ~= 1)

function f(...)
	local b = ...
	print(b)
	return ...
end

print(math.floor(-0.5))

f(1, 2)

a = 1
print(nil)

--init create rp page
--rp = {}
--rp.subpasses = {num = 0}
--rp.subpasses[0] = {sample = 1, rt = {}, ds_format = FMT_UNDEFINED}
--rp.subpasses.num += 1
--set subpass rt
--rp.subpasses[0].rt[0].fmt = R8G8B8
--rp.subpasses[0].rt[0].resolved = true
--rp.subpasses[0].rt[0].to_input = {num = 0}
--rp.subpasses[0].rt[0].to_input[0] = 1 
--rp.subpasses[0].rt[0].to_input.num += 1

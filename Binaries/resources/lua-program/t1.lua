require "class"

local a = Class()
a.x = 1

function a:f()
	print('a', self.x)
end

function a:Dtor()
	print('a.Dtor', self.x)
end


b = Class(a)
b.x = 2
b.y = 11

function b:Ctor()
	self.x = 1
end

function b:Dtor()
	print('b.Dtor', self.x)
end

function b:f()
	b._super:f()
	print('b', self.x)
end

function b:f2()
	print('b', self.x)
end

c = Class(b)
c.x = 3

function c:Dtor()
	print('c.Dtor', self.x)
end

function c:f()
	c._super:f()
	print('c', self.x)
end

cc = c(c._super(c._super._super()))
cc.x = 1
a = 1
cc:f()

a = {1, 3, 3, 1, 1, 5, 5, 7}
table.insert(a, 9)

i = 1
function Iter()
	return function()
		return a[i]
	end
end

for v in Iter() do
	if (v == 1) then
		table.remove(a, i)
	else
		i = i + 1
	end
end

for i, v in pairs(a) do
	print(i, v)
end
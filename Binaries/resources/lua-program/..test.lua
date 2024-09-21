---..test.lua--
require 'object'
require 'utility'
require 'graphic'

Print('--object test--')
a = Object()
function a:f(i)
	Print("f", i)
end

b = Object()

b:bind_event(1, a, a.f)
b:process_event(1, 3)
b:process_event(2, 3)

a:delist()
a = nil

b:process_event(1, 3)

--collectgarbage('collect')

c = Object()

a = Object()

Print(a.id, b.id, c.id)

Print('\n--utility test--')
o = {['x'] = 1, [true] = 4 / 3, ['z'] = {q = 1, qq = 2}}

s = SerializeToText(o)
Print(s)

x = load('return '..s)()

Print('\n--VBO test--')

d = {}
d[VB_ELEM_FLOAT2] = 4
d[VB_ELEM_FLOAT3] = 2
d.vtx_count = 1024
vbo = VBO(d)
vbo:at(1025)
vbo:set_val(VB_ELEM_FLOAT2 + 2, 1, 1)
Print(vbo:get_val(VB_ELEM_FLOAT2 + 2))

Print('\n---DrawCmd test---')
dc = DrawCmd()

dc:PushViewport(0, 0, 1, 1, 1, 1)
dc:PushViewport(0, 1, 1, 1, 1, 1)

dc:PopViewport()
dc:PopViewport()

dc:PushClipRect(0, 0, 1, 1)
dc:PushClipRect(0, 2, 1, 1)

--dc:PopClipRect()

for i = 1, dc.s[dc_viewport].max do
	Print(dc.s[dc_viewport][i].s.width)
end

for i = 1, dc.s[dc_cliprect].max do
	Print(dc.s[dc_cliprect][i].s.width)
end

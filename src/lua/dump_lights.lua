local n_dir = nngn.lighting:n_dir_lights()
print(string.format("dir: %d", n_dir))
local i = 0
while i < n_dir do
    local l = nngn.lighting:light(i)
    print(string.format("color: %f, %f, %f", l:color()))
    i = i + 1
end
local n = nngn.lighting:n_lights()
print(string.format("point: %d", n - n_dir))
while i < n do
    local l = nngn.lighting:light(i)
    print(string.format("color: %f, %f, %f", l:color()))
    i = i + 1
end

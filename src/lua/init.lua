dofile "src/lua/path.lua"
dofile "src/lua/debug.lua"
require("nngn.lib.graphics").init()
require("nngn.lib.collision").init()
nngn.socket:init("sock")

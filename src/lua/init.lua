dofile "src/lua/path.lua"
require("src/lua/debug")
require("nngn.lib.compute").init()
require("nngn.lib.graphics").init()
require("nngn.lib.collision").init()
nngn.socket:init("sock")

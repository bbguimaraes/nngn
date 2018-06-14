local entity = require "nngn.lib.entity"
dofile "src/lua/input.lua"

entities = {
    entity.load(nil, "src/lson/old_man.lua", {pos = {-32, 0}}),
}

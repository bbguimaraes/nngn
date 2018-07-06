if Platform.DEBUG then
    require("src/lua/strict")
    utils = require("nngn.lib.utils")
    D = require("nngn.lib.debug")
else
    assert = function(...) return ... end
end

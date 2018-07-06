if Platform.DEBUG then
    require("src/lua/strict")
else
    assert = function(...) end
end

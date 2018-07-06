do
    local load = function(_, f) return dofile(f) end
    table.insert(package.searchers, function(m)
        local _, e = string.find(m, "^nngn%.")
        if not e then return end
        local f = string.sub(m, e + 1)
        f = string.gsub(f, "%.", "/")
        f = "src/lua/" .. f .. ".lua"
        return load, f
    end)
end

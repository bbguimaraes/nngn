do
    local load <const> = function(_, f) return dofile(f) end
    table.insert(package.searchers, function(m)
        m = m:match("^nngn%.(.*)$")
        if m then
            return load, ("src/lua/%s.lua"):format(m:gsub("%.", "/"))
        end
    end)
end

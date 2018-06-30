local function load(size, name)
    name = name or "DejaVuSans.ttf"
    local ok, ret = pcall(io.popen, "fc-match --format %{file} " .. name)
    if ok then
        name = ret:read()
        ret:close()
    elseif ret ~= "'popen' not supported" then
        error(ret)
    end
    return nngn.fonts:load(size, name)
end

return {
    load = load,
}

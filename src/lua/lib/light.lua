local function sun(show)
    local l = nngn:lighting():sun_light()
    if show == nil then show = l == nil end
    if not show then
        if not l then return end
        nngn:lighting():remove_light(l)
        nngn:lighting():set_sun_light(nil)
    else
        if l then return end
        local sun = nngn:lighting():sun()
        sun:set_incidence(math.rad(315))
        sun:set_time_ms(6 * 1000 * 60 * 60)
        l = nngn:lighting():add_light(Light.DIR)
        l:set_color(1, 0.8, 0.5, 1)
        nngn:lighting():set_sun_light(l)
    end
end

return {
    sun = sun,
}

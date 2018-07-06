local function heartbeat(f)
    return nngn:schedule():next(Schedule.HEARTBEAT, f)
end

return {
    heartbeat = heartbeat,
}

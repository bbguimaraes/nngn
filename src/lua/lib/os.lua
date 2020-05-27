local function read_nonblock(f, ...)
    return (function(f, ret, err, ...)
        if ret then
            return ret, err, ...
        elseif Platform.errno() ~= Platform.EAGAIN then
            return nil, err
        else
            Platform.clear_errno()
            Platform.clearerr(f)
        end
    end)(f, f:read(...))
end

return {
    read_nonblock = read_nonblock,
}

namespace {

template<typename I>
auto as_table(const elysian::lua::ThreadView &L, I b, I e) {
    assert(b <= e);
    const auto n = static_cast<lua_Integer>(e - b);
    const elysian::lua::StaticStackTable ret =
        L.createTable(static_cast<int>(n));
    for(lua_Integer i = 1; i <= n; ++i)
        ret.setFieldRaw(i, *b++);
    return ret;
}

template<typename T>
auto as_table(const elysian::lua::ThreadView &L, const T &v)
    { return as_table(L, begin(v), end(v)); }

}

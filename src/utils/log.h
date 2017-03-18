#ifndef NNGN_LOG_H
#define NNGN_LOG_H

#include <array>
#include <sstream>
#include <string>
#include <tuple>

#define NNGN_LOG_CONTEXT(n) NNGN_LOG_CONTEXT2(n, nullptr)
#define NNGN_LOG_CONTEXT_F() NNGN_LOG_CONTEXT2(__func__, nullptr)
#define NNGN_LOG_CONTEXT_CF(c) NNGN_LOG_CONTEXT2(#c, __func__)
#define NNGN_LOG_CONTEXT2(n0, n1) \
    const auto NNGN_LOG_CONTEXT_VAR(__LINE__) = nngn::Log::context(n0, n1);
#define NNGN_LOG_CONTEXT_VAR(l) NNGN_LOG_CONTEXT_JOIN(lc_, l)
#define NNGN_LOG_CONTEXT_JOIN(x, y) x##y

namespace nngn {

class Log {
public:
    static constexpr uint8_t MAX_DEPTH = 16;
private:
    static std::ostream *stream;
    static thread_local uint8_t depth;
    using record = std::tuple<const char*, const char*>;
    static thread_local std::array<record, MAX_DEPTH> stack;
    friend std::ostream &operator<<(std::ostream &os, Log::record r);
public:
    struct context {
        context(const char *name0, const char *name1);
        context(const context&) = delete;
        context(const context&&) = delete;
        context &operator=(const context&) = delete;
        context &operator=(const context&&) = delete;
        ~context();
    };
    class replace {
        std::ostream *old;
    public:
        explicit replace(std::ostream *l) : old(Log::set(l)) {}
        replace(const replace&) = delete;
        replace(const replace&&) = delete;
        replace &operator=(const replace&) = delete;
        replace &operator=(const replace&&) = delete;
        ~replace() { Log::set(this->old); }
    };
    static std::ostream *set(std::ostream *l);
    static std::ostream &l();
    static void perror(const char *s = nullptr);
    template<typename F> static std::string capture(F f);
    template<typename F> static decltype(auto) with_context(
        const char *name, F f);
};

template<typename F>
std::string Log::capture(F f) {
    std::stringstream ss;
    const Log::replace o(&ss);
    f();
    return ss.str();
}

template<typename F>
decltype(auto) Log::with_context(const char *name, F f) {
    NNGN_LOG_CONTEXT(name);
    return f();
}

}

#endif

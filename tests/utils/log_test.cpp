#include "utils/log.h"

#include "log_test.h"

void LogTest::capture() {
    const auto s = nngn::Log::capture([]() { nngn::Log::l() << "test"; });
    QCOMPARE(s.c_str(), "test");
}

void LogTest::context() {
    NNGN_LOG_CONTEXT_CF(LogTest);
    const auto s = nngn::Log::capture([]() { nngn::Log::l() << "test"; });
    QCOMPARE(s.c_str(), "LogTest::context: test");
}

void LogTest::with_context() {
    nngn::Log::with_context("context0", []() {
        const auto s0 =
            nngn::Log::capture([]() { nngn::Log::l() << "test0"; });
        QCOMPARE(s0.c_str(), "context0: test0");
        const auto s1 = nngn::Log::with_context("context1", []() {
            return nngn::Log::capture([]() { nngn::Log::l() << "test1"; }); });
        QCOMPARE(s1.c_str(), "context0: context1: test1");
    });
}

void LogTest::replace() {
    std::stringstream ss0, ss1;
    const nngn::Log::replace r0(&ss0);
    nngn::Log::l() << 0;
    QCOMPARE(ss0.str().c_str(), "0");
    {
        const nngn::Log::replace r1(&ss1);
        nngn::Log::l() << 1;
        QCOMPARE(ss1.str().c_str(), "1");
    }
    nngn::Log::l() << 2;
    QCOMPARE(ss0.str().c_str(), "02");
}

template<size_t ...I> static auto make_full_stack_h(std::index_sequence<I...>)
    { return std::array{nngn::Log::context(&"context"[I * 0], nullptr)...}; }
template<size_t N> static auto make_full_stack()
    { return make_full_stack_h(std::make_index_sequence<N>()); }

void LogTest::max() {
    const auto cs = make_full_stack<nngn::Log::MAX_DEPTH>();
    NNGN_LOG_CONTEXT("error");
}

QTEST_MAIN(LogTest)

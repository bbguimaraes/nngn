#ifndef NNGN_TEST_REGISTRY_H
#define NNGN_TEST_REGISTRY_H

#include <QTest>

class TestRegistry {
    using V = std::vector<std::unique_ptr<QObject>>;
    static V &m_tests() { static V v; return v; }
public:
    static const V &tests() { return TestRegistry::m_tests(); }
    template<typename T> struct R { R() { add<T>(); } };
    template<typename T> static void add()
        { TestRegistry::m_tests().push_back(std::make_unique<T>()); }
};

#define NNGN_TEST(T) static TestRegistry::R<T> NNGN_TEST_REGISTRATION_##T;

#endif

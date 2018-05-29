#include <cmath>

#include "math/math.h"
#include "render/sun.h"

#include "sun_test.h"

using namespace std::chrono_literals;

using nngn::vec3;

Q_DECLARE_METATYPE(std::chrono::hours)
Q_DECLARE_METATYPE(vec3)

void SunTest::constructor() {
    nngn::Sun sun;
    QCOMPARE(sun.incidence(), 0.0f);
    QCOMPARE(sun.time().count(), 0);
    QCOMPARE(sun.time_ms(), 0);
    QCOMPARE(sun.dir(), vec3(0, -1, 0));
}

void SunTest::dir_data() {
    const float sin_a = std::sin(nngn::Math::radians(45.0f));
    QTest::addColumn<std::chrono::hours>("time");
    QTest::addColumn<vec3>("result");
    QTest::newRow("0h") << 0h << vec3(0, -sin_a, -sin_a);
    QTest::newRow("6h") << 6h << vec3(-1, 0, 0);
    QTest::newRow("12h") << 12h << vec3(0, sin_a, sin_a);
    QTest::newRow("18h") << 18h << vec3(1, 0, 0);
}

void SunTest::dir() {
    const float a = nngn::Math::radians(45.0f);
    QFETCH(std::chrono::hours, time);
    QFETCH(vec3, result);
    nngn::Sun sun;
    sun.set_incidence(a);
    sun.set_time(time);
    const vec3 dir = sun.dir();
    for(int i = 0; i < 3; ++i)
        if(!qFuzzyCompare(dir[i] + 1, result[i] + 1))
            QCOMPARE(dir, result);
}

QTEST_MAIN(SunTest)

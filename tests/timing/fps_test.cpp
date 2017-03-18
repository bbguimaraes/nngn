#include "timing/fps.h"

#include "fps_test.h"

void FpsTest::constructor() {
    nngn::FPS fps(1);
    QCOMPARE(fps.sec_count, 0u);
}

void FpsTest::sec() {
    using namespace std::chrono_literals;
    auto t = nngn::Timing::clock::now();
    nngn::FPS fps(1);
    fps.init(t);
    fps.frame(t += 500ms);
    QCOMPARE(fps.sec_count, 1u);
    QCOMPARE(fps.sec_last, 0u);
    fps.frame(t += 500ms);
    QCOMPARE(fps.sec_count, 0u);
    QCOMPARE(fps.sec_last, 1u);
    fps.frame(t += 1200ms);
    QCOMPARE(fps.sec_count, 0u);
    QCOMPARE(fps.sec_last, 0u);
    for(unsigned int i = 0; i < 8; ++i)
        fps.frame(t += 100ms);
    QCOMPARE(fps.sec_count, 8u);
    QCOMPARE(fps.sec_last, 0u);
    fps.frame(t += 1000ms);
    QCOMPARE(fps.sec_count, 0u);
    QCOMPARE(fps.sec_last, 8u);
}

void FpsTest::avg() {
    using namespace std::chrono_literals;
    const unsigned int N = 100;
    auto t = nngn::Timing::clock::now();
    nngn::FPS fps(N);
    fps.init(t);
    QCOMPARE(fps.avg, 0.0f);
    for(unsigned int i = 0; i < N; ++i)
        fps.frame(t += 10ms);
    QCOMPARE(fps.avg, 100.0f);
    for(unsigned int i = 0; i < N; ++i)
        fps.frame(t += 20ms);
    QCOMPARE(fps.avg, 50.0f);
    for(float i = 0; i < N; ++i) {
        QCOMPARE(fps.avg, N / ((0.02f * (N - i) + 0.01f * i)));
        fps.frame(t += 10ms);
    }
    QCOMPARE(fps.avg, 100.0f);
}

QTEST_MAIN(FpsTest)

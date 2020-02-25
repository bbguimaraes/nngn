#include "thread_pool_test.h"

#include <mutex>

#include "compute/thread_pool.h"

void ThreadPoolTest::start(void) {
    std::mutex m0 = {}, m1 = {};
    m0.lock();
    m1.lock();
    int called0 = 0, called1 = 0;
    nngn::ThreadPool p;
    const auto id0 = p.start([&m0, &called0] {
        std::lock_guard g{m0};
        called0 = 1;
    });
    const auto id1 = p.start([&m1, &called1] {
        std::lock_guard g{m1};
        called1 = 2;
    });
    m1.unlock();
    p.thread(id1).join();
    QCOMPARE(called1, 2);
    m0.unlock();
    p.thread(id0).join();
    QCOMPARE(called0, 1);
}

QTEST_MAIN(ThreadPoolTest)

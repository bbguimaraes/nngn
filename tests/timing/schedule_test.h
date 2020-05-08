#ifndef NNGN_TEST_TIMING_SCHEDULE_H
#define NNGN_TEST_TIMING_SCHEDULE_H

#include <QTest>

class ScheduleTest : public QObject {
    Q_OBJECT
private slots:
    void next();
    void in();
    void at();
    void frame();
    void atexit();
    void heartbeat();
    void cancel();
    void recursive();
    void recursive_remove();
    void destructor();
    void map();
};

#endif

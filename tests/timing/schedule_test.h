#ifndef NNGN_TEST_TIMING_SCHEDULE_H
#define NNGN_TEST_TIMING_SCHEDULE_H

#include <QTest>

class ScheduleTest : public QObject {
    Q_OBJECT
private slots:
    void next(void);
    void in(void);
    void at(void);
    void frame(void);
    void atexit(void);
    void ignore_failures(void);
    void heartbeat(void);
    void cancel(void);
    void recursive(void);
    void recursive_remove(void);
    void destructor(void);
    void map(void);
};

#endif

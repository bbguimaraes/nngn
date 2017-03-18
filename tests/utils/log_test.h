#ifndef NNGN_TEST_LOG_H
#define NNGN_TEST_LOG_H

#include <QTest>

class LogTest : public QObject {
    Q_OBJECT
private slots:
    void capture();
    void context();
    void with_context();
    void replace();
    void max();
};

#endif

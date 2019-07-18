#ifndef NNGN_TEST_COMPUTE_COMPUTE_H
#define NNGN_TEST_COMPUTE_COMPUTE_H

#include <QTest>

class ComputeTest : public QObject {
    Q_OBJECT
private slots:
    void execute_kernel();
    void execute();
    void execute_args();
    void events();
    void write_struct();
};

#endif

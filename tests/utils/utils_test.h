#ifndef NNGN_TEST_UTILS_H
#define NNGN_TEST_UTILS_H

#include <QTest>

class UtilsTest : public QObject {
    Q_OBJECT
private slots:
    void offsetof_ptr();
    void read_file();
    void pointer_flag();
    void read_file_err();
    void set_capacity();
    void const_time_erase();
};

#endif

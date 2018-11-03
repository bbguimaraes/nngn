#ifndef NNGN_TEST_UTILS_H
#define NNGN_TEST_UTILS_H

#include <QTest>

class UtilsTest : public QObject {
    Q_OBJECT
private slots:
    void read_file();
    void pointer_flag();
    void read_file_err();
};

#endif

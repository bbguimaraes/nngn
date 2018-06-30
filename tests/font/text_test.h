#ifndef NNGN_TEST_TEXT_H
#define NNGN_TEST_TEXT_H

#include <QTest>

class TextTest : public QObject {
    Q_OBJECT
private slots:
    void default_constructor();
    void constructor();
    void count_lines_data();
    void count_lines();
    void size_zero();
    void size_data();
    void size();
};

#endif

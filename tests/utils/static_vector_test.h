#ifndef NNGN_TESTS_UTILS_STATIC_VECTOR_H
#define NNGN_TESTS_UTILS_STATIC_VECTOR_H

#include <QTest>

class StaticVectorTest : public QObject {
    Q_OBJECT
private slots:
    void emplace();
    void erase();
    void erase_reverse();
    void erase_emplace();
    void set_capacity();
};

#endif

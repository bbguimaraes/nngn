#ifndef NNGN_TESTS_UTILS_ALLOC_REALLOC_H
#define NNGN_TESTS_UTILS_ALLOC_REALLOC_H

#include <QTest>

class ReallocTest : public QObject {
    Q_OBJECT
private slots:
    void alloc(void);
    void realloc(void);
    void typed_alloc(void);
    void typed_realloc(void);
};

#endif

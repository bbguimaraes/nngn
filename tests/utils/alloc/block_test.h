#ifndef NNGN_TESTS_UTILS_ALLOC_BLOCK_H
#define NNGN_TESTS_UTILS_ALLOC_BLOCK_H

#include <QTest>

class BlockTest : public QObject {
    Q_OBJECT
private slots:
    void bytes(void);
    void type(void);
};

#endif

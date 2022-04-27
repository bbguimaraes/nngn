#ifndef NNGN_TESTS_UTILS_ALLOC_TAGGING_H
#define NNGN_TESTS_UTILS_ALLOC_TAGGING_H

#include <QTest>

class TaggingTest : public QObject {
    Q_OBJECT
private slots:
    void alloc(void);
    void vector(void);
    void list(void);
};

#endif

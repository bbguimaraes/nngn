#ifndef NNGN_TESTS_UTILS_ALLOC_TRACKING_H
#define NNGN_TESTS_UTILS_ALLOC_TRACKING_H

#include <QTest>

class TrackingTest : public QObject {
    Q_OBJECT
private slots:
    void vector(void);
    void list(void);
    void typed(void);
    void nested(void);
    void vector_nested(void);
    void list_nested(void);
    void realloc(void);
    void typed_realloc(void);
};

#endif

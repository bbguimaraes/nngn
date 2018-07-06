#ifndef NNGN_TEST_UTILS_SCOPED_H
#define NNGN_TEST_UTILS_SCOPED_H

#include <QTest>

class ScopedTest : public QObject {
    Q_OBJECT
private slots:
    void delegate_base();
    void constructor();
    void lambda();
    void lambda_with_storage();
    void lambda_obj();
    void delegate();
    void delegate_arg();
    void delegate_obj();
    void obj_member();
    void obj_member_arg();
};

#endif

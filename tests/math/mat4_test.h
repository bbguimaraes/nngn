#ifndef NNGN_TEST_MATH_MATH_H
#define NNGN_TEST_MATH_MATH_H

#include <QTest>

class Mat4Test : public QObject {
    Q_OBJECT
private slots:
    void mat4_mat4_mul();
    void mat4_vec4_mul();
};

#endif

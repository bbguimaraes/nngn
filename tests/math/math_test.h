#ifndef NNGN_TEST_MATH_MATH_H
#define NNGN_TEST_MATH_MATH_H

#include <QTest>

class MathTest : public QObject {
    Q_OBJECT
private slots:
    void avg2_data();
    void avg2();
    void avg3_data();
    void avg3();
    void avg4_data();
    void avg4();
    void swizzle2();
    void swizzle3();
    void swizzle4();
    void clamp_len_vec_data();
    void clamp_len_vec();
    void rotate_data();
    void rotate();
    void rotate_vec_x_data();
    void rotate_vec_x();
    void rotate_vec_y_data();
    void rotate_vec_y();
    void rotate_vec_z_data();
    void rotate_vec_z();
    void rotate_x_data();
    void rotate_x();
    void rotate_y_data();
    void rotate_y();
    void rotate_z_data();
    void rotate_z();
    void mat4_rotate_data();
    void mat4_rotate();
};

#endif

#ifndef NNGN_TEST_MATH_MATH_H
#define NNGN_TEST_MATH_MATH_H

#include <QTest>

class MathTest : public QObject {
    Q_OBJECT
private slots:
    void round_down_pow2_data(void);
    void round_down_pow2(void);
    void round_up_pow2_data(void);
    void round_up_pow2(void);
    void round_down_data(void);
    void round_down(void);
    void round_up_data(void);
    void round_up(void);
    void avg2_data(void);
    void avg2(void);
    void avg3_data(void);
    void avg3(void);
    void avg4_data(void);
    void avg4(void);
    void swizzle2(void);
    void swizzle3(void);
    void swizzle4(void);
    void clamp_len_vec_data(void);
    void clamp_len_vec(void);
    void rotate_data(void);
    void rotate(void);
    void rotate_vec_x_data(void);
    void rotate_vec_x(void);
    void rotate_vec_y_data(void);
    void rotate_vec_y(void);
    void rotate_vec_z_data(void);
    void rotate_vec_z(void);
    void rotate_x_data(void);
    void rotate_x(void);
    void rotate_y_data(void);
    void rotate_y(void);
    void rotate_z_data(void);
    void rotate_z(void);
    void ortho(void);
    void perspective(void);
};

#endif

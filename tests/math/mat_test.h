#ifndef NNGN_TEST_MATH_MATH_H
#define NNGN_TEST_MATH_MATH_H

#include <QTest>

class MatTest : public QObject {
    Q_OBJECT
private slots:
    void mat3_row(void);
    void mat3_col(void);
    void mat4_row(void);
    void mat4_col(void);
    void diag(void);
    void minor_matrix_data(void);
    void minor_matrix(void);
    void mat3_vec_mul(void);
    void mat4_vec_mul(void);
    void mat3_mul(void);
    void mat4_mul(void);
    void rotate_data(void);
    void rotate(void);
    void determinant3_data(void);
    void determinant3(void);
    void determinant4_data(void);
    void determinant4(void);
    void inverse_data(void);
    void inverse(void);
};

#endif

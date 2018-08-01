#ifndef NNGN_TEST_RENDER_H
#define NNGN_TEST_RENDER_H

#include <QTest>

class RenderTest : public QObject {
    Q_OBJECT
private slots:
    void uv_coords_data();
    void uv_coords();
};

#endif

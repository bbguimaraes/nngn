#ifndef NNGN_TEST_CAMERA_H
#define NNGN_TEST_CAMERA_H

#include <QTest>

class CameraTest : public QObject {
    Q_OBJECT
private slots:
    void look_at_data();
    void look_at();
    void view();
    void view_perspective();
    void update();
    void update_max_vel();
    void damp();
    void proj();
    void proj_perspective();
    void proj_hud();
    void perspective();
    void perspective_zoom();
    void eye();
};

#endif

#ifndef NNGN_TEST_CAMERA_H
#define NNGN_TEST_CAMERA_H

#include <QTest>

class CameraTest : public QObject {
    Q_OBJECT
private slots:
    void fov_data(void);
    void fov(void);
    void center_up_data(void);
    void center_up(void);
    void world_to_view(void);
    void view_to_clip(void);
    void clip_to_screen(void);
    void screen_clip_view_world_data(void);
    void screen_clip_view_world(void);
    void z_for_fov(void);
    void scale_for_fov(void);
    void fov_z(void);
    void look_at_data(void);
    void look_at(void);
    void proj_perspective(void);
    void view_data(void);
    void view(void);
    void view_perspective_data(void);
    void view_perspective(void);
    void ortho_to_persp(void);
    void update(void);
    void update_max_vel(void);
    void damp(void);
    void proj(void);
};

#endif

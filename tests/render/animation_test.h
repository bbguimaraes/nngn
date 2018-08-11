#include <QTest>

#include "lua/state.h"

#include "render/animation.h"

class AnimationTest : public QObject {
    Q_OBJECT
    nngn::lua::state lua = {};
    nngn::SpriteAnimation sprite = {};
private slots:
    void initTestCase();
    void init();
    void load_data();
    void load();
    void update();
};

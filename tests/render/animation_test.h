#include <QTest>

#include "luastate.h"

#include "render/animation.h"

class AnimationTest : public QObject {
    Q_OBJECT
    LuaState lua = {};
    nngn::SpriteAnimation sprite = {};
private slots:
    void initTestCase();
    void init();
    void load_data();
    void load();
    void update();
};

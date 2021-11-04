#ifndef NNGN_TOOLS_AUDIO_EDIT_H
#define NNGN_TOOLS_AUDIO_EDIT_H

#include <string>

#include <QMainWindow>

#include "utils/utils.h"

#include "gen.h"

class QDial;
class QPlainTextEdit;
class QSpinBox;
class QTabBar;

namespace nngn {

class Edit final : public QMainWindow {
    Q_OBJECT
public:
    NNGN_NO_COPY(Edit)
    Edit(void);
signals:
    void updated(std::vector<std::byte> v) const;
private:
    bool init_generator(void);
    bool exec(void);
    bool generate(void);
    QSpinBox *rate = nullptr;
    QPlainTextEdit *editor = nullptr, *error = nullptr;
    QTabBar *tab_bar = nullptr;
    QMetaObject::Connection editor_con = {}, param_con = {};
    Generator gen = {};
    QList<QString> presets = {};
};

}

#endif

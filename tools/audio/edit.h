#ifndef NNGN_TOOLS_AUDIO_EDIT_H
#define NNGN_TOOLS_AUDIO_EDIT_H

#include <string>
#include <vector>

#include <QMainWindow>

#include "utils/utils.h"

#include "gen.h"

class QDial;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTabBar;

namespace nngn {

class Edit : public QMainWindow {
    Q_OBJECT
public:
    NNGN_MOVE_ONLY(Edit)
    Edit(void);
signals:
    void updated(std::vector<std::byte> v) const;
private:
    bool init_generator(void);
    bool exec(void);
    bool generate(void);
    QSpinBox *rate = nullptr;
    QPlainTextEdit *editor = nullptr, *error = nullptr;
    QDial *param = nullptr;
    QPushButton *loop = nullptr;
    QTabBar *tab_bar = nullptr;
    Generator gen = {};
    std::vector<std::string> presets = {};
};

}

#endif

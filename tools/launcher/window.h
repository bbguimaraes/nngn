#ifndef NNGN_TOOLS_LAUNCHER_WINDOW_H
#define NNGN_TOOLS_LAUNCHER_WINDOW_H

#include <QWidget>

#include "utils/utils.h"

class QLayout;
class QPushButton;

class Window : public QWidget {
    QLayout *layout = {};
public:
    NNGN_NO_MOVE(Window)
    Window();
    std::size_t add_section(const char *name);
    const QPushButton *add_button(std::size_t section, const char *name);
};

#endif

#ifndef NNGN_TOOLS_CONFIGURE_WINDOW_H
#define NNGN_TOOLS_CONFIGURE_WINDOW_H

#include <QWidget>

#include "utils/utils.h"

class FloatWidget;
class IntWidget;

class Window : public QWidget {
    Q_OBJECT
public:
    NNGN_NO_MOVE(Window)
    Window();
signals:
    void command_updated(const QString &value, const QString &cmd);
};

#endif

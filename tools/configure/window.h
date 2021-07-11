#ifndef NNGN_TOOLS_CONFIGURE_WINDOW_H
#define NNGN_TOOLS_CONFIGURE_WINDOW_H

#include <QSplitter>

#include "utils/utils.h"

class FloatWidget;
class IntWidget;

class Window : public QSplitter {
    Q_OBJECT
public:
    NNGN_NO_MOVE(Window)
    Window();
    bool add_widget(std::string_view text = {});
signals:
    void command_updated(QString value, QString cmd);
};

#endif

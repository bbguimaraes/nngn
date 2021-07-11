#ifndef NNGN_TOOLS_CONFIGURE_INT_WIDGET_H
#define NNGN_TOOLS_CONFIGURE_INT_WIDGET_H

#include <QWidget>

class IntWidget : public QWidget {
    Q_OBJECT
public:
    IntWidget();
signals:
    void value_changed(int value);
};

#endif

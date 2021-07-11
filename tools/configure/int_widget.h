#ifndef NNGN_TOOLS_CONFIGURE_INT_WIDGET_H
#define NNGN_TOOLS_CONFIGURE_INT_WIDGET_H

#include <QWidget>

class IntWidget : public QWidget {
    Q_OBJECT
public:
    IntWidget() : IntWidget{0, 100, 0} {}
    IntWidget(int min, int max, int value);
signals:
    void value_changed(int value);
};

#endif

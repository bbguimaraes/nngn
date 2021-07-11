#ifndef NNGN_TOOLS_CONFIGURE_FLOAT_WIDGET_H
#define NNGN_TOOLS_CONFIGURE_FLOAT_WIDGET_H

#include <QWidget>

class FloatWidget : public QWidget {
    Q_OBJECT
public:
    FloatWidget() : FloatWidget{0, 100, 0, 100} {}
    FloatWidget(int min, int max, int value, int div);
signals:
    void value_changed(float value);
};

#endif

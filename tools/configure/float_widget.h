#ifndef NNGN_TOOLS_CONFIGURE_FLOAT_WIDGET_H
#define NNGN_TOOLS_CONFIGURE_FLOAT_WIDGET_H

#include <QWidget>

class FloatWidget : public QWidget {
    Q_OBJECT
public:
    FloatWidget();
signals:
    void value_changed(float value);
};

#endif

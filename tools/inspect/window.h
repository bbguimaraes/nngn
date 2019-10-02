#ifndef NNGN_WINDOW_H
#define NNGN_WINDOW_H

#include <QtWidgets/QMainWindow>

class QLabel;

class Window : public QMainWindow {
    Q_OBJECT
    QLabel *label();
public:
    Window();
public slots:
    void clear();
    void new_line(QString s);
};

#endif

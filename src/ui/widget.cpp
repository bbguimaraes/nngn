#include "widget.hpp"

#include <QtGui/QKeyEvent>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>

namespace impero {

Widget::Widget(QWidget *p) : QWidget(p) {
    new QVBoxLayout(this);
    this->font.setFamily("monospace");
    this->font.setStyleHint(QFont::Monospace);
    this->font.setFixedPitch(true);
}

void Widget::keyPressEvent(QKeyEvent *e) {
    switch(e->key()) {
    case Qt::Key_Tab: return emit this->selection_moved(true);
    case Qt::Key_Backtab: return emit this->selection_moved(false);
    default: return QWidget::keyPressEvent(e);
    }
}

void Widget::add_edit(QLineEdit *e) { this->layout()->addWidget(e); }

void Widget::add_panel(QWidget *w) {
    this->layout()->addWidget(w);
    w->setFont(this->font);
}

}

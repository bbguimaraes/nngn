#include <QtWidgets/QLabel>
#include "window.h"

Window::Window() {
    this->setCentralWidget(new QLabel);
    this->label()->setAlignment(Qt::AlignTop);
}

QLabel *Window::label()
    { return static_cast<QLabel*>(this->centralWidget()); }

void Window::clear() {
    this->label()->setText("");
}

void Window::new_line(QString s) {
    auto l = this->label();
    const auto t = l->text();
    l->setText(t.isEmpty() ? s : t + "\n" + s);
}

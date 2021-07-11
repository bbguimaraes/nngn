#include "window.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QVBoxLayout>

#include "float_widget.h"
#include "int_widget.h"

Window::Window() {
    auto *const layout = new QVBoxLayout(this);
    auto *const tab = new QTabWidget;
    auto *const text_widget = new QLineEdit;
    auto *const int_widget = new IntWidget;
    auto *const float_widget = new FloatWidget;
    auto *const cmd_edit = new QPlainTextEdit;
    tab->addTab(text_widget, "text");
    tab->addTab(int_widget, "int");
    tab->addTab(float_widget, "float");
    layout->addWidget(tab);
    layout->addWidget(cmd_edit);
    QObject::connect(
        text_widget, &QLineEdit::returnPressed, [this, text_widget, cmd_edit] {
            emit this->command_updated(
                text_widget->text(), cmd_edit->toPlainText());
        });
    QObject::connect(
        int_widget, &IntWidget::value_changed, [this, cmd_edit](int v) {
            emit this->command_updated(
                QString::number(v), cmd_edit->toPlainText());
        });
    QObject::connect(
        float_widget, &FloatWidget::value_changed,
        [this, cmd_edit](float v) {
            emit this->command_updated(
                QString::number(v), cmd_edit->toPlainText());
        });
}

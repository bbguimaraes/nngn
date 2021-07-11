#include "window.h"

#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

#include "float_widget.h"
#include "int_widget.h"

using namespace std::string_view_literals;

namespace {

int min_height(QPlainTextEdit *e) {
    return 2 * QFontMetrics(e->document()->defaultFont()).height();
}

bool read_title(std::string_view *s, std::string_view *title) {
    const auto i = s->find(':');
    if(i == std::string_view::npos)
        return false;
    *title = s->substr(0, i);
    s->remove_prefix(i + 1);
    return true;
}

bool read_int_value(std::string_view *s, int *p) {
    const auto ret = std::from_chars(begin(*s), end(*s), *p);
    if(ret.ec != std::errc{} || ret.ptr == &s->back() || *ret.ptr != ':')
        return false;
    s->remove_prefix(static_cast<std::size_t>(ret.ptr + 1 - s->data()));
    return true;
}

bool read_bool_values(
    std::string_view *text, int *value, std::string_view *title
) {
    return read_int_value(text, value)
        && read_title(text, title);
}

bool read_int_values(
    std::string_view *text,
    int *min, int *max, int *value, std::string_view *title
) {
    return read_int_value(text, min)
        && read_int_value(text, max)
        && read_int_value(text, value)
        && read_title(text, title);
}

bool read_float_values(
    std::string_view *text,
    int *min, int *max, int *value, int *div, std::string_view *title
) {
    return read_int_value(text, min)
        && read_int_value(text, max)
        && read_int_value(text, value)
        && read_int_value(text, div)
        && read_title(text, title);
}

void connect_text(Window *w, QLineEdit *e, QPlainTextEdit *t) {
    QObject::connect(e, &QLineEdit::returnPressed, [w, e, t] {
        emit w->command_updated(e->text(), t->toPlainText());
    });
}

void connect_int(Window *w, IntWidget *i, QPlainTextEdit *t) {
    QObject::connect(i, &IntWidget::value_changed, [w, t](int v) {
        emit w->command_updated(QString::number(v), t->toPlainText());
    });
}

void connect_float(Window *w, FloatWidget *i, QPlainTextEdit *t) {
    QObject::connect(i, &FloatWidget::value_changed, [w, t](float v) {
        emit w->command_updated(
            QString::number(static_cast<double>(v)),
            t->toPlainText());
    });
}

void connect_bool(Window *w, QCheckBox *i, QPlainTextEdit *t) {
    QObject::connect(i, &QCheckBox::stateChanged, [w, t](int v) {
        emit w->command_updated(
            v == Qt::Checked ? "true" : "false", t->toPlainText());
    });
}

QPlainTextEdit *add_common(
    Window *w, QLayoutItem *i, std::string_view title, std::string_view text
) {
    auto *const parent = new QGroupBox{
        QString::fromUtf8(title.data(), static_cast<int>(title.size()))};
    auto *const top_layout = new QHBoxLayout;
    auto *const btn = new QPushButton{"▲"};
    auto *const cmd_edit = new QPlainTextEdit(
        QString::fromUtf8(text.data(), static_cast<int>(text.size())));
    parent->setAlignment(Qt::AlignLeft);
    btn->setMaximumSize(28, 28);
    btn->setCheckable(true);
    cmd_edit->setMinimumHeight(min_height(cmd_edit));
    cmd_edit->setVisible(false);
    top_layout->addItem(i);
    top_layout->addWidget(btn);
    auto *const layout = new QVBoxLayout(parent);
    layout->addLayout(top_layout);
    layout->addWidget(cmd_edit);
    w->addWidget(parent);
    QObject::connect(
        btn, &QPushButton::toggled,
        [btn](auto b) { btn->setText(std::array{"▲", "▼"}[b]); });
    QObject::connect(
        btn, &QPushButton::toggled,
        cmd_edit, &QPlainTextEdit::setVisible);
    return cmd_edit;
}

bool add_generic_widget(Window *w, std::string_view text) {
    std::string_view title = {};
    if(!read_title(&text, &title))
        return false;
    auto *const layout = new QVBoxLayout;
    auto *const tab = new QTabWidget;
    auto *const text_widget = new QLineEdit;
    auto *const int_widget = new IntWidget;
    auto *const float_widget = new FloatWidget;
    tab->addTab(text_widget, "text");
    tab->addTab(int_widget, "int");
    tab->addTab(float_widget, "float");
    layout->addWidget(tab);
    auto *const cmd_edit = add_common(w, layout, title, text);
    connect_text(w, text_widget, cmd_edit);
    connect_int(w, int_widget, cmd_edit);
    connect_float(w, float_widget, cmd_edit);
    return true;
}

bool add_text_widget(Window *w, std::string_view text) {
    auto *const tw = new QLineEdit;
    std::string_view title = {};
    if(!read_title(&text, &title))
        return false;
    connect_text(w, tw, add_common(w, new QWidgetItem{tw}, title, text));
    return true;
}

bool add_int_widget(Window *w, std::string_view text) {
    int min = 0, max = 0, value = 0;
    std::string_view title = {};
    if(!read_int_values(&text, &min, &max, &value, &title))
        return false;
    auto *const iw = new IntWidget{min, max, value};
    connect_int(w, iw, add_common(w, new QWidgetItem{iw}, title, text));
    return true;
}

bool add_float_widget(Window *w, std::string_view text) {
    int min = 0, max = 0, value = 0, div = 0;
    std::string_view title = {};
    if(!read_float_values(&text, &min, &max, &value, &div, &title))
        return false;
    auto *const fw = new FloatWidget(min, max, value, div);
    connect_float(w, fw, add_common(w, new QWidgetItem{fw}, title, text));
    return true;
}

bool add_bool_widget(Window *w, std::string_view text) {
    int value = false;
    std::string_view title = {};
    if(!read_bool_values(&text, &value, &title))
        return false;
    auto *const bw = new QCheckBox;
    bw->setChecked(value);
    connect_bool(w, bw, add_common(w, new QWidgetItem{bw}, title, text));
    return true;
}

}

Window::Window() {
    this->setOrientation(Qt::Vertical);
}

bool Window::add_widget(std::string_view text) {
    constexpr auto text_p = "t:"sv;
    constexpr auto int_p = "i:"sv;
    constexpr auto float_p = "f:"sv;
    constexpr auto bool_p = "b:"sv;
    constexpr auto generic_p = "g:"sv;
    if(text.empty())
        return add_generic_widget(this, {}), true;
    else if(text.starts_with(text_p))
        return add_text_widget(this, text.substr(text_p.size()));
    else if(text.starts_with(int_p)) {
        if(add_int_widget(this, text.substr(int_p.size())))
            return true;
    } else if(text.starts_with(float_p)) {
        if(add_float_widget(this, text.substr(float_p.size())))
            return true;
    } else if(text.starts_with(bool_p))
        return add_bool_widget(this, text.substr(bool_p.size()));
    else if(text.starts_with(generic_p))
        return add_generic_widget(this, text.substr(generic_p.size()));
    std::cerr
        << "Window::add_widget: invalid prefix: "
        << std::quoted(text) << '\n';
    return false;
}

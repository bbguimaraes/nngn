#include "panel.hpp"

#include <string_view>

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

#include "utils.hpp"

namespace {

auto widget(QLayout *l, auto i) {
    const auto ii = static_cast<int>(i);
    assert(ii < l->count());
    auto *const w = l->itemAt(ii)->widget();
    assert(w);
    return w;
}

std::size_t update_visible(QLayout *l, const QString &filter) {
    auto v = impero::layout_widget_view(l) | impero::as<QLabel*>;
    if(filter.isEmpty()) {
        for(auto *l : v)
            l->setVisible(true);
        return static_cast<std::size_t>(l->count());
    }
    return impero::accumulate(
        v | std::views::transform([&filter](auto *l) {
            const bool b = l->text().contains(filter);
            l->setVisible(b);
            return static_cast<std::size_t>(b);
        }));
}

template<bool S>
void set_selected(QWidget *l) {
    l->setBackgroundRole(S ? QPalette::Base : QPalette::Window);
}

template<bool S>
void set_selected(QLayout *l, std::size_t i) {
    set_selected<S>(static_cast<QLabel*>(widget(l, i)));
}

template<bool forward>
std::size_t move_selection(QLayout *l, std::size_t i) {
    set_selected<false>(l, i);
    const auto n = static_cast<std::size_t>(l->count());
    const auto inc = forward ? 1 : n - 1;
    const auto cur = i;
    QWidget *w = {};
    do {
        i = (i + inc) % n;
        w = widget(l, i);
        assert(w);
    } while(i != cur && !w->isVisible());
    if(i != cur || w->isVisible())
        set_selected<true>(static_cast<QLabel*>(w));
    return i;
}

}

namespace impero {

Panel::Panel(QWidget *p) : QWidget(p) { new QGridLayout(this); }

void Panel::add_command(std::string_view s) {
    auto *const l = new QLabel(QString::fromUtf8(s.data(), s.size()));
    this->layout()->addWidget(l);
    l->setContentsMargins(4, 4, 4, 4);
    l->setAutoFillBackground(true);
    if(!this->n_visible++)
        set_selected<true>(l);
}

void Panel::update_filter(const QString &filter) {
    auto *const l = this->layout();
    const auto prev = this->n_visible;
    if(!(this->n_visible = update_visible(l, filter)))
        set_selected<false>(l, this->cur);
    else if(!widget(l, this->cur)->isVisible())
        this->move_selection(true);
    else if(!prev)
        set_selected<true>(l, this->cur);
    this->adjustSize();
}

void Panel::move_selection(bool forward) {
    if(!this->n_visible)
        return;
    this->cur = forward
        ? ::move_selection<true>(this->layout(), this->cur)
        : ::move_selection<false>(this->layout(), this->cur);
}

void Panel::select_command(void) const {
    if(!this->n_visible)
        return;
    assert(this->cur < static_cast<std::size_t>(this->layout()->count()));
    emit this->command_selected(this->cur);
}

}

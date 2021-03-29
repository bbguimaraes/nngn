#include "window.h"

#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

Window::Window() {
    this->layout = new QVBoxLayout(this);
}

std::size_t Window::add_section(const char *name) {
    const auto ret = static_cast<std::size_t>(this->layout->count());
    auto *const group = new QGroupBox(name);
    new QVBoxLayout(group);
    this->layout->addWidget(group);
    return ret;
}

const QPushButton *Window::add_button(std::size_t section, const char *name) {
    auto *const i = this->layout->itemAt(static_cast<int>(section));
    assert(i);
    auto *const w = i->widget();
    assert(w);
    auto *const ret = new QPushButton(name);
    w->layout()->addWidget(ret);
    return ret;
}

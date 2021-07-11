#include "int_widget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>

IntWidget::IntWidget(int min, int max, int value) {
    auto *const layout = new QHBoxLayout(this);
    auto *const min_widget = new QSpinBox;
    auto *const max_widget = new QSpinBox;
    auto *const slider = new QSlider(Qt::Horizontal);
    auto *const display = new QLabel;
    min_widget->setMinimum(INT_MIN);
    min_widget->setMaximum(INT_MAX);
    max_widget->setMinimum(INT_MIN);
    max_widget->setMaximum(INT_MAX);
    min_widget->setMinimumWidth(8);
    max_widget->setMinimumWidth(8);
    display->setAlignment(Qt::AlignRight);
    layout->addWidget(min_widget, 2);
    layout->addWidget(slider, 8);
    layout->addWidget(max_widget, 2);
    layout->addWidget(display, 1);
    QObject::connect(
        min_widget, QOverload<int>::of(&QSpinBox::valueChanged),
        slider, &QSlider::setMinimum);
    QObject::connect(
        max_widget, QOverload<int>::of(&QSpinBox::valueChanged),
        slider, &QSlider::setMaximum);
    QObject::connect(
        slider, &QSlider::valueChanged,
        display, QOverload<int>::of(&QLabel::setNum));
    QObject::connect(
        slider, &QSlider::valueChanged, this, &IntWidget::value_changed);
    min_widget->setValue(min);
    max_widget->setValue(max);
    slider->setValue(value);
    display->setNum(value);
}

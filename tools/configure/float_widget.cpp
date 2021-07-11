#include "float_widget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>

FloatWidget::FloatWidget(int min, int max, int value, int div) {
    auto *const layout = new QHBoxLayout(this);
    auto *const min_widget = new QSpinBox;
    auto *const max_widget = new QSpinBox;
    auto *const slider = new QSlider(Qt::Horizontal);
    auto *const div_widget = new QSpinBox;
    auto *const display = new QLabel;
    min_widget->setMinimum(INT_MIN);
    min_widget->setMaximum(INT_MAX);
    max_widget->setMinimum(INT_MIN);
    max_widget->setMaximum(INT_MAX);
    div_widget->setMinimum(INT_MIN);
    div_widget->setMaximum(INT_MAX);
    min_widget->setMinimumWidth(8);
    max_widget->setMinimumWidth(8);
    div_widget->setMinimumWidth(8);
    display->setAlignment(Qt::AlignRight);
    layout->addWidget(min_widget, 2);
    layout->addWidget(slider, 8);
    layout->addWidget(max_widget, 2);
    layout->addWidget(new QLabel("/"));
    layout->addWidget(div_widget, 2);
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
        slider, &QSlider::valueChanged, [this, div_widget](int v) {
            const auto fv = static_cast<float>(v);
            const auto d = static_cast<float>(div_widget->value());
            emit this->value_changed(fv / d);
        });
    min_widget->setValue(min);
    max_widget->setValue(max);
    div_widget->setValue(div);
    slider->setValue(value);
    display->setNum(value);
}

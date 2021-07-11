#include "int_widget.h"

#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>

IntWidget::IntWidget() {
    auto *const layout = new QHBoxLayout(this);
    auto *const min = new QSpinBox;
    auto *const max = new QSpinBox;
    auto *const slider = new QSlider(Qt::Horizontal);
    min->setMinimum(INT_MIN);
    min->setMaximum(INT_MAX);
    max->setMinimum(INT_MIN);
    max->setMaximum(INT_MAX);
    min->setValue(0);
    max->setValue(100);
    min->setMinimumWidth(8);
    max->setMinimumWidth(8);
    layout->addWidget(min, 1);
    layout->addWidget(slider, 4);
    layout->addWidget(max, 1);
    QObject::connect(
        min, QOverload<int>::of(&QSpinBox::valueChanged),
        slider, &QSlider::setMinimum);
    QObject::connect(
        max, QOverload<int>::of(&QSpinBox::valueChanged),
        slider, &QSlider::setMaximum);
    QObject::connect(
        slider, &QSlider::valueChanged, this, &IntWidget::value_changed);
}

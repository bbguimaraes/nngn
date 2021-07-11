#include "float_widget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>

FloatWidget::FloatWidget() {
    auto *const layout = new QHBoxLayout(this);
    auto *const min = new QSpinBox;
    auto *const max = new QSpinBox;
    auto *const slider = new QSlider(Qt::Horizontal);
    auto *const div = new QSpinBox;
    min->setMinimum(INT_MIN);
    min->setMaximum(INT_MAX);
    max->setMinimum(INT_MIN);
    max->setMaximum(INT_MAX);
    div->setMinimum(INT_MIN);
    div->setMaximum(INT_MAX);
    min->setValue(0);
    max->setValue(1);
    div->setValue(100);
    min->setMinimumWidth(8);
    max->setMinimumWidth(8);
    div->setMinimumWidth(8);
    layout->addWidget(min, 1);
    layout->addWidget(slider, 4);
    layout->addWidget(max, 1);
    layout->addWidget(new QLabel("/"));
    layout->addWidget(div, 1);
    QObject::connect(
        min, QOverload<int>::of(&QSpinBox::valueChanged),
        slider, &QSlider::setMinimum);
    QObject::connect(
        max, QOverload<int>::of(&QSpinBox::valueChanged),
        slider, &QSlider::setMaximum);
    QObject::connect(
        slider, &QSlider::valueChanged, [this, div](int value) {
            const auto v = static_cast<float>(value);
            const auto d = static_cast<float>(div->value());
            emit this->value_changed(v / d);
        });
}

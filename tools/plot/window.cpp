#include "window.h"

#include <cfloat>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMenuBar>

namespace {

void append_point(QtCharts::QLineSeries *series, int max, qreal x, qreal y) {
    if(series->count() > max)
        series->remove(0);
    series->append(x, y);
}

}

Window::Window() {
    this->setCentralWidget(new QWidget);
    this->centralWidget()->setLayout(new QGridLayout);
}

void Window::new_graph(Graph::Type type, QString title) {
    auto *const view = new QtCharts::QChartView(new QtCharts::QChart);
    auto *const chart = view->chart();
    chart->setTheme(QtCharts::QChart::ChartThemeDark);
    chart->legend()->hide();
    switch(type) {
    case Graph::Type::LINE: {
        auto *const s = new QtCharts::QLineSeries;
        this->series.push_back(s);
        chart->addSeries(s);
        chart->createDefaultAxes();
        const auto axes = chart->axes();
        axes[0]->hide();
        axes[1]->setMin(0);
        break;
    }
    default: assert(false);
    }
    chart->setTitle(title);
    view->setMinimumHeight(200);
    auto *const layout =
        static_cast<QGridLayout*>(this->centralWidget()->layout());
    const auto j = this->series.size() - 1;
    const auto row = j / this->cols, col = j % this->cols;
    layout->addWidget(view, static_cast<int>(row), static_cast<int>(col));
    layout->setRowStretch(static_cast<int>(row), 1);
    layout->setColumnStretch(static_cast<int>(col), 1);
}

void Window::new_frame(qreal t) {
    this->timestamp_ms = t;
    this->i = 0;
}

void Window::new_data(Graph::Type type, qreal value) {
    assert(this->i < this->series.size());
    auto *const s = this->series[this->i];
    switch(type) {
    case Graph::Type::LINE: {
        auto *const ls = static_cast<QtCharts::QLineSeries*>(s);
        append_point(ls, 120, this->timestamp_ms, value);
        qreal min = DBL_MAX, max = DBL_MIN;
        for(int j = 0, jn = ls->count(); j < jn; ++j) {
            const auto v = ls->at(j).y();
            min = std::min(min, v);
            max = std::max(max, v);
        }
        if(!(min && max))
            max = 1;
        else if(min == max)
            --min, ++max;
        const auto axes = ls->chart()->axes();
        axes[0]->setRange(ls->at(0).x(), this->timestamp_ms);
        axes[1]->setMin(min);
        axes[1]->setMax(max);
        break;
    }
    default: assert(false);
    }
    ++this->i;
}

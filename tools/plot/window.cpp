#include "window.h"

#include <cfloat>
#include <cmath>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMenuBar>

namespace {

void append_point(QtCharts::QLineSeries *series, int max, qreal x, qreal y) {
    if(series->count() > max)
        series->remove(0);
    series->append(x, y);
}

auto series_min_max(const QtCharts::QLineSeries &s) {
    qreal min = DBL_MAX, max = DBL_MIN;
    for(int i = 0, n = s.count(); i < n; ++i) {
        const auto x = s.at(i).y();
        min = std::min(min, x);
        max = std::max(max, x);
    }
    return std::pair{min, max};
}

auto separate_equals(qreal min, qreal max) {
    return std::pair{
        std::min(min - 1, std::nextafter(min, -INFINITY)),
        std::max(max + 1, std::nextafter(max, +INFINITY)),
    };
}

void adjust_range(QtCharts::QLineSeries *s, qreal x_max) {
    const auto [s_min, s_max] = series_min_max(*s);
    const auto axes = s->chart()->axes();
    axes[0]->setRange(s->at(0).x(), x_max);
    auto *const axis1 = static_cast<QtCharts::QValueAxis*>(axes[1]);
    auto min = axis1->min(), max = axis1->max();
    if(s_min == s_max)
        std::tie(min, max) = separate_equals(s_min, s_max);
    else if(s_min < min || max < s_max) {
        while(s_min < min)
            min -= max - min;
        while(max < s_max)
            max += max - min;
    } else {
        while(min + (max - min) / 2 < s_min)
            min += (max - min) / 2;
        while(s_max < min + (max - min) / 2)
            max -= (max - min) / 2;
        if(min == max)
            std::tie(min, max) = separate_equals(s_min, s_max);
    }
    min = std::max(min, DBL_MIN);
    max = std::min(max, DBL_MAX);
    if(min != axis1->min())
        axis1->setMin(min);
    if(max != axis1->max())
        axis1->setMax(max);
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
    chart->setMargins({8, 8, 16, 8});
    switch(type) {
    case Graph::Type::LINE: {
        auto *const s = new QtCharts::QLineSeries;
        this->series.push_back(s);
        chart->addSeries(s);
        chart->createDefaultAxes();
        const auto axes = chart->axes();
        axes[0]->hide();
        auto *const axis1 = static_cast<QtCharts::QValueAxis*>(axes[1]);
        axis1->setMin(0);
        break;
    }
    default: assert(false);
    }
    chart->setTitle(title);
    view->setMinimumHeight(120);
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
        adjust_range(ls, this->timestamp_ms);
        break;
    }
    default: assert(!"invalid graph type");
    }
    ++this->i;
}

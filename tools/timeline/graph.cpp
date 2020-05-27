#include "graph.h"

#include <cmath>
#include <iostream>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMenuBar>

Graph::Graph() {
    auto *const c = this->chart();
    c->addAxis(new QtCharts::QValueAxis, Qt::AlignBottom);
    c->addAxis(new QtCharts::QBarCategoryAxis, Qt::AlignLeft);
    c->legend()->hide();
    c->setTheme(QtCharts::QChart::ChartThemeDark);
    c->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
}

void Graph::new_graph(QString title) {
    this->chart()->addSeries(this->series.emplace_back(new QLineSeries));
    auto axes = this->chart()->axes();
    auto *axis = static_cast<QtCharts::QBarCategoryAxis*>(axes[1]);
    axis->append(title);
    axis->setRange(axis->categories()[0], title);
    auto &s = this->series.back();
    s->setName(title);
    s->setPointsVisible();
    s->attachAxis(axes[0]);
    s->attachAxis(axes[1]);
    s->append(0, 0);
    s->append(0, 0);
    s->append(0, 0);
    s->append(0, 0);
    this->setMinimumHeight(
        QFontMetrics(this->font()).height()
        * 2 * static_cast<int>(this->series.size()));
}

void Graph::new_data(const QVector<qreal> &d) {
    if(d.size() % 4) {
        std::cerr << "ignoring data with size % 4 != 0\n";
        return;
    }
    const auto n = d.size() / 4;
    assert(n == static_cast<int>(this->series.size()));
    const auto [min, max] = std::minmax_element(d.cbegin(), d.cend());
    const auto axes = this->series[0]->chart()->axes();
    const qreal sub = this->m_mode == Mode::RELATIVE ? *min : 0;
    for(int i = 0; i < n; ++i) {
        const auto s = this->series[static_cast<size_t>(i)];
        s->replace(0, d[4 * i + 0] - sub, i);
        s->replace(1, d[4 * i + 1] - sub, i);
        s->replace(2, d[4 * i + 2] - sub, i);
        s->replace(3, d[4 * i + 3] - sub, i);
    }
    const auto pad = (*max - *min) / 32.0;
    axes[0]->setRange(*min - sub - pad, *max - sub + pad);
}

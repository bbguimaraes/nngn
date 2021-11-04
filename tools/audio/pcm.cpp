#include "pcm.h"

#include <cfloat>
#include <cmath>

#include <QGLWidget>
#include <QtCharts/QLineSeries>

#include "audio/wav.h"
#include "utils/utils.h"

using nngn::i16;

namespace {

bool is_drag_event(QMouseEvent *e) {
    return (e->buttons() & Qt::LeftButton)
        && (e->modifiers() & Qt::ControlModifier);
}

QtCharts::QLineSeries *create_series(QtCharts::QChart *chart) {
    auto *const ret = new QtCharts::QLineSeries;
    chart->addSeries(ret);
    chart->createDefaultAxes();
    ret->setUseOpenGL(true);
    auto *const pos = new QtCharts::QLineSeries;
    QPen pen = {};
    pen.setColor(QColorConstants::Red);
    pen.setWidth(3);
    pos->setPen(pen);
    pos->append(0, -1);
    pos->append(0, 1);
    chart->addSeries(pos);
    auto axes = chart->axes();
    pos->attachAxis(axes[0]);
    pos->attachAxis(axes[1]);
    return ret;
}

QtCharts::QLineSeries *get_series(QtCharts::QChart *chart, int i) {
    if(auto *const s = chart->series().value(i, nullptr))
        return static_cast<QtCharts::QLineSeries*>(s);
    return i ? nullptr : create_series(chart);
}

QVector<QPointF> pcm_data(qreal rate, std::span<const i16> s) {
    constexpr auto max = static_cast<qreal>(INT16_MAX);
    const auto n = static_cast<int>(s.size());
    auto ret = QVector<QPointF>(n);
    for(int i = 0; i != n; ++i)
        ret[i] = QPointF{
            static_cast<qreal>(i) / rate,
            s[static_cast<std::size_t>(i)] / max,
        };
    return ret;
}

void replace_series(QtCharts::QChart *chart, nngn::WAV wav) {
    assert(wav.channels() == 1);
    assert(wav.bits_per_sample() == 16);
    const auto rate = wav.rate();
    auto *const series = get_series(chart, 0);
    const auto v = nngn::byte_cast<const i16>(wav.data());
    series->replace(pcm_data(rate, v));
    const auto axes = chart->axes();
    axes[0]->setRange(0, static_cast<qreal>(v.size()) / rate);
    axes[1]->setRange(-1, 1);
}

}

namespace nngn {

PCMWidget::PCMWidget(void) {
    this->setRubberBand(QChartView::HorizontalRubberBand);
    auto *const c = this->chart();
    c->legend()->hide();
    c->setTheme(QtCharts::QChart::ChartThemeDark);
}

void PCMWidget::mousePressEvent(QMouseEvent *e) {
    const auto b = e->buttons();
    if(b & Qt::MiddleButton)
        this->reset_zoom();
    else if(is_drag_event(e)) {
        this->pressed = true;
        this->last_pos = e->pos();
        this->setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }
    return QChartView::mousePressEvent(e);
}

void PCMWidget::mouseReleaseEvent(QMouseEvent *e) {
    this->pressed = false;
    this->setCursor(Qt::ArrowCursor);
    return QChartView::mouseReleaseEvent(e);
}

void PCMWidget::mouseMoveEvent(QMouseEvent *e) {
    if(is_drag_event(e)) {
        const auto pos = e->pos();
        const auto d = std::exchange(this->last_pos, pos) - pos;
        if(this->pressed)
            this->chart()->scroll(d.x(), 0);
    }
    return QChartView::mouseMoveEvent(e);
}

void PCMWidget::reset_zoom(void) {
    const auto *const c = this->chart();
    const auto l = c->series();
    if(l.empty())
        return;
    const auto n = static_cast<qreal>(
        static_cast<QtCharts::QLineSeries*>(l[0])->count());
    c->axes()[0]->setRange(0, n / static_cast<qreal>(this->rate));
}

void PCMWidget::set_pos(std::size_t p) {
    if(auto *const s = get_series(this->chart(), 1)) {
        const auto pf = static_cast<qreal>(p) / static_cast<qreal>(this->rate);
        s->replace(0, pf, -1);
        s->replace(1, pf, 1);
    }
}

void PCMWidget::update(std::span<std::byte> v) {
    const auto wav = nngn::WAV{v};
    replace_series(this->chart(), wav);
    this->rate = wav.rate();
}

void PCMWidget::clear(void) {
    auto *const c = this->chart();
    c->removeAllSeries();
    for(auto x : c->axes())
        c->removeAxis(x);
}

}

#include "pcm.h"

#include <cfloat>
#include <cmath>

#include <QGLWidget>
#include <QtCharts/QLineSeries>

#include "audio/wav.h"
#include "utils/utils.h"

namespace {

constexpr auto drag_btn = Qt::LeftButton;
constexpr auto drag_mod = Qt::ControlModifier;

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
    if(b & Qt::MiddleButton) {
        this->reset_zoom();
    } else if((b & drag_btn) && (e->modifiers() & drag_mod)) {
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
    if((e->buttons() & drag_btn) && (e->modifiers() & drag_mod)) {
        const auto pos = e->pos();
        const auto d = std::exchange(this->last_pos, pos) - pos;
        if(this->pressed)
            this->chart()->scroll(d.x(), 0);
    }
    return QChartView::mouseMoveEvent(e);
}

void PCMWidget::reset_zoom(void) {
    const auto *const c = this->chart();
    const auto *const s = static_cast<QtCharts::QLineSeries*>(c->series()[0]);
    c->axes()[0]->setRange(
        0, static_cast<qreal>(s->count()) / static_cast<qreal>(this->rate));
}

void PCMWidget::set_pos(std::size_t p) {
    auto *const c = this->chart();
    const auto gp = static_cast<qreal>(p) / this->max;
    const auto s = gp * static_cast<qreal>(c->plotArea().width());
    this->chart()->scroll(s - this->scroll, 0);
    this->scroll = s;
}

void PCMWidget::update(std::span<std::byte> v) {
    const auto wav = nngn::WAV{v};
    assert(wav.channels() == 1);
    assert(wav.bits_per_sample() == 16);
    this->rate = wav.rate();
    const auto rate_f = static_cast<qreal>(wav.rate());
    auto *const chart = this->chart();
    QtCharts::QLineSeries *series = nullptr;
    if(const auto l = chart->series(); !l.empty())
        series = static_cast<QtCharts::QLineSeries*>(l[0]);
    else {
        series = new QtCharts::QLineSeries;
        chart->addSeries(series);
        chart->createDefaultAxes();
        series->setUseOpenGL(true);
    }
    const auto s = std::span{
        byte_cast<i16*>(wav.data().data()),
        wav.data().size() / 2,
    };
    const auto n = s.size();
    QVector<QPointF> pv = {};
    pv.reserve(static_cast<int>(n));
    constexpr std::size_t n_avg = 1;
    qreal x = 0;
    for(std::size_t i = 0; i < n; i += n_avg) {
        const auto e = std::min(n, i + n_avg);
        pv.append({
            x++ * n_avg / rate_f,
            std::reduce(&s[i], &s[e])
                / static_cast<qreal>(e - i)
                / static_cast<qreal>(INT16_MAX),
        });
    }
    series->replace(pv);
    const auto axes = chart->axes();
    this->max = x * n_avg;
    axes[0]->setRange(0, this->max / rate_f);
    axes[1]->setRange(-1, 1);
}

}

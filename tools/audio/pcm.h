#ifndef NNGN_TOOLS_AUDIO_PCM_H
#define NNGN_TOOLS_AUDIO_PCM_H

#include <span>

#include <QtCharts/QChartView>

namespace nngn {

class PCMWidget : public QtCharts::QChartView {
    Q_OBJECT
public:
    PCMWidget(void);
    ~PCMWidget(void) override = default;
    void set_pos(std::size_t p);
public slots:
    void update(std::span<std::byte> v);
private:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void reset_zoom(void);
    std::size_t rate = {};
    qreal max = {}, scroll = {};
    QPoint last_pos = {};
    bool pressed = {};
};

}

#endif

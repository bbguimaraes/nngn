#ifndef NNGN_TOOLS_AUDIO_PCM_H
#define NNGN_TOOLS_AUDIO_PCM_H

#include <span>

#include <QtCharts/QChartView>

namespace nngn {

class PCMWidget final : public QtCharts::QChartView {
    Q_OBJECT
public:
    PCMWidget(void);
    void set_pos(std::size_t p);
public slots:
    void update(std::span<std::byte> v);
    void clear(void);
private:
    void mousePressEvent(QMouseEvent *e) final;
    void mouseReleaseEvent(QMouseEvent *e) final;
    void mouseMoveEvent(QMouseEvent *e) final;
    void reset_zoom(void);
    std::size_t rate = {};
    QPoint last_pos = {};
    bool pressed = {};
};

}

#endif

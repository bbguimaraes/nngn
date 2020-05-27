#ifndef NNGN_TOOLS_TIMELINE_GRAPH_H
#define NNGN_TOOLS_TIMELINE_GRAPH_H

#include <vector>
#include <QVector>
#include <QtCharts/QChartView>

namespace QtCharts { class QLineSeries; }

class Graph : public QtCharts::QChartView {
    Q_OBJECT
public:
    enum class Mode { ABSOLUTE, RELATIVE };
    Graph();
    Graph(Graph&) = delete;
    Graph &operator=(Graph&) = delete;
    Mode mode() const { return this->m_mode; }
public slots:
    void new_graph(QString title);
    void new_data(const QVector<qreal> &v);
    void set_mode(Mode m) { this->m_mode = m; }
private:
    using QLineSeries = QtCharts::QLineSeries;
    std::vector<QLineSeries*> series = {};
    Mode m_mode = Mode::RELATIVE;
};

#endif

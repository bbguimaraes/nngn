#ifndef NNGN_TOOLS_AUDIO_FLOW_GRID_LAYOUT_H
#define NNGN_TOOLS_AUDIO_FLOW_GRID_LAYOUT_H

#include <QGridLayout>

namespace nngn {

/**
 * Resizable `QGridLayout` composed of uniform, fixed-size widgets.
 * Based on https://doc.qt.io/qt-6/qtwidgets-layouts-flowlayout-example.html.
 */
class FlowGridLayout final : public QGridLayout {
public:
    using QGridLayout::QGridLayout;
    QSize minimumSize(void) const final;
    QSize sizeHint(void) const final;
    void setGeometry(const QRect &rect) final;
private:
    QSize size_for_columns(int c) const;
    int rows = 0, columns = 0;
};

}

#endif

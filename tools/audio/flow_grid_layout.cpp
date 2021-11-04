#include "flow_grid_layout.h"

#include <QWidget>

#include "math/math.h"

namespace {

std::optional<QSize> widget_size(const QLayout &l) {
    if(auto *const i = l.itemAt(0))
        return i->widget()->size();
    return {};
}

QSize final_size(QSize size, int rows, int cols, QMargins m, int spacing) {
    size = {cols * size.width(), rows * size.height()};
    size += QSize{spacing * (cols - 1), spacing * (rows - 1)};
    size += QSize{22 + m.left() + m.right(), m.bottom() + m.top()};
    return size;
}

void reset_grid(QGridLayout *l, int rows0, int rows1, int cols0, int cols1) {
    QList<QLayoutItem*> items = {};
    while(auto *i = l->takeAt(0))
        items.push_back(i);
    for(int r = 0; r != rows1; ++r)
        for(int c = 0; c != cols1 && !items.empty(); ++c)
            l->addItem(items.takeAt(0), r, c);
    if(rows0) l->setRowStretch(rows0, 0);
    if(rows1) l->setRowStretch(rows1, 1);
    if(cols0) l->setColumnStretch(cols0, 0);
    if(cols1) l->setColumnStretch(cols1, 1);
}

}

namespace nngn {

QSize FlowGridLayout::size_for_columns(int c) const {
    if(const auto size = widget_size(*this))
        return final_size(
            *size, nngn::Math::round_up_div(this->count(), c), c,
            this->contentsMargins(), this->spacing());
    return {};
}

QSize FlowGridLayout::minimumSize(void) const {
    return {this->size_for_columns(1).width(), 0};
}

QSize FlowGridLayout::sizeHint(void) const {
    return this->size_for_columns(this->columns ? this->columns : 3);
}

void FlowGridLayout::setGeometry(const QRect &rect) {
    const auto size = widget_size(*this);
    if(!size)
        return QGridLayout::setGeometry(rect);
    const auto sp = this->spacing();
    const auto margins = this->contentsMargins();
    const auto width = rect.width() - margins.left() - margins.right() + sp;
    const int c = std::max(1, width / (size->width() + sp));
    if(c != this->columns) {
        const int r = nngn::Math::round_up_div(this->count(), c);
        reset_grid(this, this->rows, r, this->columns, c);
        this->rows = r, this->columns = c;
    }
    const auto s = final_size(*size, this->rows, this->columns, margins, sp);
    QRect r = rect;
    r.setHeight(s.height());
    r.setWidth(s.width());
    QGridLayout::setGeometry(r);
}

}

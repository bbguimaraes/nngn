#include "component_button.h"

#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>

namespace nngn {

ComponentButton::ComponentButton(
    int icon_size, const QString &name, const QString &c, const QString &desc,
    const QString &icon)
:
    QToolButton{}, m_code{c}
{
    this->setToolTip(QString{"<pre>%1%2</pre>"}.arg(c, desc));
    this->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    this->setText(name);
    this->setIcon(QIcon{icon});
    this->setIconSize({icon_size, icon_size});
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ComponentButton::mouseMoveEvent(QMouseEvent *e) {
    QToolButton::mouseMoveEvent(e);
    if(~e->buttons() & Qt::LeftButton)
        return;
    auto *const m = new QMimeData;
    m->setText(this->m_code);
    auto *const d = new QDrag(this);
    d->setMimeData(m);
    d->exec();
}

}

#ifndef NNGN_TOOLS_AUDIO_COMPONENT_BUTTON_H
#define NNGN_TOOLS_AUDIO_COMPONENT_BUTTON_H

#include <QToolButton>

namespace nngn {

class ComponentButton final : public QToolButton {
public:
    ComponentButton(
        int icon_size, const QString &name, const QString &code,
        const QString &desc, const QString &icon);
    QString code(void) const { return this->m_code; }
    void mouseMoveEvent(QMouseEvent*) final;
private:
    QString m_code = {};
};

}

#endif

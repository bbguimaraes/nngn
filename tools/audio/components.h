#ifndef NNGN_TOOLS_AUDIO_COMPONENTS_H
#define NNGN_TOOLS_AUDIO_COMPONENTS_H

#include <QWidget>

namespace nngn {

class Components : public QWidget {
    Q_OBJECT
public:
    Components(QWidget *parent = {}, Qt::WindowFlags flags = {});
signals:
    void clicked(const QString &s);
};

}

#endif

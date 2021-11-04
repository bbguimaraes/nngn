#ifndef NNGN_TOOLS_AUDIO_AUDIO_H
#define NNGN_TOOLS_AUDIO_AUDIO_H

#include <vector>

#include <QWidget>

namespace nngn {

class AudioWidget : public QWidget {
    Q_OBJECT
public:
    AudioWidget(void);
    void add(QWidget *w);
    QSize sizeHint(void) const final { return {640, 400}; }
public slots:
    void add_edit(void);
    void add_pcm(std::vector<std::byte> v);
    void new_pos(std::size_t i, std::size_t p);
private:
    std::vector<std::vector<std::byte>> wavs = {};
};

}

#endif

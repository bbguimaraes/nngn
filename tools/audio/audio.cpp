#include "audio.h"

#include <QVBoxLayout>

#include "edit.h"
#include "pcm.h"

namespace nngn {

AudioWidget::AudioWidget(void) { new QVBoxLayout{this}; }
void AudioWidget::add(QWidget *w) { this->layout()->addWidget(w); }
void AudioWidget::add_edit(void) { this->add(new Edit); }

void AudioWidget::add_pcm(std::vector<std::byte> v) {
    this->wavs.emplace_back(std::move(v));
    auto *const pcm = new PCMWidget;
    this->add(pcm);
    pcm->update(this->wavs.back());
    pcm->setMinimumSize(320, 200);
}

void AudioWidget::new_pos(std::size_t i, std::size_t p) {
    assert(static_cast<int>(i) < this->layout()->count());
    if(auto *const w = this->layout()->itemAt(static_cast<int>(i))->widget())
        static_cast<PCMWidget*>(w)->set_pos(p);
}

}

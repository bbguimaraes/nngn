#include <QApplication>

#include "audio/wav.h"
#include "utils/utils.h"

#include "audio.h"
#include "pcm.h"
#include "worker.h"

namespace {

bool load(nngn::AudioWidget *w, const char *filename) {
    std::vector<std::byte> v = {};
    if(!nngn::read_file(filename, &v))
        return false;
    w->add_pcm(std::move(v));
    return true;
}

}

int main(int argc, char **argv) {
    auto app = QApplication{argc, argv};
    app.setCursorFlashTime(0);
    qRegisterMetaType<std::vector<std::byte>>("std::vector<std::byte>");
    qRegisterMetaType<std::size_t>("std::size_t");
    nngn::AudioWorker worker;
    if(!worker.init(0))
        return 1;
    nngn::AudioWidget window;
    window.add_edit();
    for(auto **p = argv + 1; *p; ++p)
        if(!load(&window, *p))
            return 1;
    std::vector<std::vector<std::byte>> wavs = {};
    QObject::connect(
        &worker, &nngn::AudioWorker::finished,
        &window, &nngn::AudioWidget::close);
    QObject::connect(
        &worker, &nngn::AudioWorker::new_graph,
        &window, &nngn::AudioWidget::add_pcm);
    QObject::connect(
        &worker, &nngn::AudioWorker::new_pos,
        &window, &nngn::AudioWidget::new_pos);
    QObject::connect(
        &app, &QApplication::lastWindowClosed,
        &worker, &nngn::AudioWorker::finish);
    window.show();
    worker.start();
    const auto ret = app.exec();
    worker.wait();
    worker.destroy();
    return ret ? ret : !worker.ok();
}

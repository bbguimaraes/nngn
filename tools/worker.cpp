#include "reader.h"
#include "worker.h"

bool Worker::init(int in_fd) {
    this->fd = in_fd;
    return this->poller.init() || !(this->error = true);
}

bool Worker::destroy() {
    return this->poller.destroy() || !(this->error = true);
}

void Worker::err() { this->error = true; emit this->finished(); }
void Worker::finish() { if(!this->poller.finish()) this->error = true; }

void Worker::run() {
    LineReader reader(1024);
    for(;;) {
        switch(poller.poll(this->fd)) {
        case Poller::result::ERR: return this->err();
        case Poller::result::DONE: return emit this->finished();
        case Poller::result::CANCELLED: return;
        case Poller::result::READ: break;
        }
        switch(reader.read(this->fd)) {
        case -1: return this->err();
        case 0: return emit this->finished();
        default:
            if(!reader.for_each([this](auto l) { return this->cmd(l); }))
                return;
        }
    }
}

#include "reader.h"
#include "worker.h"

bool Worker::init(int fd_) {
    this->fd = fd_;
    return this->poller.init() || !(this->error = true);
}

bool Worker::destroy(void) {
    return this->poller.destroy() || !(this->error = true);
}

void Worker::err(void) { this->error = true; emit this->finished(); }
void Worker::finish(void) { if(!this->poller.finish()) this->error = true; }

void Worker::run(void) {
    LineReader reader = {};
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

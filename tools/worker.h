#ifndef NNGN_TOOLS_WORKER_H
#define NNGN_TOOLS_WORKER_H

#include <QThread>
#include "poller.h"

class Worker : public QThread {
    Q_OBJECT
    bool error = false;
    int fd = -1;
    Poller poller = {};
    virtual bool cmd(std::string_view s) = 0;
protected:
    void err();
public:
    bool init(int in_fd);
    bool destroy();
    void run() override;
    constexpr bool ok() const { return !this->error; }
public slots:
    void finish();
signals:
    void finished();
};

#endif

#ifndef NNGN_TOOLS_WORKER_H
#define NNGN_TOOLS_WORKER_H

#include <iomanip>

#include <QThread>

#include "utils/log.h"

#include "poller.h"

class Worker : public QThread {
    Q_OBJECT
protected:
    template<typename ...Ts>
    auto read_values(std::stringstream *s, Ts *...ts);
    void err(void);
public:
    bool init(int fd);
    bool destroy(void);
    void run(void) final;
    constexpr bool ok(void) const { return !this->error; }
public slots:
    void finish(void);
signals:
    void finished(void);
private:
    virtual bool cmd(std::string_view s) = 0;
    bool error = false;
    int fd = -1;
    Poller poller = {};
};

template<typename ...Ts>
auto Worker::read_values(std::stringstream *s, Ts *...ts) {
    (*s >> ... >> *ts);
    if(!s->bad())
        return true;
    nngn::Log::l()
        << "failed to read values from "
        << std::quoted(s->str()) << '\n';
    return false;
}

#endif

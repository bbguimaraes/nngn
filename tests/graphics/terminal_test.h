#ifndef NNGN_TESTS_GRAPHICS_TERMINAL_H
#define NNGN_TESTS_GRAPHICS_TERMINAL_H

#include <QTest>

class TerminalTest : public QObject {
    Q_OBJECT
private slots:
    void texture_sample(void);
    void ascii_empty(void);
    void ascii_write(void);
    void colored_empty(void);
    void colored_write(void);
    void dedup(void);
};

#endif

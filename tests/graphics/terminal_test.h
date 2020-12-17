#ifndef NNGN_TESTS_GRAPHICS_TERMINAL_H
#define NNGN_TESTS_GRAPHICS_TERMINAL_H

#include <QTest>

class TerminalTest : public QObject {
    Q_OBJECT
private slots:
    void texture_sample(void);
    void empty(void);
    void write(void);
};

#endif

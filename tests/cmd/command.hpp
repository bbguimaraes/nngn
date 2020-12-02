#ifndef IMPERO_TESTS_CMD_COMMAND_H
#define IMPERO_TESTS_CMD_COMMAND_H

#include <QTest>

class CommandTest : public QObject {
    Q_OBJECT
private slots:
    void exec_command(void);
};

#endif

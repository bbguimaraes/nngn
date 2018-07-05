#ifndef NNGN_TEST_TEXTBOX_H
#define NNGN_TEST_TEXTBOX_H

#include <QTest>

class TextboxTest : public QObject {
    Q_OBJECT
private slots:
    void constructor();
    void update();
};

#endif

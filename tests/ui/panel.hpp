#ifndef IMPERO_TESTS_UI_PANEL_H
#define IMPERO_TESTS_UI_PANEL_H

#include <QTest>

class PanelTest : public QObject {
    Q_OBJECT
private slots:
    void move_selection(void);
    void update_filter(void);
};

#endif

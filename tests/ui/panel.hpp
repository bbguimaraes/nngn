#ifndef IMPERO_TESTS_UI_PANEL_H
#define IMPERO_TESTS_UI_PANEL_H

#include <QTest>

class PanelTest : public QObject {
    Q_OBJECT
private slots:
    void move_selection(void);
    void matches_simple_data(void);
    void matches_simple(void);
    void matches_substring(void);
    void matches_substring_data(void);
    void update_filter(void);
};

#endif

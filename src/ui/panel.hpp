#ifndef IMPERO_UI_PANEL_H
#define IMPERO_UI_PANEL_H

#include <vector>

#include <QtWidgets/QWidget>

namespace impero {

class Command;

class Panel : public QWidget {
    Q_OBJECT
    std::size_t n_visible = {}, cur = {};
public:
    explicit Panel(QWidget *p = nullptr);
    void add_command(std::string_view s);
signals:
    void command_selected(std::size_t) const;
public slots:
    void update_filter(const QString &filter);
    void move_selection(bool forward);
    void select_command(void) const;
};

}

#endif

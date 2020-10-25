#include <QProcess>
#include <QScreen>

#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QStyle>
#include <QtWidgets/QWidget>

#include "cmd/command.hpp"
#include "ui/panel.hpp"
#include "ui/widget.hpp"

namespace {

void update_filter(QWidget *w) {
    w->adjustSize();
    w->setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight, Qt::AlignCenter,
            w->size(), w->screen()->availableGeometry()));
}

bool exec_command(std::string_view c) {
    QStringList args;
    if(const auto i = c.find(' '); i != std::string_view::npos) {
        const auto s = c.substr(i + 1);
        c = c.substr(0, i);
        args = QString::fromUtf8(s.data(), s.size()).split(' ');
    }
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(QString::fromUtf8(c.data(), c.size()), args);
    return p.waitForFinished(-1);
}

auto load_commands(char **argv) {
    std::vector<impero::ExecCommand<exec_command>> ret = {};
    ++argv;
    while(*argv)
        ret.emplace_back(*argv++);
    return ret;
}

}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    auto cmds = load_commands(argv);
    QWidget parent;
    impero::Widget w(&parent);
    QLineEdit e;
    impero::Panel p(&w);
    w.add_edit(&e);
    w.add_panel(&p);
    impero::Command *cmd = {};
    const auto update_filter = [&w] { ::update_filter(&w); };
    QObject::connect(
        &w, &impero::Widget::selection_moved,
        &p, &impero::Panel::move_selection);
    QObject::connect(
        &e, &QLineEdit::textChanged,
        &p, &impero::Panel::update_filter);
    QObject::connect(&e, &QLineEdit::textChanged, update_filter);
    QObject::connect(
        &e, &QLineEdit::editingFinished,
        &p, &impero::Panel::select_command);
    QObject::connect(
        &e, &QLineEdit::editingFinished,
        &w, &QWidget::close);
    QObject::connect(
        &p, &impero::Panel::command_selected,
        [&cmds, &cmd](std::size_t i) {
            assert(i < cmds.size()); cmd = &cmds[i];
        });
    w.setWindowFlags(Qt::Popup);
    e.setFocus();
    for(auto &x : cmds)
        p.add_command(x.command());
    update_filter();
    w.show();
    if(auto ret = app.exec())
        return ret;
    return cmd && !cmd->execute();
}

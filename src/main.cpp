#include <cmath>
#include <utility>

#include <QCommandLineParser>
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

QStringList parse_args(const QCoreApplication &app) {
    QCommandLineParser parser;
    parser.setApplicationDescription("impero");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("commands...", "list of available commands");
    parser.process(app);
    return parser.positionalArguments();
}

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

auto load_commands(QStringList &&args) {
    std::vector<impero::ExecCommand<exec_command>> ret = {};
    for(auto &&x : std::move(args))
        ret.emplace_back(std::move(x).toStdString());
    const auto n = ret.size();
    const auto cols =
        n / std::max(std::size_t{1}, static_cast<std::size_t>(std::sqrt(n)));
    return std::tuple(ret, cols);
}

}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    auto [cmds, n_rows] = load_commands(parse_args(app));
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
    for(std::size_t i = 0, n = cmds.size(); i < n; ++i)
        p.add_command(cmds[i].command(), i % n_rows, i / n_rows);
    update_filter();
    w.show();
    if(auto ret = app.exec())
        return ret;
    return cmd && !cmd->execute();
}

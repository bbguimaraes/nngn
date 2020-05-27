#include <cstdint>

#include <QtCore/QTextStream>
#include <QtGui/QScreen>
#include <QtNetwork/QLocalSocket>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

#include "window.h"

using namespace std::string_view_literals;

namespace {

constexpr auto SOCKET_PATH = "/home/bbguimaraes/src/nngn/sock"sv;
constexpr auto TIMELINE_PROF = R"(
do
    require("nngn.lib.tools").add_to_path()
    local p = require("nngn.lib.profile")
    if not p.active(Profile) then p.activate(Profile) end
    local t = require("nngn.lib.timeline")
    t.timeline(t.FS.profile)
end)"sv;

class Launcher {
public:
    const QLocalSocket &socket() const { return this->sock; }
    bool ok() const { return this->flags & Flag::OK; }
    void fail() { this->flags = static_cast<Flag>(this->flags & ~Flag::OK); }
    bool open_socket();
    bool exec(std::string_view cmd);
private:
    enum Flag : std::uint8_t { OK = 1u << 0, ADDED_TO_PATH = 1u << 1 };
    QLocalSocket sock = {};
    Flag flags = Flag::OK;
};

bool Launcher::open_socket() {
    this->sock.connectToServer(
        QString::fromUtf8(SOCKET_PATH.data(), SOCKET_PATH.size()),
        QIODevice::WriteOnly);
    return this->sock.waitForConnected(-1);
}

bool Launcher::exec(std::string_view cmd) {
    if(!this->sock.isOpen() && !this->open_socket())
        return false;
    QTextStream(&this->sock) << cmd.data();
    return this->sock.waitForBytesWritten(-1);
}

}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    Launcher launcher;
    if(!launcher.open_socket())
        QMessageBox::warning(
            nullptr, "Warning",
            "Failed to open socket.\n\n" + launcher.socket().errorString());
    Window w;
    const auto timeline_section = w.add_section("timeline");
    const auto *const timeline_prof = w.add_button(timeline_section, "prof");
    QObject::connect(
        &launcher.socket(), &QLocalSocket::errorOccurred,
        [&launcher, &w](QLocalSocket::LocalSocketError error) {
            if(error == QLocalSocket::PeerClosedError)
                return (void)w.close();
            launcher.fail();
            QMessageBox::critical(
                &w, "Error",
                "Socket error.\n\n" + launcher.socket().errorString());
        });
    QObject::connect(
        &launcher.socket(), &QLocalSocket::errorOccurred,
        &w, &QWidget::close);
    QObject::connect(
        timeline_prof, &QPushButton::pressed,
        [&l = launcher] { l.exec(TIMELINE_PROF); });
    const auto screen_height =
        app.primaryScreen()->availableGeometry().height();
    w.setWindowFlags(Qt::Dialog);
    w.adjustSize();
    w.move(32, (screen_height - w.height()) / 2);
    w.show();
    if(const int ret = app.exec())
        return ret;
    return !launcher.ok();
}

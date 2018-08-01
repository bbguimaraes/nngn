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

constexpr auto SOCKET_PATH = "sock"sv;
constexpr auto INSPECT_TEXTURE = R"(
do
    require("nngn.lib.tools").add_to_path()
    local i = require("nngn.lib.inspect")
    i.inspect(i.FS.textures)
end)"sv;
constexpr auto PLOT_FPS = R"(
do
    require("nngn.lib.tools").add_to_path()
    local p = require("nngn.lib.plot")
    p.plot(p.FS.fps)
end)"sv;
constexpr auto PLOT_LUA = R"(
do
    require("nngn.lib.tools").add_to_path()
    local p = require("nngn.lib.plot")
    p.plot(p.FS.lua)
end)"sv;
constexpr auto PLOT_RENDER = R"(
do
    require("nngn.lib.tools").add_to_path()
    local p = require("nngn.lib.plot")
    p.plot(p.FS.render)
end)"sv;
constexpr auto PLOT_GRAPHICS = R"(
do
    require("nngn.lib.tools").add_to_path()
    local p = require("nngn.lib.plot")
    p.plot(p.FS.graphics)
end)"sv;
constexpr auto TIMELINE_PROF = R"(
do
    require("nngn.lib.tools").add_to_path()
    local p = require("nngn.lib.profile")
    if not p.active(Profile) then p.activate(Profile) end
    local t = require("nngn.lib.timeline")
    t.timeline(t.FS.profile)
end)"sv;
constexpr auto TIMELINE_LUA = R"(
do
    require("nngn.lib.tools").add_to_path()
    local p = require("nngn.lib.profile")
    if not p.active(Profile) then p.activate(Profile) end
    local t = require("nngn.lib.timeline")
    t.timeline(t.FS.lua)
end)"sv;
constexpr auto CONFIGURE_LIMITS = R"(
do
    require("nngn.lib.tools").add_to_path()
    local p = require("nngn.lib.configure")
    p.configure(p.FS.limits)
end)"sv;
constexpr auto CONFIGURE_GRAPHICS = R"(
do
    require("nngn.lib.tools").add_to_path()
    local p = require("nngn.lib.configure")
    p.configure(p.FS.graphics)
end)"sv;

class Launcher {
public:
    explicit Launcher(std::string_view path_) : path{path_} {}
    const QLocalSocket &socket() const { return this->sock; }
    bool ok() const { return this->flags & Flag::OK; }
    void fail() { this->flags = static_cast<Flag>(this->flags & ~Flag::OK); }
    bool open_socket();
    bool exec(std::string_view cmd);
private:
    enum Flag : std::uint8_t { OK = 1u << 0, ADDED_TO_PATH = 1u << 1 };
    std::string_view path;
    QLocalSocket sock = {};
    Flag flags = Flag::OK;
};

bool Launcher::open_socket() {
    this->sock.connectToServer(
        QString::fromUtf8(
            this->path.data(),
            static_cast<int>(this->path.size())),
        QIODevice::WriteOnly);
    return this->sock.waitForConnected(-1);
}

bool Launcher::exec(std::string_view cmd) {
    if(this->sock.state() == QLocalSocket::UnconnectedState)
        if(!this->open_socket())
            return false;
    QTextStream(&this->sock) << cmd.data();
    return this->sock.waitForBytesWritten(-1);
}

}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    const auto path = argc > 1 ? argv[1] : SOCKET_PATH;
    Launcher launcher{path};
    if(!launcher.open_socket())
        QMessageBox::warning(
            nullptr, "Warning",
            "Failed to open socket: "
                + QString::fromUtf8(path.data(), static_cast<int>(path.size()))
                + "\n\n"
                + launcher.socket().errorString());
    Window w;
    const auto inspect_section = w.add_section("inspect");
    const auto plot_section = w.add_section("plot");
    const auto timeline_section = w.add_section("timeline");
    const auto configure_section = w.add_section("configure");
    const auto add = [&w, &l = launcher](auto s, const char *n, auto p) {
        QObject::connect(
            w.add_button(s, n), &QPushButton::pressed,
            [&l, p] { l.exec(p); });
    };
    add(inspect_section, "texture", INSPECT_TEXTURE);
    add(plot_section, "fps", PLOT_FPS);
    add(plot_section, "lua", PLOT_LUA);
    add(plot_section, "render", PLOT_RENDER);
    add(plot_section, "graphics", PLOT_GRAPHICS);
    add(timeline_section, "prof", TIMELINE_PROF);
    add(timeline_section, "lua", TIMELINE_LUA);
    add(configure_section, "limits", CONFIGURE_LIMITS);
    add(configure_section, "graphics", CONFIGURE_GRAPHICS);
    QObject::connect(
        &launcher.socket(), &QLocalSocket::errorOccurred,
        [&launcher, &w](QLocalSocket::LocalSocketError error) {
            if(error == QLocalSocket::PeerClosedError)
                return;
            launcher.fail();
            w.close();
            QMessageBox::critical(
                &w, "Error",
                "Socket error.\n\n" + launcher.socket().errorString());
        });
    const auto screen = app.primaryScreen()->availableGeometry();
    w.setWindowFlags(Qt::Dialog);
    w.adjustSize();
    w.move(screen.width() - w.width() - 32, (screen.height() - w.height()) / 2);
    w.show();
    if(const int ret = app.exec())
        return ret;
    return !launcher.ok();
}

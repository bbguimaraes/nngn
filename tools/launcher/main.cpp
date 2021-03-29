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

class Launcher {
public:
    explicit Launcher(std::string_view path_) : path{path_} {}
    const QLocalSocket &socket() const { return this->sock; }
    bool ok() const { return this->flags & Flag::OK; }
    void fail() { this->flags = static_cast<Flag>(this->flags & ~Flag::OK); }
    bool open_socket();
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

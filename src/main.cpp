#include "aiws/gui/main_window.hpp"

#include <QApplication>
#include <QMessageBox>
#include <QStringList>

#include <cstdlib>
#include <string>
#include <unistd.h>

// TODO: Define the ExitCallback function used to confirm an Exit request.
void confirm_exit(QWidget* parent) {
    if (QMessageBox::question(parent, QStringLiteral("Exit"),
                              QStringLiteral("Are you sure you want to exit?"))
        == QMessageBox::Yes) {
        QApplication::quit();
    }
}

// handles the --build-delay-ms flag
// expects --build-delay-ms N or --build-delay-ms=N
// no flag or invalid flag sets it to 0
int parse_build_delay_ms(const QStringList& args) {
    const QString flag = QStringLiteral("--build-delay-ms");
    for (int i = 1; i < args.size(); i++) {
        QString value;
        if (args[i] == flag && i + 1 < args.size()) {
            value = args[i + 1];
        } else if (args[i].startsWith(flag + QLatin1Char('='))) {
            value = args[i].mid(flag.size() + 1);
        } else {
            continue;
        }
        bool ok = false;
        const int ms = value.toInt(&ok);
        return (ok && ms > 0) ? ms : 0;
    }

    return 0;
}

std::string parse_ipc_socket(const QStringList& args) {
    const QString flag = QStringLiteral("--ipc-socket");

    for (int i = 1; i < args.size(); i++) {
        if (args[i] == flag && i + 1 < args.size()) {
            return args[i + 1].toStdString();
        }

        if (args[i].startsWith(flag + QLatin1Char('='))) {
            return args[i].mid(flag.size() + 1).toStdString();
        }
    }

    const char* user = std::getenv("USER");
    if (user && user[0]) {
        return std::string("/tmp/aiws-") + user + ".sock";
    }
    return std::string("/tmp/aiws-") + std::to_string(getuid()) + ".sock";
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    aiws::gui::MainWindow window;
    window.set_exit_callback(&confirm_exit);
    window.set_build_delay_ms(parse_build_delay_ms(QApplication::arguments()));
    window.start_ipc(parse_ipc_socket(QApplication::arguments()));
    window.show();

    return app.exec();
}

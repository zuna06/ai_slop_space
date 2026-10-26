#include "aiws/gui/main_window.hpp"

#include <QApplication>
#include <QMessageBox>

// TODO: Define the ExitCallback function used to confirm an Exit request.
void confirm_exit(QWidget* parent) {
    if (QMessageBox::question(parent, QStringLiteral("Exit"),
                              QStringLiteral("Are you sure you want to exit?"))
        == QMessageBox::Yes) {
        QApplication::quit();
    }
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    aiws::gui::MainWindow window;
    window.set_exit_callback(&confirm_exit);
    window.show();

    return app.exec();
}

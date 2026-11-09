// Your M4 tests (specification, Section 6).

#include "aiws/gui/corpus_builder.hpp"
#include "aiws/gui/main_window.hpp"
#include "aiws/gui/search_widget.hpp"

#include <QAbstractItemModel>
#include <QApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableView>
#include <QTimer>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>
#include <QFile>

#include <cstddef>
#include <iostream>
#include <cstdint>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

using aiws::gui::CorpusBuilder;
using aiws::gui::MainWindow;
using aiws::gui::SearchWidget;

namespace {
    int failures = 0;
    void check(bool ok, const char* name) {
        if (!ok) {
            std::cerr << "Fail: " << name << '\n';
            failures++;
        }
    }

    // max bound
    const int kWaitMs = 10000;

    const QString dir = QFileInfo(QStringLiteral(__FILE__)).absolutePath() + QStringLiteral("/../data/public");

    // helper functions
    QString path(const char* name) {
        return dir + QLatin1Char('/') + QString::fromLatin1(name);
    }

    QPushButton* button(QWidget& root, const char* text) {
        for (QPushButton* b : root.findChildren<QPushButton*>()) {
            if (b->text() == QLatin1String(text)) {
                return b;
            }
        } 

        return nullptr;
    }

    QString corpus_status(QWidget& root) {
        for (QLabel* label : root.findChildren<QLabel*>()) {
            if (label->text().startsWith(QStringLiteral("Corpus:"))) {
                return label->text();
            }
        }
        return {};
    }

    bool any_label_starts_with(QWidget& root, const char* prefix) {
        for (QLabel* label : root.findChildren<QLabel*>()) {
            if (label->text().startsWith(QLatin1String(prefix))) {
                return true;
            }
        }
        return false;
    }

    // helper functioned for checking when a build has finish
    bool wait_for(QSignalSpy& spy, int count) {
        QElapsedTimer timer;
        timer.start();
        while (spy.count() < count && timer.elapsed() < kWaitMs) {
            spy.wait(50);
        }
        return (spy.count() >= count);
    }


    // all tests

    void test_add_document_file() {
        MainWindow window;
        check(!window.add_document_file(path("missing.txt")), "missing file rejected");
        check(!window.add_document_file(dir), "directory rejected");
        check(window.findChild<QListWidget*>()->count() == 0, "rejected file changes nothing");
        check(!button(window, "Build Corpus")->isEnabled(), "build disabled with no documents");
        check(window.add_document_file(path("doc_ai.txt")), "readable file accepted");
        check(window.findChild<QListWidget*>()->count() == 1, "document listed");
        check(button(window, "Build Corpus")->isEnabled(), "build enabled with a document");
    }

    // R1 tests

    void test_event_loop_runs_during_build() {
        MainWindow window;
        window.set_build_delay_ms(300);
        window.add_document_file(path("doc_ai.txt"));
        window.add_document_file(path("doc_retrieval.txt"));
        window.add_document_file(path("doc_systems.txt"));
        QSignalSpy finished(window.findChild<CorpusBuilder*>(), &CorpusBuilder::build_finished);

        QTimer ticker;
        QSignalSpy ticks(&ticker, &QTimer::timeout);
        ticker.start(10);

        button(window, "Build Corpus")->click();
        check(wait_for(finished, 1), "slow build finishes");

        check(ticks.count() >= 5, "event loop keeps running during build");

    }

    // R2 tests

    void test_build_becomes_current() {
        MainWindow window;
        window.set_build_delay_ms(50);
        window.add_document_file(path("doc_ai.txt"));
        window.add_document_file(path("doc_retrieval.txt"));
        QSignalSpy finished(window.findChild<CorpusBuilder*>(), &CorpusBuilder::build_finished);

        button(window, "Build Corpus")->click();
        check(!button(window, "Build Corpus")->isEnabled(), "build disabled while building");
        check(!button(window, "Inspect")->isEnabled(), "inspect disabled while building");

        check(wait_for(finished, 1), "build finishes");
        check(corpus_status(window) == QStringLiteral("Corpus: 2 chunks"), "chunk count shown");

        window.findChild<SearchWidget*>()->findChild<QLineEdit*>()->setText(QStringLiteral("retrieval index term frequency"));
        check(button(window, "Search")->isEnabled(), "search enabled when current");
        button(window, "Search")->click();
        check(window.findChild<QTableView*>()->model()->rowCount() == 2, "search uses built corpus");
    }

    void test_add_during_build() {
        MainWindow window;
        window.set_build_delay_ms(20);
        window.add_document_file(path("doc_ai.txt"));
        QSignalSpy finished(window.findChild<CorpusBuilder*>(), &CorpusBuilder::build_finished);

        button(window, "Build Corpus")->click();
        // build can only be accepted when event loop runs
        window.add_document_file(path("doc_retrieval.txt"));
        check(corpus_status(window) == QStringLiteral("Corpus: Not built"), "add document invalidates during build");

        check(wait_for(finished, 1), "stale build finishes");
        check(corpus_status(window) == QStringLiteral("Corpus: Not built"), "stale build discarded after add document");
        check(!button(window, "Inspect")->isEnabled(), "inspect stays disabled after stale build");
        check(button(window, "Build Corpus")->isEnabled(), "build re enabled after add");
    }

    void test_reset_during_build() {
        MainWindow window;
        window.set_build_delay_ms(20);
        window.add_document_file(path("doc_ai.txt"));
        QSignalSpy finished(window.findChild<CorpusBuilder*>(), &CorpusBuilder::build_finished);

        button(window, "Build Corpus")->click();
        button(window, "Reset")->click();
        check(wait_for(finished, 1), "stale build finishes after reset");
        check(corpus_status(window) == QStringLiteral("Corpus: Not built"), "stale build discarded after reset");
        check(!button(window, "Inspect")->isEnabled(), "inspect disabled after reset");
    }

    void test_older_build_doesnt_overwrite_newer() {
        MainWindow window;
        window.add_document_file(path("doc_ai.txt"));
        QSignalSpy finished(window.findChild<CorpusBuilder*>(), &CorpusBuilder::build_finished);

        window.set_build_delay_ms(400);
        button(window, "Build Corpus")->click();
        window.add_document_file(path("doc_retrieval.txt"));
        window.set_build_delay_ms(0);
        button(window, "Build Corpus")->click();

        check(wait_for(finished, 2), "both builds finish");
        check(corpus_status(window) == QStringLiteral("Corpus: 2 chunks"), "newest build stays current");
    }

    // R3 tests

    void test_failed_build_reported() {
        MainWindow window;

        window.add_document_file(path("doc_ai.txt"));
        window.add_document_file(path("doc_ai.txt"));
        QSignalSpy finished(window.findChild<CorpusBuilder*>(), &CorpusBuilder::build_finished);

        button(window, "Build Corpus")->click();
        check(wait_for(finished, 1), "failing build finishes");
        check(corpus_status(window) == QStringLiteral("Corpus: Build failed"), "failure shown in corpus status");
        check(any_label_starts_with(window, "Build failed"), "failure message shown");
        check(button(window, "Build Corpus")->isEnabled(), "build re enabled after failure");
        check(button(window, "Add Document")->isEnabled(), "add enabled after failure");
        check(!button(window, "Inspect")->isEnabled(), "inspect disabled after failure");
    }


    // R4 tests

    void test_destroy_mid_build() {
        QElapsedTimer timer;

        // at the end the MainWindow window object is
        // destructed and the worker is joined
        {
            MainWindow window;
            window.set_build_delay_ms(5000);
            window.add_document_file(path("doc_ai.txt"));
            window.add_document_file(path("doc_retrieval.txt"));
            button(window, "Build Corpus")->click();
            timer.start();
        }
        
        check(timer.elapsed() < 2000, "destroying window mid build doesn't hang");
    }

    // should not leave any jobs
    void test_repeated_builds() {
        MainWindow window;
        window.set_build_delay_ms(20);
        auto* builder = window.findChild<CorpusBuilder*>();
        QSignalSpy finished(builder, &CorpusBuilder::build_finished);

        window.add_document_file(path("doc_ai.txt"));
        button(window, "Build Corpus")->click();
        window.add_document_file(path("doc_retrieval.txt"));
        button(window, "Build Corpus")->click();
        window.add_document_file(path("doc_systems.txt"));
        button(window, "Build Corpus")->click();

        check(wait_for(finished, 3), "all builds finish");
        check(builder->job_count() == 0, "no jobs left");
        check(corpus_status(window) == QStringLiteral("Corpus: 3 chunks"), "last build is current");
    }

    std::string test_socket(const char* tag) {
        return "/tmp/aiws-test-" + std::to_string(getpid()) + "-" + tag + ".sock";
    }

    int ipc_connect(const std::string& sock) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, sock.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(fd, (sockaddr*)(&addr), sizeof(addr)) < 0) {
            close(fd);
            return -1;
        }

        return fd;
    }

    uint8_t ipc_send_add(int fd, const std::string& file_path) {
        std::string payload = "add " + file_path;
        auto payload_len = static_cast<uint32_t>(payload.size());
        uint8_t header[4] = { static_cast<uint8_t>(payload_len >> 24), static_cast<uint8_t>(payload_len >> 16),
                              static_cast<uint8_t>(payload_len >> 8), static_cast<uint8_t>(payload_len) };

        write(fd, header, 4);
        write(fd, payload.data(), payload.size());

        uint8_t status{};
        return read(fd, &status, 1) == 1 ? status : 2;
    }

    bool wait_for_docs(QWidget& widget, int count) {
        QElapsedTimer timer;
        timer.start();
        while (widget.findChild<QListWidget*>()->count() < count && timer.elapsed() < 2000) {
            QTest::qWait(5);
        }

        return widget.findChild<QListWidget*>()->count() >= count;
    }

    // testing that remote add works the same as add document
    void test_ipc_add_document() {
        auto socket = test_socket("test");
        MainWindow window;
        window.start_ipc(socket);

        std::string file = QFileInfo(path("doc_ai.txt")).absoluteFilePath().toStdString();
        window.add_document_file(path("doc_systems.txt"));
        window.set_build_delay_ms(300);
        QSignalSpy finished(window.findChild<CorpusBuilder*>(), &CorpusBuilder::build_finished);
        button(window, "Build Corpus")->click();
        int fd = ipc_connect(socket);
        check(fd >= 0, "ipc connect");

        uint8_t status = ipc_send_add(fd, file);
        close(fd);

        check(status == 0, "ipc add accepted");

        check(wait_for_docs(window, 2), "document added via ipc");
        check(wait_for(finished, 1), "build started before remote add finishes");
        check(corpus_status(window) == QStringLiteral("Corpus: Not built"), "corpus invalidated after ipc add");
        check(button(window, "Build Corpus")->isEnabled(), "build enabled after ipc add");

        // rejected file returns -1;
        fd = ipc_connect(socket);
        status = ipc_send_add(fd, "/fake/file.txt");
        close(fd);

        check(status == 1, "ipc rejects missing file");
    }

    void test_ipc_special_paths() {
        auto socket = test_socket("test");
        MainWindow window;
        window.start_ipc(socket);

        QTemporaryDir directory;
        const QString special = QStringLiteral("aiws test ñ\nline.txt");
        QFile file(directory.filePath(special));
        if (!directory.isValid() || !file.open(QIODevice::WriteOnly)) {
            check(false, "create special path fixture");
            return;
        }
        check(file.write("test") == 4, "write special path fixture");
        file.close();

        QProcess sender;
        sender.setWorkingDirectory(directory.path());
        sender.setProgram(QApplication::applicationDirPath() + QStringLiteral("/aiws_send"));
        auto send_add = [&](const std::string& endpoint, const QString& filename) {
            sender.setArguments({"--socket", QString::fromStdString(endpoint), "add", filename});
            sender.start();
            if (!sender.waitForFinished(6000) || sender.exitStatus() != QProcess::NormalExit) return -1;
            return sender.exitCode();
        };
        check(send_add(socket, special) == 0, "sender resolves a relative UTF-8 path with a newline");
        if (wait_for_docs(window, 1)) {
            check(window.findChild<QListWidget*>()->item(0)->text() == special, "exact filename received");
        } else {
            check(false, "special path document received");
        }
        check(send_add(socket, "missing.txt") == 1, "sender reports rejection");
        check(!sender.readAllStandardError().trimmed().isEmpty(), "rejection gives a reason");
        check(send_add(socket + ".missing", special) == 2, "sender reports unreachable app");
    }

    void test_ipc_partial_writes() {
        auto socket = test_socket("test");
        MainWindow window;
        window.start_ipc(socket);

        int fd = ipc_connect(socket);
        check(fd >= 0, "ipc connect for partial writes");

        std::string file = QFileInfo(path("doc_ai.txt")).absoluteFilePath().toStdString();

        std::string payload = "add " + file;
        auto payload_len = static_cast<uint32_t>(payload.size());
        uint8_t header[4] = { static_cast<uint8_t>(payload_len >> 24), static_cast<uint8_t>(payload_len >> 16),
                              static_cast<uint8_t>(payload_len >> 8), static_cast<uint8_t>(payload_len) };
        // send header 2 bytes at a time
        // with a pause
        write(fd, header, 2);
        usleep(5000);
        write(fd, header + 2, 2);
        usleep(5000);

        // also send the payload in 2 parts
        size_t half = payload.size() / 2;
        write(fd, payload.data(), half);
        usleep(5000);
        write(fd, payload.data() + half, payload.size() - half);

        uint8_t status = 2;
        read(fd, &status, 1);
        close(fd);

        check(status == 0, "ipc handles partial writes");
        check(wait_for_docs(window, 1), "document added after partial writes");
    }

    void test_ipc_exit() {
        auto socket = test_socket("test");
        QElapsedTimer timer;
        int client = -1;
        {
            MainWindow window;
            window.set_build_delay_ms(5000);
            window.start_ipc(socket);
            check(QFileInfo(QString::fromStdString(socket)).exists(), "socket file created");
            
            window.add_document_file(path("doc_ai.txt"));
            button(window, "Build Corpus")->click();
            client = ipc_connect(socket);
            check(client >= 0 && send(client, "\0", 1, MSG_NOSIGNAL) == 1, "start a fragmented request before exit");
            timer.start();
        }
        // window is destroyed here in the middle of a build
        // the builds should cancel properly, the ipc should stop
        // and the socket file should be removed

        check(timer.elapsed() <= 500, "exit stops IPC within 500 ms");
        if (client >= 0) close(client);
        check(!QFileInfo(QString::fromStdString(socket)).exists(), "socket file removed after exit");
    }

}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    test_add_document_file();
    test_build_becomes_current();
    test_add_during_build();
    test_reset_during_build();
    test_older_build_doesnt_overwrite_newer();
    test_failed_build_reported();
    test_event_loop_runs_during_build();
    test_destroy_mid_build();
    test_repeated_builds();
    test_ipc_add_document();
    test_ipc_special_paths();
    test_ipc_partial_writes();
    test_ipc_exit();

    if (failures) {
        std::cerr << failures << " tests failed\n";
        return -1;
    }

    std::cout << "All tests passed\n";

    return 0;
}

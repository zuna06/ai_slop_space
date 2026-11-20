// Your M5 tests (specification, Section 7).

#include "aiws/gui/main_window.hpp"
#include "aiws/gui/search_widget.hpp"
#include "aiws/build_worker.hpp"

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

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

using aiws::BuildResult;
using aiws::BuildWorker;
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

    // waits until the corpus status label shows text
    // runs the event loop so queued build results arrive
    bool wait_for_status(QWidget& root, const char* text) {
        QElapsedTimer timer;
        timer.start();
        while (corpus_status(root) != QLatin1String(text) && timer.elapsed() < kWaitMs) {
            QTest::qWait(10);
        }
        return corpus_status(root) == QLatin1String(text);
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
        QSignalSpy finished(&window, &MainWindow::build_result_received);

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
        QSignalSpy finished(&window, &MainWindow::build_result_received);

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
        window.set_build_delay_ms(200);
        window.add_document_file(path("doc_ai.txt"));

        button(window, "Build Corpus")->click();
        // build can only be accepted when event loop runs
        window.add_document_file(path("doc_retrieval.txt"));
        check(corpus_status(window) == QStringLiteral("Corpus: Not built"), "add document invalidates during build");
        check(button(window, "Build Corpus")->isEnabled(), "build re enabled after add");

        // longer than the outdated build would have taken
        QTest::qWait(400);
        check(corpus_status(window) == QStringLiteral("Corpus: Not built"), "outdated build discarded after add document");
        check(!button(window, "Inspect")->isEnabled(), "inspect stays disabled after outdated build");

        window.set_build_delay_ms(0);
        button(window, "Build Corpus")->click();
        check(wait_for_status(window, "Corpus: 2 chunks"), "rebuild after add becomes current");
    }

    void test_reset_during_build() {
        MainWindow window;
        window.set_build_delay_ms(200);
        window.add_document_file(path("doc_ai.txt"));

        button(window, "Build Corpus")->click();
        button(window, "Reset")->click();
        check(corpus_status(window) == QStringLiteral("Corpus: Not built"), "reset invalidates during build");

        // longer than the outdated build would have taken
        QTest::qWait(400);
        check(corpus_status(window) == QStringLiteral("Corpus: Not built"), "outdated build discarded after reset");
        check(!button(window, "Inspect")->isEnabled(), "inspect disabled after reset");
    }

    void test_older_build_doesnt_overwrite_newer() {
        MainWindow window;
        window.add_document_file(path("doc_ai.txt"));

        window.set_build_delay_ms(400);
        button(window, "Build Corpus")->click();
        window.add_document_file(path("doc_retrieval.txt"));
        window.set_build_delay_ms(0);
        button(window, "Build Corpus")->click();

        check(wait_for_status(window, "Corpus: 2 chunks"), "newest build becomes current");
        // the older build would have finished by now if it kept running
        QTest::qWait(600);
        check(corpus_status(window) == QStringLiteral("Corpus: 2 chunks"), "newest build stays current");
    }

    // R3 tests

    void test_failed_build_reported() {
        MainWindow window;

        window.add_document_file(path("doc_ai.txt"));
        window.add_document_file(path("doc_ai.txt"));
        QSignalSpy finished(&window, &MainWindow::build_result_received);

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

    // only the last of several quick builds becomes current
    void test_repeated_builds() {
        MainWindow window;
        window.set_build_delay_ms(20);

        window.add_document_file(path("doc_ai.txt"));
        button(window, "Build Corpus")->click();
        window.add_document_file(path("doc_retrieval.txt"));
        button(window, "Build Corpus")->click();
        window.add_document_file(path("doc_systems.txt"));
        button(window, "Build Corpus")->click();

        check(wait_for_status(window, "Corpus: 3 chunks"), "last build is current");
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
        button(window, "Build Corpus")->click();
        int fd = ipc_connect(socket);
        check(fd >= 0, "ipc connect");

        uint8_t status = ipc_send_add(fd, file);
        close(fd);

        check(status == 0, "ipc add accepted");

        check(wait_for_docs(window, 2), "document added via ipc");
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

    // BuildWorker tests
    // these run without a window so the worker
    // is tested by itself

    bool has_job_id(const std::vector<std::uint64_t>& job_ids, std::uint64_t job_id) {
        return std::find(job_ids.begin(), job_ids.end(), job_id) != job_ids.end();
    }

    // records what the worker reports
    // callbacks run on the worker thread so
    // everything here is guarded by mutex
    struct WorkerLog {
        std::mutex mutex;
        std::condition_variable changed;
        std::vector<std::uint64_t> started;
        std::vector<std::uint64_t> finished;
        std::vector<std::thread::id> threads;

        void on_started(std::uint64_t job_id) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                started.push_back(job_id);
                threads.push_back(std::this_thread::get_id());
            }
            changed.notify_all();
        }

        void on_finished(const BuildResult& result) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                finished.push_back(result.job_id);
                threads.push_back(std::this_thread::get_id());
            }
            changed.notify_all();
        }

        // waits on the condition variable instead of polling
        // returns false if kWaitMs runs out first
        bool wait_started(std::uint64_t job_id) {
            std::unique_lock<std::mutex> lock(mutex);
            return changed.wait_for(lock, std::chrono::milliseconds(kWaitMs), [&] { return has_job_id(started, job_id); });
        }

        bool wait_finished(std::uint64_t job_id) {
            std::unique_lock<std::mutex> lock(mutex);
            return changed.wait_for(lock, std::chrono::milliseconds(kWaitMs), [&] { return has_job_id(finished, job_id); });
        }

        bool was_finished(std::uint64_t job_id) {
            std::lock_guard<std::mutex> lock(mutex);
            return has_job_id(finished, job_id);
        }
    };

    std::unique_ptr<BuildWorker> make_worker(WorkerLog& log) {
        return std::make_unique<BuildWorker>(
            [&log](BuildResult result) { log.on_finished(result); },
            [&log](std::uint64_t job_id) { log.on_started(job_id); }
        );
    }

    // made up docs so the worker tests
    // don't depend on the data folder
    aiws::Workspace sample_workspace(int docs) {
        aiws::Workspace workspace;
        for (int i = 0; i < docs; i++) {
            const std::string id = "doc" + std::to_string(i);
            workspace.add_document(aiws::Document(id, id, "retrieval index term frequency " + id));
        }
        return workspace;
    }

    // worker R4 tests

    // exit with an idle worker must not hang
    // runs first so a hang is caught fast
    void test_worker_exit_when_idle() {
        WorkerLog log;
        auto worker = make_worker(log);
        check(log.wait_finished(worker->submit(sample_workspace(1), 0)), "idle test build finishes");

        // give the worker time to go back to waiting
        // so stop() has to wake it up
        usleep(100 * 1000);

        std::promise<void> stopped;
        auto done = stopped.get_future();
        std::thread exiting([&] {
            worker.reset();
            stopped.set_value();
        });

        // a stuck join can't be undone so end
        // the whole test run here
        if (done.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
            std::cerr << "Fail: idle worker stops on exit\n";
            std::_Exit(1);
        }
        exiting.join();
    }

    // exit during a long build stops within one document
    void test_worker_exit_mid_build() {
        WorkerLog log;
        auto worker = make_worker(log);

        const auto job_id = worker->submit(sample_workspace(4), 5000);
        check(log.wait_started(job_id), "long build starts");

        QElapsedTimer timer;
        timer.start();
        worker.reset();
        check(timer.elapsed() < 500, "exit mid build stops within one document");
        check(!log.was_finished(job_id), "build cancelled by exit never reported");
    }

    // worker R1 tests

    // every build runs on the same worker thread
    void test_worker_one_thread() {
        WorkerLog log;
        auto worker = make_worker(log);
        const auto worker_id = worker->thread_id();

        bool all_finished = true;
        for (int i = 0; i < 3; i++) {
            if (!log.wait_finished(worker->submit(sample_workspace(1), 0))) all_finished = false;
        }
        check(all_finished, "worker finishes three builds");
        check(worker_id != std::this_thread::get_id(), "worker is its own thread");

        std::lock_guard<std::mutex> lock(log.mutex);
        bool same_thread = true;
        for (const auto& id : log.threads) {
            if (id != worker_id) same_thread = false;
        }
        check(same_thread, "every build runs on the one worker thread");
    }

    // worker R2 tests

    // replaced and cancelled requests are never built
    void test_worker_outdated_request_never_built() {
        WorkerLog log;
        auto worker = make_worker(log);

        const auto a = worker->submit(sample_workspace(2), 300);
        check(log.wait_started(a), "first build starts");
        const auto b = worker->submit(sample_workspace(2), 300);
        const auto c = worker->submit(sample_workspace(2), 0);

        check(log.wait_finished(c), "newest request is built");
        check(!log.was_finished(a), "cancelled running build never reported");
        check(!log.was_finished(b), "replaced request never built");
    }

    // worker R3 tests

    // the next build doesn't wait for the cancelled one
    void test_worker_cancel_does_not_delay_next_build() {
        WorkerLog log;
        auto worker = make_worker(log);

        // 2 seconds if it is not cancelled
        const auto slow = worker->submit(sample_workspace(4), 500);
        check(log.wait_started(slow), "slow build starts");

        QElapsedTimer timer;
        timer.start();
        const auto next = worker->submit(sample_workspace(1), 0);
        check(log.wait_finished(next), "next build finishes");
        check(timer.elapsed() < 400, "next build does not wait for the cancelled one");
    }

    // MainWindow tests
    // same requirements as above but through the window
    // so the worker is checked the way the user reaches it

    // destroys the window on this thread while a watchdog waits
    // a stuck join can't be undone so the watchdog ends the run
    void destroy_or_fail(std::unique_ptr<MainWindow>& window, const char* name) {
        std::mutex mutex;
        std::condition_variable changed;
        bool done = false;

        std::thread watchdog([&] {
            std::unique_lock<std::mutex> lock(mutex);
            if (!changed.wait_for(lock, std::chrono::seconds(2), [&] { return done; })) {
                std::cerr << "Fail: " << name << '\n';
                std::_Exit(1);
            }
        });

        window.reset();
        {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
        }
        changed.notify_one();
        watchdog.join();
    }

    // window R4 tests

    // exit with no build running must not hang
    void test_window_exit_when_idle() {
        auto window = std::make_unique<MainWindow>();
        window->add_document_file(path("doc_ai.txt"));
        button(*window, "Build Corpus")->click();
        check(wait_for_status(*window, "Corpus: 1 chunks"), "build finishes before idle exit");

        // give the worker time to go back to waiting
        // so the window's destructor has to wake it up
        QTest::qWait(100);
        destroy_or_fail(window, "idle window exits");
    }

    // window R3 tests

    // reset during a long build, the next build doesn't wait for it
    void test_rebuild_after_reset_does_not_wait() {
        MainWindow window;
        window.set_build_delay_ms(500);
        window.add_document_file(path("doc_ai.txt"));
        window.add_document_file(path("doc_retrieval.txt"));
        window.add_document_file(path("doc_systems.txt"));
        button(window, "Build Corpus")->click();
        // let the worker start the long build
        QTest::qWait(100);

        button(window, "Reset")->click();
        window.add_document_file(path("doc_ai.txt"));
        window.set_build_delay_ms(0);

        QElapsedTimer timer;
        timer.start();
        button(window, "Build Corpus")->click();
        check(wait_for_status(window, "Corpus: 1 chunks"), "build after reset finishes");
        check(timer.elapsed() < 400, "build after reset does not wait for the cancelled one");
    }

    // add document during a long build, the next build doesn't wait for it
    void test_rebuild_after_add_does_not_wait() {
        MainWindow window;
        window.set_build_delay_ms(500);
        window.add_document_file(path("doc_ai.txt"));
        window.add_document_file(path("doc_retrieval.txt"));
        button(window, "Build Corpus")->click();
        // let the worker start the long build
        QTest::qWait(100);

        window.add_document_file(path("doc_systems.txt"));
        window.set_build_delay_ms(0);

        QElapsedTimer timer;
        timer.start();
        button(window, "Build Corpus")->click();
        check(wait_for_status(window, "Corpus: 3 chunks"), "build after add finishes");
        check(timer.elapsed() < 400, "build after add does not wait for the cancelled one");
    }

    // remote add during a long build works like add document
    // the running build is cancelled and the user sees Not built
    void test_rebuild_after_remote_add_does_not_wait() {
        auto socket = test_socket("remote");
        MainWindow window;
        window.start_ipc(socket);
        window.set_build_delay_ms(500);
        window.add_document_file(path("doc_ai.txt"));
        window.add_document_file(path("doc_retrieval.txt"));
        button(window, "Build Corpus")->click();
        // let the worker start the long build
        QTest::qWait(100);

        int fd = ipc_connect(socket);
        check(fd >= 0, "ipc connect during build");
        std::string file = QFileInfo(path("doc_systems.txt")).absoluteFilePath().toStdString();
        uint8_t status = ipc_send_add(fd, file);
        close(fd);

        check(status == 0, "remote add during build accepted");
        check(wait_for_docs(window, 3), "remote add during build listed");
        check(corpus_status(window) == QStringLiteral("Corpus: Not built"), "remote add invalidates during build");

        window.set_build_delay_ms(0);
        QElapsedTimer timer;
        timer.start();
        button(window, "Build Corpus")->click();
        check(wait_for_status(window, "Corpus: 3 chunks"), "build after remote add finishes");
        check(timer.elapsed() < 400, "build after remote add does not wait for the cancelled one");
    }

}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    test_worker_exit_when_idle();
    test_worker_exit_mid_build();
    test_worker_one_thread();
    test_worker_outdated_request_never_built();
    test_worker_cancel_does_not_delay_next_build();

    test_window_exit_when_idle();
    test_rebuild_after_reset_does_not_wait();
    test_rebuild_after_add_does_not_wait();
    test_rebuild_after_remote_add_does_not_wait();

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

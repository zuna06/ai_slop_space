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

#include <cstddef>
#include <iostream>

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
        check(corpus_status(window) == QStringLiteral("Corpus: Not Built"), "add document invalidates during build");

        check(wait_for(finished, 1), "stale build finishes");
        check(corpus_status(window) == QStringLiteral("Corpus: Not Built"), "stale build discarded after add document");
        check(!button(window, "Inspect")->isEnabled(), "inspect stays disabled after stale build");
        check(button(window, "Build Corpus")->isEnabled(), "build re enabled after add");
    }

    void test_reset_during_build() {
        MMainWindow window;
        window.set_build_delay_ms(20);
        window.add_document_file(path("doc_ai.txt"));
        QSignalSpy finished(window.findChild<CorpusBuilder*>(), &CorpusBuilder::build_finished);

        button(window, "Build Corpus")->click();
        button(window, "Reset")->click();
        check(wait_for(finished, 1), "stale build finishes after reset");
        check(corpus_status(window) == QStringLiteral("Corpus: Not Built"), "stale build discarded after reset");
        check(!button(window, "Inspect")->isEnabled(), "inspect disabled after reset");
    }
}
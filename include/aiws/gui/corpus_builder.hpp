#pragma once

#include <QObject>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <future>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "aiws/processing_core.hpp"
#include "aiws/workspace.hpp"

namespace aiws::gui {

    // Stores the result of one finished build
    // Handed to the gui thread by CorpusBuilder::take()
    struct BuildResult {
        std::uint64_t workspace_id{};
        // similar to Rust's Option type
        // stores the built core if the build succeeded
        std::optional<ProcessingCore> core;
        // store the exception if the build throws
        std::exception_ptr error;
    };

    // Runs corpus builds on the background using threads
    // Owns all of them
    // All builds get their own thread, copy of the Workspace,
    // and ProcessingCore
    // When a build finishes, its worker posts on_job_done() to
    // the gui thread which calls build_finished(id)
    // Then take is called to get the result and the destructor
    // cancels and joins every build that is still running
    class CorpusBuilder final : public QObject {
        Q_OBJECT

        public:
            CorpusBuilder(QObject* parent);
            ~CorpusBuilder();

            // delete copy constructors
            // CorpusBuilder shouledd not be allowed to be copied
            // due to thread ownership and state handling
            // and it is unnecessary
            CorpusBuilder(const CorpusBuilder&) = delete;
            CorpusBuilder& operator=(const CorpusBuilder&) = delete;

    };

}
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
    // When a build finishes, its worker posts job_done() to
    // the gui thread which emits build_finished(id) signal
    // Then take is called to get the result and the destructor
    // cancels and joins every build that is still running
    class CorpusBuilder final : public QObject {
        Q_OBJECT

        public:
            // explicit prevents a QObject from converting
            // into a CorpusBuilder, edge case
            explicit CorpusBuilder(QObject* parent);
            ~CorpusBuilder() override;

            // delete copy constructors
            // CorpusBuilder shouledd not be allowed to be copied
            // due to thread ownership and state handling
            // and it is unnecessary
            CorpusBuilder(const CorpusBuilder&) = delete;
            CorpusBuilder& operator=(const CorpusBuilder&) = delete;

            // using quint64 because it the job id gets passed to Qt's
            // queued call
            // best practice to pass lib types if using its api
            // other values use standard lib types

            // starts a build, gets copy of workspace, id, and delay_ms
            // returns the id of the new build
            quint64 start(Workspace workspace, std::uint64_t workspace_id, int delay_ms);
            
            // takes a finished build and joins its thread, and moves
            // its result to a BuildResult object and returns it to
            // caller
            BuildResult take(quint64 job_id);

            // number of builds started that have not
            // been joined yet (take function)
            size_t job_count() const noexcept;

        signals:
            // signal for gui thread for when a build has finished
            void build_finished(quint64 job_id);
        
        private slots:
            // from worker thread when its build is done
            void job_done(quint64 job_id);

        private:
            // internal struct for the build jobs state and info
            // kept private since it's only used within this class
            // result is set by worker on build completetion
            // and read by gui after job_done()
            struct BuildJob {
                quint64 id{};
                std::uint64_t workspace_id{};
                std::atomic<bool> cancel{false};
                std::future<ProcessingCore> result;
                std::thread worker;
            };

            // stores unique pointers to all build jobs
            // owned by this class
            // in vector for resizing and access
            std::vector<std::unique_ptr<BuildJob>> jobs_;
            quint64 next_id{1};
    };

}
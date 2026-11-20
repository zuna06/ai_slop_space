#pragma once

#include "aiws/cancel_token.hpp"
#include "aiws/processing_core.hpp"
#include "aiws/workspace.hpp"

#include <condition_variable>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace aiws {

    // build result that was not cancelled
    struct BuildOutcome {
        std::uint64_t ticket{};
        std::optional<ProcessingCore> core;
        std::exception_ptr error;
    };

    // owns the single build thread
    // gui thread calls submit, cancel, or stop never waits for a build
    // New submits replace old submits, outdated builds are never built
    class BuildWorker final {
        public:
            using FinishedCallback = std::function<void(BuildOutcome)>;
            using StartedCallback = std::function<void(std::uint64_t ticket)>;

            explicit BuildWorker(FinishedCallback on_finished, StartedCallback on_started = {});

            ~BuildWorker();

            // delelte copy constructions
            BuildWorker(const BuildWorker&) = delete;
            BuildWorker& operator=(const BuildWorker&) = delete;

            // gui thread queues a build and passes workspace by value
            // replaces any pending build request and cancels the current
            // build
            std::uint64_t submit(Workspace workspace, int delay_ms);

            // called by gui thread
            void cancel();
            
            // called by gui thread
            // cancels everything, wakes the worker, and joins it
            void stop();

            std::thread::id thread_id() const noexcept;

        private:
            struct Request {
                std::uint64_t ticket{};
                Workspace workspace;
                int delay_ms{};
            };

            // worker thread loop
            void run();

            // caller holds mutex_
            void cancel_running_locked();

            // sets before thread starts
            // and remains read only after
            FinishedCallback on_finished_;
            StartedCallback on_started_;

            // shared between gui and worker threads
            // guarded by mutex
            std::mutex mutex_;
            std::condition_variable wake_;
            std::optional<Request> pending_;
            std::shared_ptr<CancelToken> running_;

            bool stopping_{ false };

            // gui thread only
            std::uint64_t next_ticket_{ 0 };

            // starts after every member exists
            std::thread thread_;
    };

}
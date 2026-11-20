#include <aiws/build_worker.hpp>

#include <aiws/chunker.hpp>
#include <aiws/context_builder.hpp>
#include <aiws/delaying_chunker.hpp>
#include <aiws/retrieval_engine.hpp>

#include <utility>

namespace aiws {

    namespace {
        BuildResult run_build(std::uint64_t job_id, const Workspace& workspace, int delay_ms, std::shared_ptr<const CancelToken> token) {
            BuildResult result;
            result.job_id = job_id;
            
            try {
                auto chunker = std::make_unique<DelayingChunker>(
                    std::make_unique<Chunker>( ChunkingPolicy {
                        ProcessingCore::kMaxChunkTokens,
                        ProcessingCore::kChunkOverlap,
                        ProcessingCore::kParagraphPreferenceWindow }),
                        delay_ms, std::move(token));
                
                ProcessingCore core(std::move(chunker),
                                    std::make_unique<RetrievalEngine>(),
                                    std::make_unique<ContextBuilder>());
                core.rebuild(workspace);
                result.core.emplace(std::move(core));
            } catch (...) {
                result.error = std::current_exception();
            }

            return result;
        }
    }

    BuildWorker::BuildWorker(FinishedCallback on_finished, StartedCallback on_started)
    :   on_finished_(std::move(on_finished)),
        on_started_(std::move(on_started)),
        thread_(&BuildWorker::run, this) {}

    BuildWorker::~BuildWorker() {
        stop();
    }

    std::uint64_t BuildWorker::submit(Workspace workspace, int delay_ms) {
        std::uint64_t job_id = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            job_id = ++next_job_id_;
            // replaces older pending request
            pending_ = Request{job_id, std::move(workspace), delay_ms < 0 ? 0 : delay_ms};

            cancel_running_locked();
        }

        // running build is useless
        // stop so new build doesn't wait
        wake_.notify_one();

        return job_id;
    }

    void BuildWorker::cancel() {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.reset();
        cancel_running_locked();
    }

    void BuildWorker::stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
            pending_.reset();
            if (running_) running_->cancel();
        }

        #ifndef AIWS_FAULT_SILENT_SHUTDOWN
        wake_.notify_one();
        #endif
        if (thread_.joinable()) thread_.join();
    }

    std::thread::id BuildWorker::thread_id() const noexcept {
        return thread_.get_id();
    }

    void BuildWorker::cancel_running_locked() {
        #ifndef AIWS_FAULT_NO_CANCEL
        if (running_) running_->cancel();
        #endif
    }

    void BuildWorker::run() {
        while (true) {
            Request request;
            auto token = std::make_shared<CancelToken>();
            {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait(lock, [this] {return stopping_ || pending_.has_value(); });
                
                if (stopping_) return;

                request = std::move(*pending_);
                pending_.reset();
                running_ = token;
            }

            if (on_started_) on_started_(request.job_id);

            BuildResult result = run_build(request.job_id, request.workspace, request.delay_ms, token);

            {
                std::lock_guard<std::mutex> lock(mutex_);
                running_.reset();
            }

            if (!token->cancelled() && on_finished_) {
                on_finished_(std::move(result));
            }
        }
    }

}
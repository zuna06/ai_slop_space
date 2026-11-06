#include "aiws/gui/corpus_builder.hpp"

#include "aiws/chunker.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/delaying_chunker.hpp"
#include "aiws/retrieval_engine.hpp"

#include <QMetaObject>

#include <stdexcept>
#include <utility>

namespace aiws::gui {

    namespace {
        // function for worker thread
        // Builds a ProcessingCore using the Workspace copy
        // and stores the result or exception depending on
        // if the build succeeds or fails, then signals the gui
        // thread
        void run_build(
            Workspace workspace,
            int delay_ms,
            const std::atomic<bool>& cancel,
            std::promise<ProcessingCore> promise,
            CorpusBuilder* builder,
            quint64 job_id
        ) {
            try {
                auto chunker = std::make_unique<Chunker>(ChunkingPolicy {
                    ProcessingCore::kMaxChunkTokens,
                    ProcessingCore::kChunkOverlap,
                    ProcessingCore::kParagraphPreferenceWindow
                });

                ProcessingCore core(
                    std::make_unique<DelayingChunker>(std::move(chunker), delay_ms, cancel),
                    std::make_unique<RetrievalEngine>(),
                    std::make_unique<ContextBuilder>()
                );

                core.rebuild(workspace);
                promise.set_value(std::move(core));
            } catch (...) {
                // catch all exceptions
                promise.set_exception(std::current_exception());
            }

            QMetaObject::invokeMethod(builder, "job_done", Qt::QueuedConnection, Q_ARG(quint64, job_id));
        }
    }

    CorpusBuilder::CorpusBuilder(QObject* parent) : QObject(parent) {}

    // cancels all jobs first since join() blocks
    // then check if the worker is joinable() in a separate for loop
    // and join() if true
    CorpusBuilder::~CorpusBuilder() {
        for (auto& job : jobs_) {
            job->cancel = true;
        }
        for (auto& job : jobs_) {
            if (job->worker.joinable()) {
                job->worker.join();
            }
        }
    }

    quint64 CorpusBuilder::start(Workspace workspace, std::uint64_t workspace_id, int delay_ms) {
        // create a unique ptr to a new BuildJob
        // and populate it
        auto job = std::make_unique<BuildJob>();
        job->id = next_id++;
        job->workspace_id = workspace_id;

        std::promise<ProcessingCore> promise;

        job->result = promise.get_future();

        // std::thread does not pass arguments by reference by itself
        // passing cancel is copied into the thread's storage
        // but cancel needs to be shared across functions
        // std::cref wraps the cancel reference into a
        // std::reference_wrapper object that the thread can copy
        // but this time when the object is unwrapped and used 
        // the reference will point back to the original BuildJob
        // cancel
        // std::cref vs std::ref
        // using cref because it is read only since the worker only 
        // needs to see the value of cancel
        job->worker = std::thread(
            run_build, 
            std::move(workspace), 
            delay_ms,
            std::cref(job->cancel),
            std::move(promise),
            this,
            job->id
        );

        // push the constructed BuildJob to the jobs_ vector
        // and return the id 
        const quint64 id = job->id;
        jobs_.push_back(std::move(job));
        return id;
    }
 
    BuildResult CorpusBuilder::take(quint64 job_id) {
        // use iterator ptr to find job_id
        // linear search
        auto it = jobs_.begin();
        while (it != jobs_.end()) {
            if ((*it)->id == job_id) {
                break;
            }

            it++;
        }

        // iterator validity check
        if (it == jobs_.end()) throw std::out_of_range("unknown build id");

        // get ref, auto -> copies
        BuildJob& job = (**it);
        job.worker.join();

        BuildResult result;
        result.workspace_id = job.workspace_id;

        // check result of build and get
        // core or set error inside result
        try {
            result.core.emplace(job.result.get());
        } catch (...) {
            result.error = std::current_exception();
        }

        jobs_.erase(it);
        return result;
    }

    size_t CorpusBuilder::job_count() const noexcept {
        return jobs_.size();
    }

    // emits build_finished signal
    void CorpusBuilder::job_done(quint64 job_id) {
        emit build_finished(job_id);
    }

}
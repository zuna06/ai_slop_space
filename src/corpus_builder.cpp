#include "aiws/gui/corpus_builder.hpp"

#include "aiws/chunker.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/delaying_chunker.hpp"
#include "aiws/retrieval_engine.hpp"

#include <QMetaObject>

#include <stdexcept>
#include <utility>

namespace aiws::gui {

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
        CorpusBuilder* builder    
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
}
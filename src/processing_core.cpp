#include "aiws/processing_core.hpp"

#include "aiws/text_processor.hpp"
#include "aiws/chunker.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/corpus_index.hpp"
#include "aiws/retrieval_engine.hpp"
#include <unordered_set>
#include <stdexcept>

namespace aiws {

struct ProcessingCore::Impl {
    Chunker chunker { 
        ChunkingPolicy {
            kMaxChunkTokens,
            kChunkOverlap,
            kParagraphPreferenceWindow
        }
    };
    CorpusIndex index;
    RetrievalEngine retrieval;
    ContextBuilder context_builder;
    std::vector<Chunk> chunks;
};

ProcessingCore::ProcessingCore() : impl_(std::make_unique<Impl>()) { }

ProcessingCore::~ProcessingCore() = default;

ProcessingCore::ProcessingCore(ProcessingCore&&) noexcept = default;

ProcessingCore& ProcessingCore::operator=(ProcessingCore&&) noexcept = default;

std::string ProcessingCore::normalize(const std::string& text) {
    return TextProcessor::normalize(text);
}

void ProcessingCore::rebuild(const Workspace& workspace) {
    std::unordered_set<std::string> ids;
    for (const auto& doc : workspace.documents()) {
        if (!ids.insert(doc.id()).second) {
            throw std::invalid_argument("duplicate document id");
        }
    }

    std::vector<Chunk> chunks;
    size_t order = 0;

    // the jonkler
    for (const auto& doc : workspace.documents()) {
        auto doc_chunks = impl_->chunker.chunk(doc, order);
        
        for (auto& chunk : doc_chunks) {
            chunks.push_back(chunk);
        }

        order++;
    }

    CorpusIndex index(chunks);

    // wow how did i forget move exists
    impl_->chunks = std::move(chunks);
    impl_->index = std::move(index);
}

const std::vector<Chunk>& ProcessingCore::chunks() const noexcept {
    return impl_->chunks;
}

std::size_t ProcessingCore::chunk_count() const noexcept {
    return impl_->chunks.size();
}

std::size_t ProcessingCore::document_frequency(const std::string& term) const {
    std::vector<std::string> normalized = TextProcessor::terms(term);
    if (normalized.empty()) return 0;
    if (normalized.size() > 1) {
        throw std::invalid_argument("must be single token");
    }

    return impl_->index.document_frequency(normalized[0]);
}

std::size_t ProcessingCore::term_frequency(const std::string& term,
                                           const std::string& chunk_id) const {
    std::vector<std::string> normalized = TextProcessor::terms(term);
    if (normalized.empty()) return 0;
    if (normalized.size() > 1) {
        throw std::invalid_argument("must be single token");
    }

    return impl_->index.term_frequency(normalized[0], chunk_id);
}

std::vector<SearchResult> ProcessingCore::search(const std::string& query, int num) const {
    return impl_->retrieval.search(query, num, impl_->chunks, impl_->index);
}

std::vector<ContextItem> ProcessingCore::build_context(const std::string& query,
                                                       int num,
                                                       std::size_t max_tokens) const {
    auto results = search(query, num);
    return impl_->context_builder.build(results, max_tokens);
}

}  // namespace aiws

#include "aiws/delaying_chunker.hpp"


#include <chrono>
#include <stdexcept>
#include <utility>

namespace aiws {

    DelayingChunker::DelayingChunker(std::unique_ptr<ChunkingStrategy> inner, int delay_ms, std::shared_ptr<const CancelToken> cancel)
    :   inner_(std::move(inner)),
        delay_ms_(delay_ms < 0 ? 0 : delay_ms),
        cancel_(std::move(cancel))
    {

        if (!inner_) throw std::invalid_argument("DelayingChunker needs an inner strategy");
        if (!cancel_) throw std::invalid_argument("DelayingChunker needs a cancel token");
    }

    // waits on token's condition var instead of sleeping 10 ms slices
    // this is so that cancel() wakes it instantly with 0 delay
    std::vector<Chunk> DelayingChunker::chunk(const Document& doc, size_t doc_order) const {
        if (cancel_->wait_for(std::chrono::milliseconds(delay_ms_))) throw BuildCancelled();
        return inner_->chunk(doc, doc_order);
    }
    
}

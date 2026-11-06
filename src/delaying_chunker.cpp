#include "aiws/delaying_chunker.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <utility>

namespace aiws {

    DelayingChunker::DelayingChunker(std::unique_ptr<ChunkingStrategy> inner, int delay_ms, const std::atomic<bool>& cancel) 
    :   inner_(std::move(inner)),
        delay_ms_(delay_ms < 0 ? 0 : delay_ms),
        cancel_(cancel)
    {

        if (!inner_) throw std::invalid_argument("DelayingChunker needs an inner strategy");
    }

    // using 10 ms so that the shutdown feels instead
    // low overhead
    std::vector<Chunk> DelayingChunker::chunk(const Document& doc, size_t doc_order) const {
        for (int i = delay_ms_; i > 0; i -= 10) {
            if (cancel_) throw std::runtime_error("build cacnaled");
            std::this_thread::sleep_for(std::chrono::milliseconds(std::min(i, 10)));
        }
        
        if (cancel_) throw std::runtime_error("build cancelled");
        return inner_->chunk(doc, doc_order);
    }
}
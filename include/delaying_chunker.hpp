#pragma once

#include "aiws/chunking_strategy.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

namespace aiws {

    class DelayingChunker final : public ChunkingStrategy {
        public:
            DelayingChunker(std::unique_ptr<ChunkingStrategy> inner,
                            int delay_ms,
                            const std::atomic<bool>& cancel);

            std::vector<Chunk> chunk(const Document& doc, size_t doc_order) const override;

        private:
            std::unique_ptr<ChunkingStrategy> inner_;
            int delay_ms_;
            const std::atomic<bool>& cancel;
    }
}
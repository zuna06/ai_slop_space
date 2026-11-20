#pragma once

#include "aiws/chunking_strategy.hpp"
#include "aiws/cancel_token.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

namespace aiws {

    class DelayingChunker final : public ChunkingStrategy {
        public:
            DelayingChunker(std::unique_ptr<ChunkingStrategy> inner,
                            int delay_ms,
                            std::shared_ptr<const CancelToken> cancel);

            std::vector<Chunk> chunk(const Document& doc, size_t doc_order) const override;

        private:
            std::unique_ptr<ChunkingStrategy> inner_;
            int delay_ms_;
            std::shared_ptr<const CancelToken> cancel_;
    };
}

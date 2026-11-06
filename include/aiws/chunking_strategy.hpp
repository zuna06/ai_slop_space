#pragma once

#include "aiws/document.hpp"
#include "aiws/processing_types.hpp"

#include <cstddef>
#include <vector>

namespace aiws {

class ChunkingStrategy {
public:
    virtual ~ChunkingStrategy() = default;
    virtual std::vector<Chunk> chunk(const Document& document,
                                     size_t document_order) const = 0;
};

}  // namespace aiws

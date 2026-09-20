#include "aiws/chunker.hpp"

#include <stdexcept>

namespace aiws {

Chunker::Chunker(ChunkingPolicy policy) : policy_(policy) {
    if (policy_.max_tokens == 0 || policy_.overlap >= policy_.max_tokens ||
        policy_.paragraph_window > policy_.max_tokens) {
        throw std::invalid_argument("invalid chunking policy");
    }
}

std::vector<Chunk> Chunker::chunk(const Document& document, std::size_t document_order) const {
    std::vector<Chunk> chunks;

    std::vector<TokenInfo> tokens = TextProcessor::tokenize(document.text());

    // empty check
    if (tokens.empty()) return chunks;

    size_t start = 0;
    size_t sequence = 0;

    while (start < tokens.size()) {
        size_t remaining = tokens.size() - start;
        size_t end;

        // last chunk
        if (remaining <= policy_.max_tokens) {
            end = tokens.size();
        } else {
            end = start + policy_.max_tokens;

            size_t begin = end - policy_.paragraph_window;

            for (size_t i = end; i > begin; i--) {
                if (tokens[i].paragraph != tokens[i - 1].paragraph) {
                    end = i;
                    break;
                }
            }
        }

        Chunk chunk;
        chunk.id = document.id() + "#" + std::to_string(sequence);
        chunk.document_id = document.id();
        chunk.document_order = document_order;
        chunk.sequence = sequence;
        chunk.text = TextProcessor::join(tokens, start, end);
        chunk.token_count = end - start;
        chunk.source_begin = tokens[start].begin;
        chunk.source_end = tokens[end - 1].end;

        chunks.push_back(chunk);

        sequence++;

        if (end >= tokens.size()) break;

        start = end - policy_.overlap;
    }

    return chunks;
}

}  // namespace aiws

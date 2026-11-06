#pragma once

#include <cstddef>
#include <string>

namespace aiws {

struct Chunk {
    std::string id;
    std::string document_id;
    size_t document_order{};
    size_t sequence{};
    std::string text;
    size_t token_count{};
    size_t source_begin{};
    size_t source_end{};
};

struct SearchResult {
    std::string chunk_id;
    std::string document_id;
    size_t chunk_sequence{};
    std::string text;
    double score{};
    size_t matched_terms{};
};

struct ContextItem {
    std::string chunk_id;
    std::string document_id;
    size_t chunk_sequence{};
    std::string text;
    size_t token_count{};
    double score{};
    bool truncated{};
};

}  // namespace aiws

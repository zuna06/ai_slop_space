#pragma once

#include "aiws/processing_types.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace aiws {

class CorpusIndex {
public:
    struct Posting {
        size_t chunk_index{};
        size_t frequency{};
    };

    CorpusIndex() = default;
    explicit CorpusIndex(const std::vector<Chunk>& chunks);

    void build(const std::vector<Chunk>& chunks);
    size_t document_frequency(const std::string& normalized_term) const noexcept;
    size_t term_frequency(const std::string& normalized_term,
                               const std::string& chunk_id) const noexcept;
    const std::vector<Posting>* postings(const std::string& normalized_term) const noexcept;
    const Chunk* find_chunk(const std::vector<Chunk>& chunks,
                            const std::string& chunk_id) const noexcept;
    size_t chunk_index(const std::string& chunk_id) const;

private:
    std::unordered_map<std::string, std::vector<Posting>> postings_;
    std::unordered_map<std::string, size_t> chunk_by_id_;
};

}  // namespace aiws

#include "aiws/corpus_index.hpp"

namespace aiws {

CorpusIndex::CorpusIndex(const std::vector<Chunk>& chunks) {
    build(chunks);
}

void CorpusIndex::build(const std::vector<Chunk>& chunks) {
    std::unordered_map<std::string, std::vector<Posting>> postings;
    std::unordered_map<std::string, size_t> chunk_by_id;

    for (size_t i = 0; i < chunks.size(); i++) {
        auto& chunk = chunks[i];
        chunk_by_id[chunk.id] = i;

        // check freq of term for this chunk
        std::unordered_map<std::string, size_t> freq;
        std::vector<std::string> terms = TextProcessor::terms(chunk.text);

        for (auto& term : terms) {
            freq[term]++;
        }

        // add a posting for each unique term
        for (auto& pair : freq) {
            postings[pair.first].push_back({i, pair.second});
        }
    }

    postings_ = postings;
    chunk_by_id_ = chunk_by_id;
}

std::size_t CorpusIndex::document_frequency(
    const std::string& normalized_term) const noexcept {
    // iterator
    auto it = postings_.find(normalized_term);
    if (it == postings_.end()) return 0;
    return it->second.size();
}

std::size_t CorpusIndex::term_frequency(
    const std::string& normalized_term,
    const std::string& chunk_id) const noexcept {
    // iterator
    auto postings_it = postings_.find(normalized_term);
    if (postings_it == postings_.end()) return 0;
    
    auto chunk_it = chunk_by_id_.find(chunk_id);
    if (chunk_it == chunk_by_id_.end()) return 0;

    size_t target = chunk_it->second;
    for (const auto& posting : postings_it->second) {
        if (posting.chunk_index == target) {
            return posting.frequency;
        }
    }

    return 0;
}

const std::vector<CorpusIndex::Posting>* CorpusIndex::postings(
    const std::string& normalized_term) const noexcept {
    auto it = postings_.find(normalized_term);
    if (it == postings_.end()) return nullptr;
    return &it->second;
}

const Chunk* CorpusIndex::find_chunk(
    const std::vector<Chunk>& chunks,
    const std::string& chunk_id) const noexcept {
    auto it = chunk_by_id_.find(chunk_id);
    if (it == chunk_by_id_.end()) return nullptr;
    if (it->second >= chunks.size()) return nullptr;
    return &chunks[it->second];
}

std::size_t CorpusIndex::chunk_index(const std::string& chunk_id) const {
    auto it = chunk_by_id_.find(chunk_id);
    if (it == chunk_by_id_.end()) {
        throw std::out_of_range("invalid chunk id");
    }

    return it->second;
}

}  // namespace aiws

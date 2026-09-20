#include "aiws/retrieval_engine.hpp"

namespace aiws {

double RetrievalEngine::canonical_score(double value) {
    // round for 12 digits
    double factor = 1e12;
    return std::round(value * factor) / factor;
}

std::vector<SearchResult> RetrievalEngine::search(const std::string& query,
                                                  int k,
                                                  const std::vector<Chunk>& chunks,
                                                  const CorpusIndex& index) const {
    // check input
    if (k < 0) throw std::invalid_argument("k should be positive");
    if (k == 0 || chunks.empty()) return {};

    // get normalized terms vec
    std::vector<std::string> terms = TextProcessor::terms(query);
    if (terms.empty()) return {};
    
    std::vector<std::string> query_terms;
    std::unordered_set<std::string> seen_terms;
    for(auto& term : terms) {
        if (seen_terms.insert(term).second) {
            query_terms.push_back(term);
        }
    }

    size_t query_size = query_terms.size();
    size_t chunks_size = chunks.size();

    std::unordered_set<size_t> chunk_indices;
    for (const auto& term : query_terms) {
        const auto* posts = index.postings(term);
        if (posts) {
            for (const auto& post : *posts) {
                chunk_indices.insert(post.chunk_index);
            }
        }
    }

    // scoring
    std::vector<SearchResult> results;
    for (size_t ci : chunk_indices) {
        const Chunk& chunk = chunks[ci];
        double base = 0;
        size_t matched = 0;

        for (const auto& term : query_terms) {
            size_t term_freq_temp = index.term_frequency(term, chunk.id);
            if (term_freq_temp == 0) continue;
            matched++;

            double term_freq = 1 + std::log(static_cast<double>(term_freq_temp));
            size_t doc_freq = index.document_frequency(term);
            double idf = std::log((static_cast<double>(chunks_size) + 1.0) /
                                  (static_cast<double>(doc_freq) + 1.0)) + 1.0;
            base += term_freq * idf;
        }

        double coverage = 1 + 0.1 * static_cast<double>(matched) / static_cast<double>(query_size);
        double score = canonical_score(base * coverage);

        SearchResult result;
        result.chunk_id = chunk.id;
        result.document_id = chunk.document_id;
        result.chunk_sequence = chunk.sequence;
        result.text = chunk.text;
        result.score = score;
        result.matched_terms = matched;
        results.push_back(result);
    }

    // i luv sort and lambdas ^_^
    auto sort_lambda = [&chunks, &index](const SearchResult& a, const SearchResult& b) {
        if (a.score != b.score) return a.score > b.score;

        size_t a_idx = index.chunk_index(a.chunk_id);
        size_t a_order = chunks[a_idx].document_order;
        size_t b_idx = index.chunk_index(b.chunk_id);
        size_t b_order = chunks[b_idx].document_order;

        if (a_order != b_order) return a_order < b_order;
        return a.chunk_sequence < b.chunk_sequence;
    };

    std::sort(results.begin(), results.end(), sort_lambda);

    // resize if results are too long
    if (results.size() > static_cast<size_t>(k)) {
        results.resize(k);
    }

    return results;
}

}  // namespace aiws


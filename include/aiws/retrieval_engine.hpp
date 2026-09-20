#pragma once

#include "aiws/corpus_index.hpp"
#include "aiws/processing_types.hpp"
#include "aiws/text_processor.hpp"

#include <string>
#include <vector>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>

namespace aiws {

class RetrievalEngine {
public:
    std::vector<SearchResult> search(const std::string& query,
                                     int k,
                                     const std::vector<Chunk>& chunks,
                                     const CorpusIndex& index) const;

private:
    static double canonical_score(double value);
};

}  // namespace aiws

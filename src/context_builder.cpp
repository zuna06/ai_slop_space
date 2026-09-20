#include "aiws/context_builder.hpp"

namespace aiws {

std::vector<ContextItem> ContextBuilder::build(const std::vector<SearchResult>& ranked, std::size_t token_budget) const {
    std::vector<ContextItem> items;
    if (token_budget == 0) return items;

    size_t remaining = token_budget;
    for (const auto& result : ranked) {
        if (remaining == 0) break;

        std::vector<std::string> tokens = TextProcessor::terms(result.text);
        size_t token_count = tokens.size();

        ContextItem item;

        item.chunk_id = result.chunk_id;
        item.document_id = result.document_id;
        item.chunk_sequence = result.chunk_sequence;
        item.score = result.score;

        if (token_count <= remaining) {
            item.text = result.text;
            item.token_count = token_count;
            item.truncated = false;
            remaining -= token_count;
        } else {
            item.text = TextProcessor::join(tokens, 0, remaining);
            item.token_count = remaining;
            item.truncated = true;
            remaining = 0;
        }
        
        items.push_back(item);
    }

    return items;
}

}  // namespace aiws

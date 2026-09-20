#include "aiws/text_processor.hpp"

// anonymous namespace for helper functions
namespace {

    size_t check_paragraph(const std::string& text, size_t position) {
        size_t text_size = text.size();
        size_t i = position;

        // check bounds
        if (i >= text_size) return 0;

        // checking for first newline
        if (text[i] == '\r' && i + 1 < text_size && text[i + 1] == '\n') {
            i += 2;
        } else if (text[i] == '\n') {
            i += 1;
        } else {
            return 0;
        }

        // ignore all spaces and tabs
        while (i < text_size && (text[i] == ' ' || text[i] == '\t')) {
            i++;
        }
        
        // check bounds
        if (i >= text_size) return 0;
        
        // checking if a second newline exists
        if (text[i] == '\r' && i + 1 < text_size && text[i + 1] == '\n') {
            i += 2;
        } else if (text[i] == '\n') {
            i += 1;
        } else {
            return 0;
        }
        
        // return position after second newline if it exists
        return i - position;
    }

}

namespace aiws {

std::vector<TokenInfo> TextProcessor::tokenize(const std::string& text) {
    std::vector<TokenInfo> tokens;
    size_t paragraph = 0;
    size_t i = 0;
    size_t text_size = text.size();

    while (i < text_size) {
        
        // check paragraph
        size_t _index = check_paragraph(text, i);
        if (_index > 0) {
            paragraph++;
            i += _index;
            continue;
        }

        char chr = text[i];
        if (std::isalpha(chr) || std::isdigit(chr)) {
            size_t begin = i;
            std::string token;

            while (i < text_size && (std::isalpha(text[i]) || std::isdigit(text[i]))) {
                token.push_back(std::tolower(text[i]));
                i++;
            }

            tokens.push_back( {token, begin, i, paragraph} );
        } else {
            i++;
        }
    } 

    return tokens;
}

std::vector<std::string> TextProcessor::terms(const std::string& data) {
    std::vector<std::string> terms;
    std::string temp;

    size_t data_size = data.size();
    temp.reserve(data_size);
    
    for (size_t i = 0; i < data_size; i++) {
        char chr = std::tolower(data[i]);

        if (std::isalpha(chr) || std::isdigit(chr)) {
            temp.push_back(chr);
        } else {
            if (temp.size() > 0) {
                terms.push_back(temp);
                temp.clear();
            }
        }
    }

    if (!temp.empty()) terms.push_back(temp);

    return terms;
}

std::string TextProcessor::normalize(const std::string& data) {
    std::vector<std::string> terms = TextProcessor::terms(data);
    std::string normalized;
    
    normalized.reserve(data.size());
    for (size_t i = 0; i < terms.size(); i++) {
        normalized.append(terms[i]);
        if (i != terms.size() - 1) normalized.append(" ");
    }
    
    return normalized;
}

std::string TextProcessor::join(const std::vector<TokenInfo>& tokens,
                                std::size_t start,
                                std::size_t end) {
    std::string normalized;
    
    for (size_t i = start; i < end; i++) {
        if (i != start) normalized.append(" ");
        normalized.append(tokens[i].token);
    }
    
    return normalized;
}

std::string TextProcessor::join(const std::vector<std::string>& terms,
                                std::size_t start,
                                std::size_t end) {
    std::string normalized;
    
    for (size_t i = start; i < end; i++) {
        if (i != start) normalized.append(" ");
        normalized.append(terms[i]);
    }
    
    return normalized;
}

}  // namespace aiws

#include "Document.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

// consider std::move
Document::Document(std::string title, std::string contents) {
    this->title_ = title;
    this->contents_ = contents;
}

bool Document::operator==(const Document& other) const {
    return (this->title_ == other.title_
            && this->sourcePath_ == other.sourcePath_
            && this->contents_ == other.contents_);
}

bool Document::operator!=(const Document& other) const {
    return !(*this == other);
}

bool Document::load(const std::string& path) {
    std::filesystem::path fs_path = path;
    std::ifstream file(fs_path);
    std::ostringstream oss;

    // change this
    if (file.fail()) {
        // std::cerr << "Error: failed to open " << path << "\n";
        return false;
    }

    // read into oss
    oss << file.rdbuf();
    if (file.bad()) {
        // std::cerr << "Error: failed process file stream" << "\n";
        return false;
    }

    // set document members
    this->title_ = fs_path.filename().string();
    this->sourcePath_ = path;
    this->contents_ = oss.str();

    return true;
}

const std::string& Document::title() const noexcept {
    return title_;
}

const std::string& Document::sourcePath() const noexcept {
    return sourcePath_;
}

const std::string& Document::contents() const noexcept {
    return contents_;
}

void Document::setTitle(std::string title) {
    this->title_ = title;
}

std::size_t Document::characterCount() const noexcept {
    return contents_.size();
}

bool Document::empty() const noexcept {
    return contents_.empty();
}

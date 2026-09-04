#include "Prompt.hpp"

// consider std::move
Prompt::Prompt(std::string title, std::string text) 
:   title_(title), 
    text_(text)
{
    /*
    // uncomment if needed later
    // depends on parameter use case after object construction
    title_ = std::move(title);
    text_ = std::move(text);
    */
}

bool Prompt::operator==(const Prompt& other) const {
    return (this->title_ == other.title_ && this->text_ == other.text_);
}

bool Prompt::operator!=(const Prompt& other) const {
    return !(*this == other);
}

const std::string& Prompt::title() const noexcept {
    return title_;
}

const std::string& Prompt::text() const noexcept {
    return text_;
}

void Prompt::setTitle(std::string title) {
    this->title_ = title;
}

void Prompt::setText(std::string text) {
    this->text_ = text;
}

bool Prompt::empty() const noexcept {
    return text_.empty(); 
}

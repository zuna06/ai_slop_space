#include "Message.hpp"

// consider std::move
Message::Message(MessageRole role, std::string text) {
    this->role_ = role;
    this->text_ = text;
}

bool Message::operator==(const Message& other) const {
    return (this->role_ == other.role_ && this->text_ == other.text_);
}

bool Message::operator!=(const Message& other) const {
    return !(*this == other);
}

MessageRole Message::role() const noexcept {
    return role_;
}

const std::string& Message::text() const noexcept {
    return text_;
}

void Message::setRole(MessageRole role) noexcept {
    this->role_ = role;
}

void Message::setText(std::string text) {
    this->text_ = text;
}

bool Message::empty() const noexcept {
    return text_.empty();
}

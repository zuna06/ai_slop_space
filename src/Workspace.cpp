#include "Workspace.hpp"

// consider std::move
Workspace::Workspace(std::string name) {
    this->name_ = name;
}

bool Workspace::operator==(const Workspace& other) const {
    return (this->name_ == other.name_
            && this->documents_ == other.documents_
            && this->prompts_ == other.prompts_
            && this->messages_ == other.messages_);
}

bool Workspace::operator!=(const Workspace& other) const {
    return !(*this == other);
}

const std::string& Workspace::name() const noexcept {
    return this->name_;
}

void Workspace::setName(std::string name) {
    this->name_ = name;
}

void Workspace::addDocument(Document document) {
    this->documents_.push_back(document);
}

void Workspace::addPrompt(Prompt prompt) {
    this->prompts_.push_back(prompt);
}

void Workspace::addMessage(Message message) {
    this->messages_.push_back(message);
}

std::size_t Workspace::documentCount() const noexcept {
    return this->documents_.size();
}

std::size_t Workspace::promptCount() const noexcept {
    return this->prompts_.size();
}

std::size_t Workspace::messageCount() const noexcept {
    return this->messages_.size();
}

Document& Workspace::documentAt(std::size_t index) {
    return this->documents_.at(index);
}

const Document& Workspace::documentAt(std::size_t index) const {
    return this->documents_.at(index);
}

Prompt& Workspace::promptAt(std::size_t index) {
    return this->prompts_.at(index);
}

const Prompt& Workspace::promptAt(std::size_t index) const {
    return this->prompts_.at(index);
}

Message& Workspace::messageAt(std::size_t index) {
    return this->messages_.at(index);
}

const Message& Workspace::messageAt(std::size_t index) const {
    return this->messages_.at(index);
}

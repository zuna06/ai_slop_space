#include "aiws/gui/context_widget.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace aiws::gui {

ContextWidget::ContextWidget(QWidget* parent) : QWidget(parent) {
    auto* box = new QGroupBox(QStringLiteral("Context"), this);

    context_budget_slider_ = new QSlider(Qt::Horizontal, box);
    context_budget_slider_->setRange(50, 1000);
    context_budget_slider_->setSingleStep(50);
    context_budget_slider_->setPageStep(50);
    budget_value_label_ = new QLabel(box);
    build_context_button_ = new QPushButton(QStringLiteral("Build Context"), box);
    context_text_ = new QPlainTextEdit(box);
    context_text_->setReadOnly(true);

    auto* budget_row = new QHBoxLayout;
    budget_row->addWidget(new QLabel(QStringLiteral("Token budget:"), box));
    budget_row->addWidget(context_budget_slider_);
    budget_row->addWidget(budget_value_label_);
    budget_row->addWidget(build_context_button_);

    auto* box_layout = new QVBoxLayout(box);
    box_layout->addLayout(budget_row);
    box_layout->addWidget(context_text_);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(box);

    connect(context_budget_slider_, &QSlider::valueChanged, this, &ContextWidget::budget_changed);
    connect(build_context_button_, &QPushButton::clicked, this, &ContextWidget::request_context);

    reset();
}

int ContextWidget::token_budget() const {
    return context_budget_slider_->value();
}

void ContextWidget::set_build_context_enabled(bool enabled) {
    build_context_button_->setEnabled(enabled);
}

void ContextWidget::set_context(const std::vector<ContextItem>& items) {
    QString text;
    for (const ContextItem& item : items) {
        text += QStringLiteral("Document: %1\nChunk: %2\nTokens: %3\nScore: %4\nTruncated: %5\nText: %6\n\n")
                    .arg(QString::fromStdString(item.document_id))
                    .arg(static_cast<qulonglong>(item.chunk_sequence))
                    .arg(static_cast<qulonglong>(item.token_count))
                    .arg(item.score)
                    .arg(item.truncated ? QStringLiteral("Yes") : QStringLiteral("No"))
                    .arg(QString::fromStdString(item.text));
    }
    context_text_->setPlainText(text);
}

void ContextWidget::clear_context() {
    context_text_->clear();
}

void ContextWidget::reset() {
    context_budget_slider_->setValue(300);
    budget_value_label_->setText(QString::number(300));
    clear_context();
    build_context_button_->setEnabled(false);
}

void ContextWidget::request_context() {
    build_context_requested(token_budget());
}

void ContextWidget::budget_changed(int value) {
    const int snapped = (value + 25) / 50 * 50;
    if (snapped != value) {
        context_budget_slider_->setValue(snapped);
        return;
    }
    budget_value_label_->setText(QString::number(value));
}

}  // namespace aiws::gui
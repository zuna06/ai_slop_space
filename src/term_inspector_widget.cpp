#include "aiws/gui/term_inspector_widget.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace aiws::gui {

TermInspectorWidget::TermInspectorWidget(QWidget* parent) : QWidget(parent) {
    auto* box = new QGroupBox(QStringLiteral("Term Inspector"), this);

    term_edit_ = new QLineEdit(box);
    inspect_button_ = new QPushButton(QStringLiteral("Inspect"), box);
    document_frequency_value_ = new QLabel(box);
    chunk_frequency_value_ = new QLabel(box);

    auto* term_row = new QHBoxLayout;
    term_row->addWidget(new QLabel(QStringLiteral("Term:"), box));
    term_row->addWidget(term_edit_);
    term_row->addWidget(inspect_button_);

    auto* stats = new QFormLayout;
    stats->addRow(QStringLiteral("Document frequency:"), document_frequency_value_);
    stats->addRow(QStringLiteral("Selected-chunk TF:"), chunk_frequency_value_);

    auto* box_layout = new QVBoxLayout(box);
    box_layout->addLayout(term_row);
    box_layout->addLayout(stats);
    box_layout->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(box);

    connect(inspect_button_, &QPushButton::clicked, this, &TermInspectorWidget::request_inspection);

    reset();
}

void TermInspectorWidget::set_inspection_enabled(bool enabled) {
    inspect_button_->setEnabled(enabled);
}

void TermInspectorWidget::set_selected_chunk(const QString& chunk_id) {
    selected_chunk_id_ = chunk_id;
}

void TermInspectorWidget::set_statistics(std::size_t document_frequency,
                                         bool has_chunk_frequency,
                                         std::size_t chunk_frequency) {
    document_frequency_value_->setText(QString::number(document_frequency));
    chunk_frequency_value_->setText(has_chunk_frequency ? QString::number(chunk_frequency) : QString());
}

void TermInspectorWidget::clear_statistics() {
    document_frequency_value_->clear();
    chunk_frequency_value_->clear();
}

void TermInspectorWidget::reset() {
    term_edit_->clear();
    selected_chunk_id_.clear();
    clear_statistics();
    inspect_button_->setEnabled(false);
}

void TermInspectorWidget::request_inspection() {
    if (term_edit_->text().trimmed().isEmpty()) return;
    term_inspection_requested(term_edit_->text(), selected_chunk_id_);
}

}  // namespace aiws::gui
#include "aiws/gui/search_widget.hpp"

#include "aiws/gui/search_result_model.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QVBoxLayout>

#include <utility>

namespace aiws::gui {

SearchWidget::SearchWidget(QWidget* parent) : QWidget(parent) {
    auto* box = new QGroupBox(QStringLiteral("Query"), this);

    query_edit_ = new QLineEdit(box);
    result_count_spin_ = new QSpinBox(box);
    result_count_spin_->setRange(1, 100);
    search_button_ = new QPushButton(QStringLiteral("Search"), box);

    auto* query_row = new QHBoxLayout;
    query_row->addWidget(new QLabel(QStringLiteral("Query:"), box));
    query_row->addWidget(query_edit_);
    query_row->addWidget(new QLabel(QStringLiteral("Results (k):"), box));
    query_row->addWidget(result_count_spin_);
    query_row->addWidget(search_button_);

    result_model_ = new SearchResultModel(this);
    results_view_ = new QTableView(box);
    results_view_->setModel(result_model_);
    results_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    results_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    results_view_->setSelectionMode(QAbstractItemView::SingleSelection);
    results_view_->horizontalHeader()->setStretchLastSection(true);

    selected_result_text_ = new QPlainTextEdit(box);
    selected_result_text_->setReadOnly(true);
    view_source_button_ = new QPushButton(QStringLiteral("View Source"), box);

    auto* result_header = new QHBoxLayout;
    result_header->addWidget(new QLabel(QStringLiteral("Selected Result"), box));
    result_header->addStretch();
    result_header->addWidget(view_source_button_);

    auto* box_layout = new QVBoxLayout(box);
    box_layout->addLayout(query_row);
    box_layout->addWidget(results_view_, 3);
    box_layout->addLayout(result_header);
    box_layout->addWidget(selected_result_text_, 2);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(box);

    connect(search_button_, &QPushButton::clicked,
            this, &SearchWidget::request_search);
    connect(query_edit_, &QLineEdit::textChanged,
            this, &SearchWidget::query_changed);
    connect(results_view_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &SearchWidget::show_selected_result);
    connect(view_source_button_, &QPushButton::clicked,
            this, &SearchWidget::request_source_document);

    reset();
}

QString SearchWidget::query() const {
    return query_edit_->text();
}

int SearchWidget::result_count() const {
    return result_count_spin_->value();
}

bool SearchWidget::has_query() const {
    return !query_edit_->text().trimmed().isEmpty();
}

void SearchWidget::set_search_enabled(bool enabled) {
    search_button_->setEnabled(enabled);
}

void SearchWidget::set_results(std::vector<SearchResult> results) {
    result_model_->set_results(std::move(results));
    show_selected_result();
}

void SearchWidget::clear_results() {
    result_model_->clear();
    show_selected_result();
}

void SearchWidget::reset() {
    query_edit_->clear();
    result_count_spin_->setValue(3);
    clear_results();
    search_button_->setEnabled(false);
}

void SearchWidget::request_search() {
    search_requested(query(), result_count());
}

void SearchWidget::show_selected_result() {
    const QModelIndexList rows = results_view_->selectionModel()->selectedRows();
    const SearchResult* result = rows.isEmpty() ? nullptr : result_model_->result_at(rows.front().row());

    if (result == nullptr) {
        selected_result_text_->clear();
        view_source_button_->setEnabled(false);
        selected_chunk_changed(QString());
        return;
    }

    selected_result_text_->setPlainText(
        QStringLiteral("Rank: %1\nDocument: %2\nChunk: %3\nScore: %4\nMatched terms: %5\n\n%6")
            .arg(rows.front().row() + 1)
            .arg(QString::fromStdString(result->document_id))
            .arg(static_cast<qulonglong>(result->chunk_sequence))
            .arg(result->score)
            .arg(static_cast<qulonglong>(result->matched_terms))
            .arg(QString::fromStdString(result->text)));
    view_source_button_->setEnabled(true);
    selected_chunk_changed(QString::fromStdString(result->chunk_id));
}

void SearchWidget::request_source_document() {
    const QModelIndexList rows = results_view_->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    if (const SearchResult* result = result_model_->result_at(rows.front().row())) {
        source_document_requested(QString::fromStdString(result->document_id));
    }
}

}
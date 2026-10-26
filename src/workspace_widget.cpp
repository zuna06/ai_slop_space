#include "aiws/gui/workspace_widget.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace aiws::gui {

WorkspaceWidget::WorkspaceWidget(QWidget* parent) : QWidget(parent) {
    auto* workspace_box = new QGroupBox(QStringLiteral("Workspace"), this);
    document_list_ = new QListWidget(workspace_box);
    add_document_button_ = new QPushButton(QStringLiteral("Add Document"), workspace_box);
    build_corpus_button_ = new QPushButton(QStringLiteral("Build Corpus"), workspace_box);
    corpus_status_label_ = new QLabel(workspace_box);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(add_document_button_);
    buttons->addWidget(build_corpus_button_);

    auto* workspace_layout = new QVBoxLayout(workspace_box);
    workspace_layout->addWidget(document_list_);
    workspace_layout->addLayout(buttons);
    workspace_layout->addWidget(corpus_status_label_);

    auto* document_box = new QGroupBox(QStringLiteral("Selected Document"), this);
    document_title_label_ = new QLabel(document_box);
    document_text_ = new QPlainTextEdit(document_box);
    document_text_->setReadOnly(true);

    auto* document_layout = new QVBoxLayout(document_box);
    document_layout->addWidget(document_title_label_);
    document_layout->addWidget(document_text_);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(workspace_box, 2);
    layout->addWidget(document_box, 3);

    connect(add_document_button_, &QPushButton::clicked, this, &WorkspaceWidget::add_document_requested);
    connect(build_corpus_button_, &QPushButton::clicked, this, &WorkspaceWidget::build_corpus_requested);
    connect(document_list_, &QListWidget::currentRowChanged, this, &WorkspaceWidget::document_selected);

    reset();
}

void WorkspaceWidget::add_document(const QString& title) {
    document_list_->addItem(title);
}

void WorkspaceWidget::select_document(int row) {
    document_list_->setCurrentRow(row); 
}

void WorkspaceWidget::show_selected_document(const QString& title,
                                              const QString& text) {
    document_title_label_->setText(title);
    document_text_->setPlainText(text);
}

void WorkspaceWidget::clear_selected_document() {
    document_title_label_->clear();
    document_text_->clear();
}

void WorkspaceWidget::set_corpus_status(const QString& text) {
    corpus_status_label_->setText(text);
}

void WorkspaceWidget::set_build_corpus_enabled(bool enabled) {
    build_corpus_button_->setEnabled(enabled);
}

void WorkspaceWidget::reset() {
    document_list_->clear();
    clear_selected_document();
    corpus_status_label_->setText(QStringLiteral("Corpus: Not built"));
    build_corpus_button_->setEnabled(false);
}

}  // namespace aiws::gui

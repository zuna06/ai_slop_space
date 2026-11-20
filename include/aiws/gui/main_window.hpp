#pragma once

#include <QMainWindow>

#include "aiws/processing_core.hpp"
#include "aiws/workspace.hpp"
#include "aiws/gui/ipc_server.hpp"
#include "aiws/build_worker.hpp"
#include "aiws/gui/corpus_state.hpp"

#include <cstdint>
#include <string>
#include <memory>

class QLabel;
class QPushButton;
class QString;
class QWidget;

namespace aiws::gui {

class ContextWidget;
class SearchWidget;
class TermInspectorWidget;
class WorkspaceWidget;

/**
 * @brief Main application window and coordinator for the M3 AI Workspace GUI.
 *
 * MainWindow owns the application Workspace and ProcessingCore, composes the
 * major GUI components, and coordinates application-wide state changes and
 * backend operations in response to component events.
 */
class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Function-pointer type used for the application Exit action.
     *
     * The callback receives the main window as a parent for any exit-related
     * interaction. In M3, the registered callback asks the user to confirm the
     * exit. Declining the confirmation leaves the running application and its
     * current state unchanged.
     */
    using ExitCallback = void (*)(QWidget* parent);

    /**
     * @brief Constructs the main AI Workspace application window.
     * @param parent Optional Qt parent widget.
     */
    explicit MainWindow(QWidget* parent = nullptr);

    ~MainWindow() override;

    /**
     * @brief Registers the callback invoked when the user requests Exit.
     * @param callback Function pointer to invoke for the Exit action. A null
     * callback means that an Exit request has no registered callback to invoke.
     */
    void set_exit_callback(ExitCallback callback) noexcept;

    // ---- M4 required public API (see specification, Section 5) ----------

    /**
     * @brief Adds the file at path exactly as the Add Document action does.
     * @return false, changing nothing, if the file cannot be read.
     */
    bool add_document_file(const QString& path);

    /**
     * @brief Sets the extra build time per document for builds started after this call.
     * @param ms_per_document Delay in milliseconds (the --build-delay-ms value).
     */
    void set_build_delay_ms(int ms_per_document);

    // starts up the ipc server for adding documents
    void start_ipc(const std::string& socket_path);

    // Everything below this line is the M3 design. In M4 it is yours to
    // change: add, remove, or redesign private members, slots, and helpers.

signals:

    // this signal ets emitted on the gui thread when a finished build reaches the window
    void build_result_received();

private slots:
    /** @brief Handles a request to select and add a document to the workspace. */
    void add_document();

    /** @brief Rebuilds the processing corpus from the current non-empty workspace. */
    void build_corpus();

    /**
     * @brief Updates the selected-document display for the requested workspace row.
     * @param row Zero-based document row, or a negative value when no row is selected.
     */
    void document_selected(int row);

    /**
     * @brief Performs a search of the current corpus and presents the returned results.
     * @param query Query text to search for.
     * @param k Maximum number of results requested.
     */
    void search_requested(const QString& query, int k);

    /**
     * @brief Responds to a query edit by updating application action availability.
     * @param query Updated query text.
     */
    void query_changed(const QString& query);

    /**
     * @brief Builds and displays context for the current query and search settings.
     * @param token_budget Maximum token budget requested for the context.
     */
    void build_context_requested(int token_budget);
    /**
     * @brief Selects and displays the workspace document identified by a search result.
     * @param document_id Identifier of the source document to reveal.
     */
    void source_document_requested(const QString& document_id);

    /**
     * @brief Queries and presents term statistics for the current corpus.
     * @param term Term to inspect.
     * @param chunk_id Selected result chunk identifier; may be empty, in which
     * case document frequency is still available but chunk frequency is not.
     */
    void term_inspection_requested(const QString& term, const QString& chunk_id);

    /**
     * @brief Restores the application and its major GUI components to initial state.
     */
    void reset_application();

    /**
     * @brief Invokes the registered Exit callback for the current exit request.
     */
    void exit_application();

private:
    /**
     * @brief Updates availability of actions that depend on current application state.
     */
    void update_action_states();

    /**
     * @brief Clears displayed search and context output that depends on processing state.
     */
    void clear_processing_outputs();

    // accepts result only if the job_id is the newest one
    void build_finished(const std::shared_ptr<BuildResult>& result);

    // state helpers
    friend class NotBuiltState;
    friend class BuildingState;
    friend class CurrentState;
    friend class FailedState;

    // state switcher
    void set_state(std::unique_ptr<CorpusState> next);

    // gives copy of workspace to worker and returns job id
    std::uint64_t start_build();

    // drops queued build and cancels running build
    void cancel_build();

    // corpus display functions
    void show_not_built();
    void show_built(ProcessingCore& core);
    void show_build_failed(const std::string& message);

    Workspace workspace_;
    ProcessingCore processing_core_;
    std::unique_ptr<CorpusState> state_{std::make_unique<NotBuiltState>()};
    int build_delay_ms_{0};
    ExitCallback exit_callback_{nullptr};
    IPCServer ipc_server_;

    // one build thread, starts with window
    BuildWorker build_worker_;

    WorkspaceWidget* workspace_widget_{nullptr};
    SearchWidget* search_widget_{nullptr};
    TermInspectorWidget* term_inspector_widget_{nullptr};
    ContextWidget* context_widget_{nullptr};
    QLabel* status_label_{nullptr};
    QPushButton* reset_button_{nullptr};
    QPushButton* exit_button_{nullptr};
};

}  // namespace aiws::gui

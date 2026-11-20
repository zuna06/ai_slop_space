#include "aiws/gui/corpus_state.hpp"
#include "aiws/gui/main_window.hpp"

#include <memory>

namespace aiws::gui {

    // ignored, parameters not used
    void CorpusState::build_succeeded(MainWindow&, std::uint64_t, ProcessingCore&) {}
    void CorpusState::build_failed(MainWindow&, std::uint64_t, const std::string&) {}

    // not built state
    void NotBuiltState::build_requested(MainWindow& window) {
        window.set_state(std::make_unique<BuildingState>(window.start_build()));
    }

    void NotBuiltState::workspace_changed(MainWindow& window) {
        window.show_not_built();
    }

    bool NotBuiltState::can_build() const { return true; }
    bool NotBuiltState::corpus_usable() const { return false; }

    // building state
    BuildingState::BuildingState(std::uint64_t job_id) : job_id_(job_id) {}

    // build corpus is disabled while building
    // so other requests are ignored
    void BuildingState::build_requested(MainWindow&) {}

    // cancel the running or queued build since it's now stale
    void BuildingState::workspace_changed(MainWindow& window) {
        window.cancel_build();
        window.show_not_built();
        window.set_state(std::make_unique<NotBuiltState>());
    }

    // 
    void BuildingState::build_succeeded(MainWindow& window, std::uint64_t job_id, ProcessingCore& core) {
        // stale check
        if (job_id != job_id_) return;

        window.show_built(core);
        window.set_state(std::make_unique<CurrentState>());
    }

    void BuildingState::build_failed(MainWindow& window, std::uint64_t job_id, const std::string& message) {
        // stale check
        if (job_id != job_id_) return;

        window.show_build_failed(message);
        window.set_state(std::make_unique<FailedState>());
    }

    bool BuildingState::can_build() const { return false; }
    bool BuildingState::corpus_usable() const { return false; }

    // current state

    void CurrentState::build_requested(MainWindow& window) {
        window.set_state(std::make_unique<BuildingState>(window.start_build()));
    }

    void CurrentState::workspace_changed(MainWindow& window) {
        window.show_not_built();
        window.set_state(std::make_unique<NotBuiltState>());
    }

    bool CurrentState::can_build() const { return true; }
    bool CurrentState::corpus_usable() const { return true; }

    // failed state

    void FailedState::build_requested(MainWindow& window) {
        window.set_state(std::make_unique<BuildingState>(window.start_build()));
    }

    void FailedState::workspace_changed(MainWindow& window) {
        window.show_not_built();
        window.set_state(std::make_unique<NotBuiltState>());
    }

    bool FailedState::can_build() const { return true; }
    bool FailedState::corpus_usable() const { return false; }
}
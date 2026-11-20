#pragma once

#include "aiws/processing_core.hpp"

#include <cstdint>
#include <string>

namespace aiws::gui {

    class MainWindow;

    // base class for the corpus state pattern
    // states are created, called, and destroyed on gui thread only
    // essentially the same states from enum but now as classes
    // with the related functions

    class CorpusState {
        public:
            virtual ~CorpusState() = default;

            // for when user clicks build corpus
            virtual void build_requested(MainWindow& window) = 0;

            // called when doc is added or workspace is reset
            virtual void workspace_changed(MainWindow& window) = 0;

            // build result reaches gui thread
            virtual void build_succeeded(MainWindow& window, std::uint64_t job_id, ProcessingCore& core);
            virtual void build_failed(MainWindow& window, std::uint64_t job_id, const std::string& message);

            // checks what user can or can't do in state
            virtual bool can_build() const = 0;
            virtual bool corpus_usable() const = 0;
    };

    // all the actual states
    // derive from base class
            
    class NotBuiltState final : public CorpusState {
        public:
            void build_requested(MainWindow& window) override;
            void workspace_changed(MainWindow& window) override;
            bool can_build() const override;
            bool corpus_usable() const override;
    };

    class BuildingState final : public CorpusState {
        public:
            explicit BuildingState(std::uint64_t job_id);

            void build_requested(MainWindow& window) override;
            void workspace_changed(MainWindow& window) override;
            
            void build_succeeded(MainWindow& window, std::uint64_t job_id, ProcessingCore& core) override;
            void build_failed(MainWindow& window, std::uint64_t job_id, const std::string& message) override;

            bool can_build() const override;
            bool corpus_usable() const override;

        private:
            std::uint64_t job_id_;
    };

    class CurrentState final : public CorpusState {
        public:
            void build_requested(MainWindow& window) override;
            void workspace_changed(MainWindow& window) override;
            bool can_build() const override;
            bool corpus_usable() const override;
    };

    class FailedState final : public CorpusState {
        public:
            void build_requested(MainWindow& window) override;
            void workspace_changed(MainWindow& window) override;
            bool can_build() const override;
            bool corpus_usable() const override;
    };
    
}
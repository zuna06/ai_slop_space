#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>

namespace aiws {

    class BuildCancelled: public std::runtime_error {
        public:
            BuildCancelled() : std::runtime_error("Build cancelled") {}
    };

    // one per build
    // cancelled by GUI thread through BuildWorker
    class CancelToken {
        public:
            // sets the flag and wakes any waiting worker
            void cancel();
            bool cancelled() const;
            
            // sleeps up to duration
            // returns true early if cancel() is called
            bool wait_for(std::chrono::milliseconds duration) const;
        
        private:
            mutable std::mutex mutex_;
            mutable std::condition_variable cancelled_var;
            // guarded by mutax
            bool cancelled_{ false };
    };
}
#include "aiws/cancel_token.hpp"

namespace aiws {

    void CancelToken::cancel() {
        std::lock_guard<std::mutex> lock(mutex_);
        cancelled_ = true;
        cancelled_var.notify_all();
    }

    bool CancelToken::cancelled() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cancelled_;
    }

    bool CancelToken::wait_for(std::chrono::milliseconds duration) const {
        std::unique_lock<std::mutex> lock(mutex_);

        return cancelled_var.wait_for(lock, duration, [this] { return cancelled_; });
    }

}
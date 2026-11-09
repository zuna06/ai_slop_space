#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace aiws::gui {

    class IPCServer {
        public:
            // helpful alias
            using AddCallback = std::function<void(const std::string&)>;
            
            IPCServer() = default;
            ~IPCServer();

            // delete the copy constructors
            IPCServer(const IPCServer&) = delete;
            IPCServer& operator=(const IPCServer&) = delete;

            // only want start and stop to be seen and handled in public
            void start(const std::string& socket_path, AddCallback callback);
            void stop();
        
        private:
            // run and handle client are backend
            void run();
            void handle_client(int fd);

            std::string socket_path_;
            AddCallback callback_;
            std::atomic<bool> running_{false};
            int listen_fd_{-1};
            std::thread thread_;
    };
}
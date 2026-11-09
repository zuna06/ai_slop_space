#include "aiws/gui/ipc_server.hpp"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <vector>

namespace aiws::gui {
    namespace {
        bool read_exact(int fd, void* destination, std::size_t size,
                        const std::atomic<bool>& running) {
            auto* bytes = static_cast<char*>(destination);
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            pollfd socket{fd, POLLIN, 0};
            while (size && running && std::chrono::steady_clock::now() < deadline) {
                const int ready = poll(&socket, 1, 50);
                if (ready == 0 || (ready < 0 && errno == EINTR)) continue;
                if (ready < 0) return false;
                const auto count = recv(fd, bytes, size, MSG_DONTWAIT);
                if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) continue;
                if (count <= 0) return false;
                bytes += count;
                size -= static_cast<std::size_t>(count);
            }
            return size == 0;
        }

        void reply(int fd, const char* reason = nullptr) {
            std::string message(1, reason ? '\1' : '\0');
            if (reason) message += reason;
            std::size_t sent = 0;
            while (sent < message.size()) {
                const auto n = send(fd, message.data() + sent, message.size() - sent,
                                    MSG_NOSIGNAL | MSG_DONTWAIT);
                if (n < 0 && errno == EINTR) continue;
                if (n <= 0) return;
                sent += static_cast<std::size_t>(n);
            }
        }
    }
    
    IPCServer::~IPCServer() {
        stop();
    }

    void IPCServer::start(const std::string& path, AddCallback callback) {
        if (thread_.joinable() || path.empty() || path.size() >= sizeof(sockaddr_un::sun_path)) return;
        socket_path_ = path;
        callback_ = std::move(callback);

        listen_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        // check for errors
        if (listen_fd_ < 0) return;

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

        auto conn = bind(listen_fd_, (sockaddr*)(&addr), sizeof(addr));

        // check if connection was successful
        // if not then close the fd and unlink the socket
        if (conn < 0 || listen(listen_fd_, 4) < 0) {
            close(listen_fd_);
            listen_fd_ = -1;
            if (conn == 0) unlink(path.c_str());
            return;
        }

        running_ = true;
        thread_ = std::thread(&IPCServer::run, this);
    }

    // shutdown and closes the fd
    // joins the thread and unlinks the socket
    void IPCServer::stop() {
        if (running_ == false) return;
        running_ = false;
        
        if (listen_fd_ >= 0) {
            shutdown(listen_fd_, SHUT_RDWR);
        }

        if (thread_.joinable()) {
            thread_.join();
        }

        close(listen_fd_);
        listen_fd_ = -1;
        unlink(socket_path_.c_str());
    }

    void IPCServer::run() {
        while (running_) {
            int client = accept(listen_fd_, nullptr, nullptr);
            if (client < 0 && errno == EINTR) continue;
            if (client < 0) break;

            handle_client(client);
            close(client);
        }
    }

    // parses the data sent over
    // then queues the add on the gui thread
    void IPCServer::handle_client(int fd) {
        uint8_t header[4];
        if (!read_exact(fd, header, sizeof(header), running_)) return;

        // pack the header bytes into one 32 bit int
        uint32_t payload_len = (uint32_t(header[0]) << 24 | uint32_t(header[1]) << 16 |
                                uint32_t(header[2]) << 8 | uint32_t(header[3]));
        
        // check the length
        // rejects empty or oversized lengths
        if (payload_len == 0 || payload_len > 1024 * 1024) return;
        
        std::string payload(payload_len, '\0');
        if (!read_exact(fd, payload.data(), payload.size(), running_)) return;

        // payload should be more than 4 bytes
        // and should have add for add document
        // for the first 4 bytes, that is the
        // only command to be accepted from remote
        // connection
        // write back an failed status and error message
        if (payload.size() <= 4 || payload.compare(0, 4, "add ") || payload.find('\0') != std::string::npos) {
            reply(fd, "invalid add request");
            return;
        }

        // read the file path
        // stat is for storing file attributes
        std::string file_path = payload.substr(4);
        struct stat stat_{};

        // stores the file attributes from file_path
        // in stat_
        bool stat_failed = stat(file_path.c_str(), &stat_);

        // checks file exists and is a regular file
        if (stat_failed != 0 || !S_ISREG(stat_.st_mode)) {
            reply(fd, S_ISDIR(stat_.st_mode) ? "is a directory" : "cannot read file");
            return;
        }

        // check for access permissions
        if (access(file_path.c_str(), R_OK) != 0) {
            reply(fd, "permission denied");
            return;
        }

        // finally if it gets accepted then queue the add on gui thread
        if (callback_) callback_(file_path);
        reply(fd);
    }
}

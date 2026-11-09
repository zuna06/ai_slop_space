#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <chrono>

namespace {

    std::string default_socket_path() {
        // user is the env var in unix for the username
        const char* user = std::getenv("USER");
        if (user && user[0]) {
            return std::string("/tmp/aiws-") + user + ".sock";
        }

        // if user env variable is not found or too short
        // use the users id instead
        return std::string("/tmp/aiws-") + std::to_string(getuid()) + ".sock";
    }

    // turn the relative path into an absolute path
    std::string make_absolute(const char* path) {
        // if path is already absolute return it
        if (path[0] == '/') {
            return path;
        }

        char current_dir[4096];
        if (getcwd(current_dir, sizeof(current_dir))) {
            return std::string(current_dir) + "/" + path;
        }

        return path;
    }

    using Clock = std::chrono::steady_clock;

    // wait until the socket is ready
    bool wait_for_socket(int fd, short events, Clock::time_point deadline) {
        pollfd pfd{fd, events, 0};

        while (true) {
            auto left = std::chrono::ceil<std::chrono::milliseconds>(
                deadline - Clock::now()
            );
            if (left.count() <= 0) return false;

            int ready = poll(&pfd, 1, static_cast<int>(left.count()));
            if (ready < 0 && errno == EINTR) continue;
            if (ready <= 0) return false;

            // let read or send handle a closed socket
            return (pfd.revents & (events | POLLHUP | POLLERR)) != 0;
        }
    }

    // send the rest if the first send only wrote part of the data
    bool send_all(int fd, const void* data, size_t size, Clock::time_point deadline) {
        const char* bytes = static_cast<const char*>(data);
        size_t sent = 0;

        while (sent < size) {
            if (!wait_for_socket(fd, POLLOUT, deadline)) return false;

            auto n = send(fd, bytes + sent, size - sent, MSG_NOSIGNAL);
            if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;
            }
            if (n <= 0) return false;

            sent += static_cast<size_t>(n);
        }

        return true;
    }

    // returns bytes read, 0 if the server closed, or -1 on failure
    ssize_t read_reply(int fd, void* data, size_t size, Clock::time_point deadline) {
        while (wait_for_socket(fd, POLLIN, deadline)) {
            auto n = read(fd, data, size);
            if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;
            }
            return n;
        }

        return -1;
    }

}

int main(int argc, char* argv[]) {
    std::string socket_path = default_socket_path();
    int i = 1;

    if (i < argc && std::string(argv[i]) == "--socket") {
        if (++i >= argc) {
            std::cerr << "missing socket path\n";
            return 2;
        }

        socket_path = argv[i++];
    }

    if (i >= argc || std::string(argv[i]) != "add") {
        std::cerr << "usage: aiws_send [--socket PATH] add FILE\n";
        return 2;
    }

    i++;

    if (i >= argc) {
        std::cerr << "missing file argument\n";
        return 2;
    }
    std::string file_path = make_absolute(argv[i]);

    // after argument is read build the payload to be sent
    // the payload should be length + "add" + path
    // should be in big endian
    std::string payload = "add " + file_path;
    auto len = static_cast<uint32_t>(payload.size());
    uint8_t header[4] = {  static_cast<uint8_t>(len >> 24), static_cast<uint8_t>(len >> 16),
                        static_cast<uint8_t>(len >> 8), static_cast<uint8_t>(len)
    };

    // get fd
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket failed\n";
        return 2;
    }

    // set non blocking so connect can time out
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(fd);
        return 2;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (socket_path.empty() || socket_path.size() >= sizeof(addr.sun_path)) {
        close(fd);
        std::cerr << "invalid socket path\n";
        return 2;
    }
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

        // connect to the app
    int remote_conn = connect(fd, (sockaddr*)(&addr), sizeof(addr));
    if (remote_conn < 0) {
        if (errno != EINPROGRESS) {
            close(fd);
            std::cerr << "connection failed\n";
            return 2;
        }

        // connect is still running, give it up to 2 seconds
        pollfd pfd{fd, POLLOUT, 0};
        if (poll(&pfd, 1, 2000) <= 0) {
            close(fd);
            std::cerr << "connection failed or timed out\n";
            return 2;
        }

        int err = 0;
        socklen_t error_len = sizeof(err);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &error_len) < 0 || err != 0) {
            close(fd);
            std::cerr << "connection failed\n";
            return 2;
        }
    }

    // keep the socket nonblocking so the whole exchange has one 5 second limit
    auto deadline = Clock::now() + std::chrono::seconds(5);

    if (!send_all(fd, header, sizeof(header), deadline) ||
        !send_all(fd, payload.data(), payload.size(), deadline)) {
        close(fd);
        std::cerr << "send failed\n";
        return 2;
    }

    uint8_t status{};
    if (read_reply(fd, &status, 1, deadline) != 1 || status > 1) {
        close(fd);
        std::cerr << "invalid or missing response\n";
        return 2;
    }
    if (status == 0) {
        close(fd);
        return 0;
    }

    // the server closes the connection after the error message
    char buffer[4096];
    std::string error_message;
    ssize_t n;
    while ((n = read_reply(fd, buffer, sizeof(buffer), deadline)) > 0) {
        error_message.append(buffer, static_cast<size_t>(n));
    }

    close(fd);
    if (n < 0) {
        std::cerr << "could not read response\n";
        return 2;
    }

    std::cerr << (error_message.empty() ? "request rejected" : error_message) << '\n';
    return 1;
}

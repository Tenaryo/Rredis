#include "replica_connector.hpp"
#include "protocol/resp_parser.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

ReplicaConnector::ReplicaConnector(std::string host, int port)
    : host_(std::move(host)), port_(port) {}

ReplicaConnector::~ReplicaConnector() {
    if (fd_ >= 0)
        ::close(fd_);
}

ReplicaConnector::ReplicaConnector(ReplicaConnector&& other) noexcept
    : host_(std::move(other.host_)), port_(other.port_), fd_(other.fd_) {
    other.fd_ = -1;
}

ReplicaConnector& ReplicaConnector::operator=(ReplicaConnector&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0)
            ::close(fd_);
        host_ = std::move(other.host_);
        port_ = other.port_;
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

bool ReplicaConnector::connect_to_master() {
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0)
        return false;

    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);

    if (::inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    if (::connect(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    return true;
}

bool ReplicaConnector::send_ping() {
    if (fd_ < 0 && !connect_to_master())
        return false;

    auto ping = RespParser::encode_array({"PING"});
    ssize_t sent = ::send(fd_, ping.data(), ping.size(), MSG_NOSIGNAL);
    if (sent != static_cast<ssize_t>(ping.size()))
        return false;

    char buf[256]{};
    ssize_t n = ::read(fd_, buf, sizeof(buf));
    if (n <= 0)
        return false;

    return std::string_view(buf, n) == "+PONG\r\n";
}

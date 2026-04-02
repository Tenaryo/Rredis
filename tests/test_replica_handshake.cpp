#include "../src/handler/command_handler.hpp"
#include "../src/protocol/resp_parser.hpp"
#include "../src/replica/replica_connector.hpp"
#include "../src/server/server_config.hpp"
#include "../src/store/store.hpp"
#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

class MockMaster {
    int server_fd_{-1};
    int port_{0};
  public:
    MockMaster() {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        assert(server_fd_ >= 0);

        int reuse = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        struct sockaddr_in addr {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(0);

        assert(bind(server_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0);
        assert(listen(server_fd_, 1) == 0);

        socklen_t len = sizeof(addr);
        getsockname(server_fd_, reinterpret_cast<struct sockaddr*>(&addr), &len);
        port_ = ntohs(addr.sin_port);
    }

    ~MockMaster() {
        if (server_fd_ >= 0)
            ::close(server_fd_);
    }

    [[nodiscard]] int port() const noexcept { return port_; }
    [[nodiscard]] int fd() const noexcept { return server_fd_; }

    struct HandshakeResult {
        std::string received;
        bool accepted{false};
    };

    struct MultiStepResult {
        std::vector<std::string> messages;
        bool accepted{false};
    };

    HandshakeResult run_ping_handshake() {
        HandshakeResult result;
        struct sockaddr_in client_addr {};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd_, reinterpret_cast<struct sockaddr*>(&client_addr), &len);
        if (client_fd < 0)
            return result;

        result.accepted = true;

        char buf[256]{};
        auto n = ::read(client_fd, buf, sizeof(buf));
        if (n > 0)
            result.received = std::string(buf, static_cast<size_t>(n));

        const char* pong = "+PONG\r\n";
        ::send(client_fd, pong, std::strlen(pong), 0);
        ::close(client_fd);
        return result;
    }

    MultiStepResult run_multi_step_handshake(const std::vector<std::string>& responses) {
        MultiStepResult result;
        struct sockaddr_in client_addr {};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd_, reinterpret_cast<struct sockaddr*>(&client_addr), &len);
        if (client_fd < 0)
            return result;

        result.accepted = true;

        for (const auto& resp : responses) {
            char buf[512]{};
            auto n = ::read(client_fd, buf, sizeof(buf));
            if (n > 0)
                result.messages.emplace_back(buf, static_cast<size_t>(n));
            ::send(client_fd, resp.c_str(), resp.size(), 0);
        }

        ::close(client_fd);
        return result;
    }
};

void test_replica_ping_handshake_with_localhost() {
    MockMaster master;
    int port = master.port();

    MockMaster::HandshakeResult server_result;

    std::thread server_thread([&]() { server_result = master.run_ping_handshake(); });

    ReplicaConnector connector("localhost", port);
    bool success = connector.send_ping();

    server_thread.join();

    assert(success);
    assert(server_result.accepted);
    assert(server_result.received == "*1\r\n$4\r\nPING\r\n");

    std::cout << "\u2713 Test passed: replica connects via hostname localhost and completes PING "
                 "handshake\n";
}

void test_replica_ping_handshake() {
    MockMaster master;
    int port = master.port();

    MockMaster::HandshakeResult server_result;

    std::thread server_thread([&]() { server_result = master.run_ping_handshake(); });

    ReplicaConnector connector("127.0.0.1", port);
    bool success = connector.send_ping();

    server_thread.join();

    assert(success);
    assert(server_result.accepted);
    assert(server_result.received == "*1\r\n$4\r\nPING\r\n");

    std::cout << "\u2713 Test passed: replica sends RESP-encoded PING and receives PONG\n";
}

void test_replica_sends_replconf_listening_port() {
    MockMaster master;
    int port = master.port();
    int replica_port = 6380;

    MockMaster::MultiStepResult server_result;

    std::thread server_thread([&]() {
        server_result = master.run_multi_step_handshake({"+PONG\r\n", "+OK\r\n", "+OK\r\n"});
    });

    ReplicaConnector connector("127.0.0.1", port);
    assert(connector.send_ping());
    assert(connector.send_replconf(replica_port));

    server_thread.join();

    assert(server_result.accepted);
    assert(server_result.messages.size() == 3);
    assert(server_result.messages[1] ==
           "*3\r\n$8\r\nREPLCONF\r\n$14\r\nlistening-port\r\n$4\r\n6380\r\n");

    std::cout << "\u2713 Test passed: replica sends REPLCONF listening-port correctly\n";
}

void test_replica_sends_replconf_capa() {
    MockMaster master;
    int port = master.port();
    int replica_port = 6380;

    MockMaster::MultiStepResult server_result;

    std::thread server_thread([&]() {
        server_result = master.run_multi_step_handshake({"+PONG\r\n", "+OK\r\n", "+OK\r\n"});
    });

    ReplicaConnector connector("127.0.0.1", port);
    assert(connector.send_ping());
    assert(connector.send_replconf(replica_port));

    server_thread.join();

    assert(server_result.accepted);
    assert(server_result.messages.size() == 3);
    assert(server_result.messages[2] == "*3\r\n$8\r\nREPLCONF\r\n$4\r\ncapa\r\n$6\r\npsync2\r\n");

    std::cout << "\u2713 Test passed: replica sends REPLCONF capa psync2 correctly\n";
}

void test_master_handles_replconf_listening_port() {
    Store store;
    ServerConfig config;
    CommandHandler handler(store, config);

    std::string input = "*3\r\n$8\r\nREPLCONF\r\n$14\r\nlistening-port\r\n$4\r\n6380\r\n";
    auto response = handler.process(input);

    assert(response == "+OK\r\n");

    std::cout << "\u2713 Test passed: master responds +OK to REPLCONF listening-port\n";
}

void test_master_handles_replconf_capa() {
    Store store;
    ServerConfig config;
    CommandHandler handler(store, config);

    std::string input = "*3\r\n$8\r\nREPLCONF\r\n$4\r\ncapa\r\n$6\r\npsync2\r\n";
    auto response = handler.process(input);

    assert(response == "+OK\r\n");

    std::cout << "\u2713 Test passed: master responds +OK to REPLCONF capa psync2\n";
}

void test_replica_sends_psync() {
    MockMaster master;
    int port = master.port();
    int replica_port = 6380;

    MockMaster::MultiStepResult server_result;

    std::thread server_thread([&]() {
        server_result = master.run_multi_step_handshake(
            {"+PONG\r\n",
             "+OK\r\n",
             "+OK\r\n",
             "+FULLRESYNC 8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb 0\r\n"});
    });

    ReplicaConnector connector("127.0.0.1", port);
    assert(connector.send_ping());
    assert(connector.send_replconf(replica_port));
    assert(connector.send_psync());

    server_thread.join();

    assert(server_result.accepted);
    assert(server_result.messages.size() == 4);
    assert(server_result.messages[3] == "*3\r\n$5\r\nPSYNC\r\n$1\r\n?\r\n$2\r\n-1\r\n");

    std::cout << "\u2713 Test passed: replica sends PSYNC ? -1 correctly\n";
}

void test_master_handles_psync() {
    Store store;
    ServerConfig config;
    CommandHandler handler(store, config);

    std::string input = "*3\r\n$5\r\nPSYNC\r\n$1\r\n?\r\n$2\r\n-1\r\n";
    auto response = handler.process(input);

    assert(response == "+FULLRESYNC 8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb 0\r\n");

    std::cout << "\u2713 Test passed: master responds FULLRESYNC to PSYNC\n";
}

constexpr std::string_view kEmptyRdbHex =
    "524544495330303131fa0972656469732d76657205371e125227"
    "0a3c8391000d31d09bfb91e19e5176a";

void test_master_psync_includes_empty_rdb() {
    Store store;
    ServerConfig config;
    CommandHandler handler(store, config);

    std::string input = "*3\r\n$5\r\nPSYNC\r\n$1\r\n?\r\n$2\r\n-1\r\n";
    auto response = handler.process(input);

    std::string expected_prefix =
        "+FULLRESYNC 8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb 0\r\n";
    assert(response.starts_with(expected_prefix));

    std::string_view rdb_part(response.data() + expected_prefix.size(),
                              response.size() - expected_prefix.size());

    std::array<unsigned char, 41> rdb_bytes{0x52, 0x45, 0x44, 0x49, 0x53, 0x30, 0x30, 0x31,
                                            0x31, 0xfa, 0x09, 0x72, 0x65, 0x64, 0x69, 0x73,
                                            0x2d, 0x76, 0x65, 0x72, 0x05, 0x37, 0x1e, 0x12,
                                            0x52, 0x27, 0x0a, 0x3c, 0x83, 0x91, 0x00, 0xd3,
                                            0x1d, 0x09, 0xbf, 0xb9, 0x1e, 0x19, 0xe5, 0x17,
                                            0x6a};

    std::string expected_rdb_header = "$41\r\n";
    assert(rdb_part.starts_with(expected_rdb_header));

    std::string_view binary_part(rdb_part.data() + expected_rdb_header.size(),
                                 rdb_part.size() - expected_rdb_header.size());
    assert(binary_part.size() == 41);
    assert(std::memcmp(binary_part.data(), rdb_bytes.data(), 41) == 0);

    std::cout
        << "\u2713 Test passed: master PSYNC response includes empty RDB file after FULLRESYNC\n";
}

void test_replica_receives_rdb() {
    std::array<unsigned char, 41> rdb_bytes{0x52, 0x45, 0x44, 0x49, 0x53, 0x30, 0x30, 0x31,
                                            0x31, 0xfa, 0x09, 0x72, 0x65, 0x64, 0x69, 0x73,
                                            0x2d, 0x76, 0x65, 0x72, 0x05, 0x37, 0x1e, 0x12,
                                            0x52, 0x27, 0x0a, 0x3c, 0x83, 0x91, 0x00, 0xd3,
                                            0x1d, 0x09, 0xbf, 0xb9, 0x1e, 0x19, 0xe5, 0x17,
                                            0x6a};

    std::string rdb_file(rdb_bytes.begin(), rdb_bytes.end());
    std::string fullresync_resp =
        "+FULLRESYNC 8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb 0\r\n";
    std::string rdb_transfer = "$41\r\n" + rdb_file;

    MockMaster master;
    int port = master.port();

    std::string received_rdb;
    MockMaster::MultiStepResult server_result;

    std::thread server_thread([&]() {
        struct sockaddr_in client_addr {};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(master.fd(), nullptr, nullptr);
        assert(client_fd >= 0);

        auto read_msg = [&]() -> std::string {
            char buf[512]{};
            auto n = ::read(client_fd, buf, sizeof(buf));
            return n > 0 ? std::string(buf, static_cast<size_t>(n)) : std::string{};
        };

        auto msg1 = read_msg();
        server_result.messages.push_back(std::move(msg1));
        ::send(client_fd, "+PONG\r\n", 7, 0);

        auto msg2 = read_msg();
        server_result.messages.push_back(std::move(msg2));
        ::send(client_fd, "+OK\r\n", 5, 0);

        auto msg3 = read_msg();
        server_result.messages.push_back(std::move(msg3));
        ::send(client_fd, "+OK\r\n", 5, 0);

        auto msg4 = read_msg();
        server_result.messages.push_back(std::move(msg4));
        ::send(client_fd, fullresync_resp.c_str(), fullresync_resp.size(), 0);
        ::send(client_fd, rdb_transfer.c_str(), rdb_transfer.size(), 0);

        char rdb_buf[256]{};
        auto rdb_n = ::read(client_fd, rdb_buf, sizeof(rdb_buf));
        if (rdb_n > 0) {
            received_rdb = std::string(rdb_buf, static_cast<size_t>(rdb_n));
        }

        ::close(client_fd);
    });

    ReplicaConnector connector("127.0.0.1", port);
    assert(connector.send_ping());
    assert(connector.send_replconf(6380));

    auto rdb_result = connector.receive_rdb();

    server_thread.join();

    assert(rdb_result.has_value());
    assert(rdb_result->size() == 41);
    assert(std::memcmp(rdb_result->data(), rdb_bytes.data(), 41) == 0);

    std::cout << "\u2713 Test passed: replica receives empty RDB file after PSYNC\n";
}

int main() {
    std::cout << "Running replica handshake tests...\n\n";

    test_replica_ping_handshake();
    test_replica_ping_handshake_with_localhost();
    test_replica_sends_replconf_listening_port();
    test_replica_sends_replconf_capa();
    test_replica_sends_psync();
    test_master_handles_replconf_listening_port();
    test_master_handles_replconf_capa();
    test_master_handles_psync();
    test_master_psync_includes_empty_rdb();
    test_replica_receives_rdb();

    std::cout << "\n\u2713 All tests passed!\n";
    return 0;
}

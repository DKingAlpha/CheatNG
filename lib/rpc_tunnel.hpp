#pragma once

#include <string>
#include <unistd.h>
#include <thread>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// c++ 20
#include <bit>

template <typename NumT>
NumT ensure_little_endian(NumT x)
{
    if constexpr (std::endian::native == std::endian::little) {
        return x;
    } else {
        // swap
        NumT ret = 0;
        for (size_t i = 0; i < sizeof(NumT); i++) {
            ret |= (x & 0xff) << (8 * i);
            x >>= 8;
        }
        return x;
    }
}

enum class TunnelStatus
{
    OK,
    CLOSED,
    ERROR,  // implies CLOSED
};

class ITunnel
{
public:
    virtual TunnelStatus send(std::string data) = 0;
    virtual TunnelStatus recv(std::string& data) = 0;
    virtual bool close() = 0;
    virtual ~ITunnel() {}

    virtual bool is_connected() const = 0;
    virtual bool is_server_up() const = 0;  // for server mode. may be not connected
    operator bool() const { return is_connected(); };
};

// ------ Tunnel based on POSIX ------
// do not use fcntl. some platform may not support it.

/**
 * @brief read/write fd.
 * 
 * close connection instantly if any error occurs.
 */
class BaseFdIO : public ITunnel
{
public:
    BaseFdIO() : fd(-1) {}
    BaseFdIO(int fd) : fd(fd) {}
    virtual ~BaseFdIO()
    {
        this->close();
    }
    virtual TunnelStatus send(std::string data) override
    {
        if (fd < 0) {
            return TunnelStatus::ERROR;
        }
        // write u64-le size first
        uint64_t size = data.size();
        size = ensure_little_endian(size);
        auto written_size = write(fd, &size, sizeof(size));
        if (written_size != sizeof(size)) {
            this->close();
            return written_size == 0 ? TunnelStatus::CLOSED : TunnelStatus::ERROR;
        }
        // a successful write() may transfer fewer than count bytes.
        uint64_t remaining_bytes = data.size();
        while (remaining_bytes > 0) {
            ssize_t sent_bytes = write(fd, data.data() + data.size() - remaining_bytes, remaining_bytes);
            if (sent_bytes < 0) {
                this->close();
                return TunnelStatus::ERROR;
            }
            if (sent_bytes == 0) {
                // should not happen
                this->close();
                return TunnelStatus::CLOSED;
            }
            remaining_bytes -= sent_bytes;
        }
        return TunnelStatus::OK;
    }
    virtual TunnelStatus recv(std::string& data) override
    {
        if (fd < 0) {
            return TunnelStatus::ERROR;
        }
        // read u64-le size first
        uint64_t size = 0;
        auto read_size = read(fd, &size, sizeof(size));
        if (read_size != sizeof(size)) {
            this->close();
            return read_size == 0 ? TunnelStatus::CLOSED : TunnelStatus::ERROR;
        }
        size = ensure_little_endian(size);
        if (size > 16L * 1024 * 1024 * 1024) {   // more than 16 GB. malicious data.
            // too large
            this->close();
            return TunnelStatus::ERROR;
        }
        data.resize(size);
        if (size == 0) {
            return TunnelStatus::OK;
        }
        // data size should be non-zero but actually read 0. it means EOF
        uint64_t remaining_bytes = size;
        while (remaining_bytes > 0) {
            ssize_t read_bytes = read(fd, data.data() + data.size() - remaining_bytes, remaining_bytes);
            if (read_bytes < 0) {
                this->close();
                return TunnelStatus::ERROR;
            }
            if (read_bytes == 0) {
                // should not happen
                this->close();
                return TunnelStatus::CLOSED;
            }
            remaining_bytes -= read_bytes;
        }
        return TunnelStatus::OK;
    }
    
    virtual bool close() override {
        if (fd < 0) {
            return false;
        }
        ::close(fd);
        fd = -1;
        return true;
    }

    virtual bool is_connected() const override { return fd >= 0; }
protected:
    int fd;
};

/**
 * @brief 
 * Server: server fd x1, client fd x0/x1. 1-1 connection.
 * Client: server fd x0, client fd x1
 */
class TunnelTCP : public BaseFdIO
{
public:
    TunnelTCP(bool is_server, TargetConfig cfg) : cfg(cfg), socketfd(-1), is_server(is_server)
    {
        socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketfd < 0) {
            return;
        }
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(cfg.server_addr.c_str());
        server_addr.sin_port = htons(cfg.port);
        if (is_server) {
            // REUSE to avoid TIME_WAIT
            int opt = 1;
            setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            // bind to server_addr and port
            if (bind(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                ::close(socketfd);
                socketfd = -1;
                return;
            }
            if (listen(socketfd, 1) < 0) {
                ::close(socketfd);
                socketfd = -1;
                return;
            }
            server_thread = std::make_unique<std::thread>([&]() {
                while(socketfd >= 0) {
                    if (fd < 0) {
                        struct sockaddr_in client_addr{};
                        socklen_t client_addr_len = sizeof(client_addr);
                        fd = accept(socketfd, (struct sockaddr*)&client_addr, &client_addr_len);
                        if (fd >= 0) {
                            // connected
                            cheatng_log("client connected\n");
                        }
                    }
                    // has client fd, wait for client to close
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                // closing socket will unblock accept if it's still waiting for client
            });
        } else {
            // connect to server_addr and port
            if (connect(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
                fd = socketfd;
                socketfd = -1;   // transfer ownership, avoid duplicate close
            } else {
                ::close(socketfd);
                socketfd = -1;
            }
        }
    }
    virtual ~TunnelTCP() { 
        if (socketfd >= 0) {
            ::close(socketfd);    // close sockerfd first, then join
            socketfd = -1;
        }
        if (server_thread) {
            server_thread->join();
        }
    }

    virtual bool is_connected() const override { return fd >= 0; }
    virtual bool is_server_up() const override { return is_server && socketfd >= 0; }

private:
    bool is_server;
    const TargetConfig cfg;
    int socketfd;
    std::unique_ptr<std::thread> server_thread;
};


class TunnelFactory
{
public:
    static std::unique_ptr<ITunnel> create(bool is_server, TargetConfig cfg)
    {
        if (cfg.protocol == TargetConfig::Protocol::TCP) {
            return std::make_unique<TunnelTCP>(is_server, cfg);
        }
        return {nullptr};
    }
};

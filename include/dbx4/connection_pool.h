#ifndef DBX4_CONNECTION_POOL_H
#define DBX4_CONNECTION_POOL_H
#include <queue>
#include <mutex>
#include <memory>
namespace dbx4 {
class Connection {
public:
    bool is_active() { return true; }
    bool execute(const std::string& sql) { return true; }
};
class ConnectionPool {
private:
    std::queue<std::shared_ptr<Connection>> available_;
    std::mutex pool_mutex_;
    int pool_size_ = 10;
public:
    ConnectionPool(int size) : pool_size_(size) {
        for (int i = 0; i < size; i++) {
            available_.push(std::make_shared<Connection>());
        }
    }
    std::shared_ptr<Connection> acquire() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        if (available_.empty()) return nullptr;
        auto conn = available_.front();
        available_.pop();
        return conn;
    }
    void release(std::shared_ptr<Connection> conn) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        available_.push(conn);
    }
};
}
#endif

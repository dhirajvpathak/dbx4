#ifndef DBX4_ADMIN_API_H
#define DBX4_ADMIN_API_H
#include <string>
namespace dbx4 {
class AdminAPI {
public:
    bool kill_transaction(uint32_t txn_id) {
        std::cout << "[Admin] Killing transaction " << txn_id << "\n";
        return true;
    }
    bool kill_idle_connections(int idle_seconds) {
        std::cout << "[Admin] Killing idle connections > " << idle_seconds << "s\n";
        return true;
    }
    bool restart_database() {
        std::cout << "[Admin] Restarting database\n";
        return true;
    }
    bool shutdown_gracefully() {
        std::cout << "[Admin] Graceful shutdown initiated\n";
        return true;
    }
};
}
#endif

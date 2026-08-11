#ifndef DBX4_TRANSACTION_API_H
#define DBX4_TRANSACTION_API_H
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
namespace dbx4 {
enum class TxnState { IDLE, ACTIVE, PREPARING, COMMITTED, ROLLED_BACK, ABORTED };
class Transaction {
public:
    bool begin() { return true; }
    bool commit() { return true; }
    bool rollback() { return true; }
    std::string execute_sql(const std::string& sql) { return "OK"; }
    TxnState state() const { return TxnState::IDLE; }
    uint32_t txn_id() const { return 0; }
    bool is_active() const { return false; }
};
}
#endif

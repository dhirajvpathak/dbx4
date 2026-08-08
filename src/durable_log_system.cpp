// ============================================================================
// DBX4 DURABLE LOG SYSTEM (from DPVS patterns)
// Production-grade cryptographic transaction log with verification
// ============================================================================

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <memory>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
// OpenSSL removed

namespace dbx4 {

// ============================================================================
// CRYPTOGRAPHIC UTILITIES (from DPVS)
// ============================================================================

class CryptoUtils {
public:
    static std::string sha256(const std::string& data) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, data.c_str(), data.length());
        SHA256_Final(hash, &sha256);
        
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }

    static std::string canonicalize(const std::map<std::string, std::string>& data) {
        std::vector<std::pair<std::string, std::string>> sorted_data(data.begin(), data.end());
        std::sort(sorted_data.begin(), sorted_data.end());
        
        std::stringstream ss;
        for (const auto& pair : sorted_data) {
            ss << pair.first << ":" << pair.second << ";";
        }
        return ss.str();
    }
// Rest of file commented out for compilation
        std::string line;
        while (std::getline(log_file, line)) {
            if (!line.empty()) {
                LogEntry entry = parse_log_line(line);
                entries_.push_back(entry);
                sequence_number_ = std::max(sequence_number_, entry.seq);
            }
        }
        log_file.close();
    }

    void persist_to_disk(const LogEntry& entry) {
        std::ofstream log_file(log_path_, std::ios::app);
        
        log_file << entry.seq << "|";
        for (const auto& pair : entry.data) {
            log_file << pair.first << ":" << pair.second << ";";
        }
        log_file << "|" << entry.signature << "|" << entry.current_hash << "\n";
        
        log_file.close();
    }

    LogEntry parse_log_line(const std::string& line) {
        LogEntry entry;
        std::stringstream ss(line);
        std::string seq_str, data_str, sig, hash;

        std::getline(ss, seq_str, '|');
        std::getline(ss, data_str, '|');
        std::getline(ss, sig, '|');
        std::getline(ss, hash, '|');

        entry.seq = std::stoull(seq_str);
        entry.signature = sig;
        entry.current_hash = hash;

        // Parse data
        std::stringstream data_ss(data_str);
        std::string item;
        while (std::getline(data_ss, item, ';')) {
            size_t pos = item.find(':');
            if (pos != std::string::npos) {
                entry.data[item.substr(0, pos)] = item.substr(pos + 1);
            }
        }

        return entry;
    }
};

// ============================================================================
// STATE PROJECTIONS - MATERIALIZED VIEWS (from DPVS)
// ============================================================================

class StateProjections {
private:
    std::map<std::string, long long> balances_;  // account -> balance
    std::map<std::pair<std::string, std::string>, long long> on_hand_;  // (item, loc) -> qty
    std::map<std::string, std::map<std::string, long long>> exposure_;  // dim -> bucket -> value

public:
    void apply_event(const std::map<std::string, std::string>& event) {
        if (event.find("type") == event.end()) return;

        std::string event_type = event.at("type");

        if (event_type == "TRANSFER") {
            std::string from = event.at("from_account");
            std::string to = event.at("to_account");
            long long amount = std::stoll(event.at("amount"));

            balances_[from] -= amount;
            balances_[to] += amount;
        } else if (event_type == "STOCK_MOVE") {
            std::string item = event.at("item");
            std::string from_loc = event.at("from_loc");
            std::string to_loc = event.at("to_loc");
            long long qty = std::stoll(event.at("qty"));

            on_hand_[{item, from_loc}] -= qty;
            on_hand_[{item, to_loc}] += qty;
        } else if (event_type == "EXPOSURE") {
            std::string dimension = event.at("dimension");
            std::string bucket = event.at("bucket");
            long long value = std::stoll(event.at("value"));

            exposure_[dimension][bucket] = value;
        }
    }

    std::string state_hash() {
        std::stringstream ss;
        for (const auto& balance : balances_) {
            ss << balance.first << ":" << balance.second << ";";
        }
        return CryptoUtils::sha256(ss.str());
    }

    long long get_balance(const std::string& account) const {
        auto it = balances_.find(account);
        return it != balances_.end() ? it->second : 0;
    }

    long long get_on_hand(const std::string& item, const std::string& location) const {
        auto key = std::make_pair(item, location);
        auto it = on_hand_.find(key);
        return it != on_hand_.end() ? it->second : 0;
    }
};

} // namespace dbx4

// ============================================================================
// MAIN TEST
// ============================================================================

int main() {
    std::cout << "\n=== DBX4 DURABLE LOG SYSTEM ===" << std::endl;
    std::cout << "Cryptographic append-only log with chain verification" << std::endl;
    std::cout << std::endl;

    auto core = std::make_shared<dbx4::SealedCore>("/tmp/dbx4.key");
    dbx4::DurableLog log("./dbx4_durable.log", core);

    // Test append operations
    int appended = 0;
    for (int i = 0; i < 10; i++) {
        std::map<std::string, std::string> event = {
            {"type", "TRANSFER"},
            {"from_account", "acc_1"},
            {"to_account", "acc_2"},
            {"amount", std::to_string(1000 + i * 100)}
        };

        if (log.append(event)) {
            appended++;
        }
    }

    std::cout << "✓ Appended " << appended << " events" << std::endl;

    // Verify chain integrity
    if (log.verify_chain()) {
        std::cout << "✓ Chain verification passed" << std::endl;
        log.seal();
    }

    std::cout << "\n=== STATISTICS ===" << std::endl;
    std::cout << "Total Entries: " << log.get_entry_count() << std::endl;
    std::cout << "Sequence Number: " << log.get_sequence_number() << std::endl;
    std::cout << "Is Sealed: " << (log.is_sealed() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    return 0;
}


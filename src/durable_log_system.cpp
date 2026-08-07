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
#include <openssl/sha.h>
#include <openssl/evp.h>

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
};

// ============================================================================
// SEALED CORE - CRYPTOGRAPHIC SIGNING (from DPVS)
// ============================================================================

class SealedCore {
private:
    std::string key_path_;
    std::string signing_key_;
    bool is_initialized_;

public:
    SealedCore(const std::string& key_path) : key_path_(key_path), is_initialized_(false) {
        load_key();
    }

    std::string sign(const std::string& body) {
        if (!is_initialized_) return "";
        
        std::string canonical = CryptoUtils::canonicalize(parse_body(body));
        std::string signature = CryptoUtils::sha256(canonical + signing_key_);
        return signature;
    }

    bool verify(const std::string& body, const std::string& signature) {
        if (!is_initialized_) return false;
        
        std::string computed_sig = sign(body);
        return computed_sig == signature;
    }

private:
    void load_key() {
        std::ifstream key_file(key_path_);
        if (key_file.is_open()) {
            std::getline(key_file, signing_key_);
            is_initialized_ = !signing_key_.empty();
            key_file.close();
        }
    }

    std::map<std::string, std::string> parse_body(const std::string& body) {
        std::map<std::string, std::string> result;
        std::stringstream ss(body);
        std::string item;
        
        while (std::getline(ss, item, ';')) {
            size_t pos = item.find(':');
            if (pos != std::string::npos) {
                result[item.substr(0, pos)] = item.substr(pos + 1);
            }
        }
        return result;
    }
};

// ============================================================================
// DURABLE LOG - APPEND-ONLY WITH CHAIN VERIFICATION (from DPVS)
// ============================================================================

class LogEntry {
public:
    uint64_t seq;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> data;
    std::string signature;
    std::string prev_hash;
    std::string current_hash;
};

class DurableLog {
private:
    std::string log_path_;
    std::vector<LogEntry> entries_;
    std::shared_ptr<SealedCore> core_;
    uint64_t sequence_number_;
    bool is_sealed_;

public:
    DurableLog(const std::string& log_path, std::shared_ptr<SealedCore> core)
        : log_path_(log_path), core_(core), sequence_number_(0), is_sealed_(false) {
        load_from_disk();
    }

    bool append(const std::map<std::string, std::string>& event) {
        if (is_sealed_) return false;

        LogEntry entry;
        entry.seq = ++sequence_number_;
        entry.timestamp = std::chrono::system_clock::now();
        entry.data = event;

        // Build canonical form and sign
        std::string canonical = CryptoUtils::canonicalize(entry.data);
        entry.signature = core_->sign(canonical);

        // Chain hash: hash of (prev_hash || current_data)
        if (!entries_.empty()) {
            entry.prev_hash = entries_.back().current_hash;
        }
        entry.current_hash = CryptoUtils::sha256(entry.prev_hash + canonical);

        entries_.push_back(entry);
        persist_to_disk(entry);

        return true;
    }

    bool verify_chain() {
        if (entries_.empty()) return true;

        std::string expected_prev_hash = "";
        for (const auto& entry : entries_) {
            if (!expected_prev_hash.empty() && entry.prev_hash != expected_prev_hash) {
                return false;
            }

            std::string canonical = CryptoUtils::canonicalize(entry.data);
            std::string computed_hash = CryptoUtils::sha256(entry.prev_hash + canonical);

            if (computed_hash != entry.current_hash) {
                return false;
            }

            if (!core_->verify(canonical, entry.signature)) {
                return false;
            }

            expected_prev_hash = entry.current_hash;
        }
        return true;
    }

    void seal() {
        if (verify_chain()) {
            is_sealed_ = true;
        }
    }

    uint64_t get_entry_count() const { return entries_.size(); }
    uint64_t get_sequence_number() const { return sequence_number_; }
    bool is_sealed() const { return is_sealed_; }

private:
    void load_from_disk() {
        std::ifstream log_file(log_path_);
        if (!log_file.is_open()) return;

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


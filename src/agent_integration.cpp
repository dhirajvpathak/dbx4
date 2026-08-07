// ============================================================================
// DBX4 AGENT INTEGRATION LAYER
// Pluggable AI model with confidence gating (from live_agent.py pattern)
// ============================================================================

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <chrono>

namespace dbx4 {

// ============================================================================
// AGENT PROPOSAL - IMMUTABLE SUGGESTION (from live_agent pattern)
// ============================================================================

struct Proposal {
    std::string proposal_id;
    std::string agent_id;
    std::string model_version;
    std::vector<std::pair<std::string, std::string>> effects;  // account->amount
    float confidence;
    std::string rationale;
    bool requires_gate_approval;
    std::chrono::system_clock::time_point created_at;
};

// ============================================================================
// MATCH MODEL INTERFACE - PLUGGABLE AI BACKEND
// ============================================================================

class MatchModel {
public:
    virtual ~MatchModel() = default;
    
    struct MatchResult {
        std::string matched_id;
        float confidence;
        std::string explanation;
    };

    virtual std::string get_version() const = 0;
    virtual MatchResult match(const std::map<std::string, std::string>& invoice,
                            const std::vector<std::map<std::string, std::string>>& purchase_orders) = 0;
};

// ============================================================================
// MOCK MATCHER - DETERMINISTIC TEST IMPLEMENTATION
// ============================================================================

class MockMatcher : public MatchModel {
private:
    static float string_similarity(const std::string& s1, const std::string& s2) {
        std::string a = s1, b = s2;
        std::transform(a.begin(), a.end(), a.begin(), ::tolower);
        std::transform(b.begin(), b.end(), b.begin(), ::tolower);
        
        int matches = 0;
        for (size_t i = 0; i < std::min(a.length(), b.length()); i++) {
            if (a[i] == b[i]) matches++;
        }
        return static_cast<float>(matches) / std::max(a.length(), b.length());
    }

public:
    std::string get_version() const override { return "mock-matcher-v1"; }

    MatchResult match(const std::map<std::string, std::string>& invoice,
                     const std::vector<std::map<std::string, std::string>>& pos) override {
        MatchResult best{.confidence = -1.0f};

        for (const auto& po : pos) {
            float vendor_sim = string_similarity(
                invoice.at("vendor"), po.at("vendor")
            );

            float invoice_amt = std::stof(invoice.at("amount"));
            float po_amt = std::stof(po.at("amount"));
            float amt_gap = std::abs(invoice_amt - po_amt) / std::max(po_amt, 1.0f);
            float amt_sim = std::max(0.0f, 1.0f - amt_gap);

            float score = 0.95f * vendor_sim + 0.05f * amt_sim;  // vendor-dominant

            if (score > best.confidence) {
                best.matched_id = po.at("id");
                best.confidence = score;
                best.explanation = "vendor~" + invoice.at("vendor") + 
                                 " amount~" + invoice.at("amount") +
                                 " score=" + std::to_string(score);
            }
        }

        return best;
    }
};

// ============================================================================
// LIVE MATCH AGENT - PROPOSES WITHOUT COMMITTING
// ============================================================================

class LiveMatchAgent {
private:
    std::shared_ptr<MatchModel> model_;
    std::vector<std::map<std::string, std::string>> purchase_orders_;
    std::string agent_id_;
    std::vector<Proposal> memory_;
    
    uint64_t proposals_generated_;
    uint64_t proposals_accepted_;
    uint64_t proposals_rejected_;

public:
    LiveMatchAgent(std::shared_ptr<MatchModel> model,
                  const std::vector<std::map<std::string, std::string>>& pos,
                  const std::string& agent_id)
        : model_(model), purchase_orders_(pos), agent_id_(agent_id),
          proposals_generated_(0), proposals_accepted_(0), proposals_rejected_(0) {}

    Proposal on_invoice(const std::map<std::string, std::string>& invoice) {
        auto match_result = model_->match(invoice, purchase_orders_);

        Proposal proposal;
        proposal.proposal_id = "prop_" + std::to_string(proposals_generated_++);
        proposal.agent_id = agent_id_;
        proposal.model_version = model_->get_version();
        proposal.confidence = match_result.confidence;
        proposal.rationale = match_result.explanation;
        proposal.created_at = std::chrono::system_clock::now();

        // Find the matched PO and create accounting effects
        for (const auto& po : purchase_orders_) {
            if (po.at("id") == match_result.matched_id) {
                // GR/NI (goods received not invoiced) debit
                proposal.effects.push_back({
                    "GR_NI_debit", po.at("amount")
                });
                // AP (accounts payable) credit
                proposal.effects.push_back({
                    "AP_credit", invoice.at("amount")
                });
                break;
            }
        }

        // Gate approval required if confidence is below threshold
        proposal.requires_gate_approval = (proposal.confidence < 0.85f);

        memory_.push_back(proposal);
        return proposal;
    }

    bool accept_proposal(const std::string& proposal_id) {
        proposals_accepted_++;
        return true;
    }

    bool reject_proposal(const std::string& proposal_id, const std::string& reason) {
        proposals_rejected_++;
        return true;
    }

    std::vector<Proposal> get_pending_proposals() {
        std::vector<Proposal> pending;
        for (const auto& prop : memory_) {
            if (prop.requires_gate_approval) {
                pending.push_back(prop);
            }
        }
        return pending;
    }

    uint64_t get_proposals_generated() const { return proposals_generated_; }
    uint64_t get_proposals_accepted() const { return proposals_accepted_; }
    uint64_t get_proposals_rejected() const { return proposals_rejected_; }
};

// ============================================================================
// CONFIDENCE GATE - VALIDATES PROPOSALS BEFORE COMMITMENT
// ============================================================================

class ConfidenceGate {
private:
    float acceptance_threshold_;
    uint64_t accepted_count_;
    uint64_t rejected_count_;

public:
    ConfidenceGate(float threshold = 0.85f)
        : acceptance_threshold_(threshold), accepted_count_(0), rejected_count_(0) {}

    bool evaluate(const Proposal& proposal) {
        if (proposal.confidence >= acceptance_threshold_) {
            accepted_count_++;
            return true;
        } else {
            rejected_count_++;
            return false;
        }
    }

    std::string get_gate_reason(const Proposal& proposal) {
        if (proposal.confidence >= acceptance_threshold_) {
            return "APPROVED: confidence " + std::to_string(proposal.confidence) + 
                   " >= " + std::to_string(acceptance_threshold_);
        } else {
            return "REJECTED: confidence " + std::to_string(proposal.confidence) + 
                   " < " + std::to_string(acceptance_threshold_);
        }
    }

    uint64_t get_accepted() const { return accepted_count_; }
    uint64_t get_rejected() const { return rejected_count_; }
};

} // namespace dbx4

// ============================================================================
// MAIN TEST
// ============================================================================

int main() {
    std::cout << "\n=== DBX4 AGENT INTEGRATION LAYER ===" << std::endl;
    std::cout << "Pluggable AI model with confidence gating" << std::endl;
    std::cout << std::endl;

    // Create mock model
    auto model = std::make_shared<dbx4::MockMatcher>();
    std::cout << "Model Version: " << model->get_version() << std::endl << std::endl;

    // Create sample purchase orders
    std::vector<std::map<std::string, std::string>> pos = {
        {{"id", "PO_001"}, {"vendor", "Acme Corp"}, {"amount", "5000"}},
        {{"id", "PO_002"}, {"vendor", "Beta Inc"}, {"amount", "3000"}},
        {{"id", "PO_003"}, {"vendor", "Gamma LLC"}, {"amount", "7500"}}
    };

    // Create agent
    dbx4::LiveMatchAgent agent(model, pos, "match-agent-1");

    // Test invoices
    std::vector<std::map<std::string, std::string>> invoices = {
        {{"vendor", "Acme Corp"}, {"amount", "5000"}},  // Clean match
        {{"vendor", "Acme Corp"}, {"amount", "5100"}},  // Slight amount diff
        {{"vendor", "Unknown Vendor"}, {"amount", "9999"}}  // No match
    };

    // Process invoices
    int matched = 0;
    std::vector<dbx4::Proposal> proposals;

    for (const auto& invoice : invoices) {
        auto proposal = agent.on_invoice(invoice);
        proposals.push_back(proposal);
        std::cout << "✓ Generated proposal: " << proposal.proposal_id << std::endl;
        std::cout << "  Confidence: " << std::fixed << std::setprecision(2) 
                 << proposal.confidence << std::endl;
        std::cout << "  Rationale: " << proposal.rationale << std::endl;
    }

    // Gate evaluation
    dbx4::ConfidenceGate gate(0.85f);
    std::cout << "\n=== GATE EVALUATION ===" << std::endl;
    for (const auto& proposal : proposals) {
        bool approved = gate.evaluate(proposal);
        std::cout << "✓ " << gate.get_gate_reason(proposal) << std::endl;
        if (approved) {
            agent.accept_proposal(proposal.proposal_id);
            matched++;
        }
    }

    std::cout << "\n=== STATISTICS ===" << std::endl;
    std::cout << "Proposals Generated: " << agent.get_proposals_generated() << std::endl;
    std::cout << "Gate Accepted: " << gate.get_accepted() << std::endl;
    std::cout << "Gate Rejected: " << gate.get_rejected() << std::endl;
    std::cout << std::endl;

    return 0;
}


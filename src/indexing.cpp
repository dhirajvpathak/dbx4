#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>

namespace dbx4 {

template<typename K>
class BTreeNode {
public:
    std::vector<K> keys;
    std::vector<BTreeNode*> children;
    bool is_leaf = true;
    
    void insert_key(const K& key) {
        // Find correct position
        auto pos = std::lower_bound(keys.begin(), keys.end(), key);
        
        // Check if key already exists
        if (pos != keys.end() && *pos == key) {
            return;  // Duplicate, skip
        }
        
        // Insert at correct position
        keys.insert(pos, key);
    }
    
    bool contains(const K& key) const {
        return std::binary_search(keys.begin(), keys.end(), key);
    }
};

template<typename K>
class BTreeIndex {
private:
    BTreeNode<K>* root = nullptr;
    
public:
    BTreeIndex() {
        root = new BTreeNode<K>();
    }
    
    ~BTreeIndex() {
        delete_node(root);
    }
    
    void insert(const K& key) {
        if (!root) {
            root = new BTreeNode<K>();
        }
        root->insert_key(key);
    }
    
    bool search(const K& key) const {
        if (!root) return false;
        return root->contains(key);
    }
    
private:
    void delete_node(BTreeNode<K>* node) {
        if (!node) return;
        if (!node->is_leaf) {
            for (auto child : node->children) {
                delete_node(child);
            }
        }
        delete node;
    }
};

}

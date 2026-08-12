#ifndef DBX4_ARRAY_TYPE_H
#define DBX4_ARRAY_TYPE_H

#include <string>
#include <vector>
#include <iostream>

namespace dbx4 {

template<typename T>
class DBArray {
private:
    std::vector<T> elements_;
    std::string element_type_;
    
public:
    DBArray() {}
    
    DBArray(const std::string& type) : element_type_(type) {}
    
    void append(const T& element) {
        elements_.push_back(element);
    }
    
    // 1-based indexing (PostgreSQL style)
    T get_at(int index) {
        if (index >= 1 && index <= (int)elements_.size()) {
            return elements_[index - 1];
        }
        throw std::out_of_range("Array index out of bounds");
    }
    
    int length() const {
        return elements_.size();
    }
    
    // =ANY operator
    bool contains(const T& element) const {
        for (const auto& e : elements_) {
            if (e == element) return true;
        }
        return false;
    }
    
    // Array slice [start:end]
    DBArray slice(int start, int end) {
        DBArray result(element_type_);
        if (start >= 1 && end <= (int)elements_.size() && start <= end) {
            for (int i = start - 1; i < end; ++i) {
                result.elements_.push_back(elements_[i]);
            }
        }
        return result;
    }
    
    // Prepend element
    void prepend(const T& element) {
        elements_.insert(elements_.begin(), element);
    }
    
    // Remove element
    bool remove(const T& element) {
        for (auto it = elements_.begin(); it != elements_.end(); ++it) {
            if (*it == element) {
                elements_.erase(it);
                return true;
            }
        }
        return false;
    }
    
    int size() const {
        return elements_.size();
    }
    
    std::string to_string() const {
        std::string result = "ARRAY[";
        for (size_t i = 0; i < elements_.size(); ++i) {
            if (i > 0) result += ",";
            result += std::to_string(elements_[i]);
        }
        result += "]";
        return result;
    }
};

}

#endif

#pragma once
#include <exception>
#include <string>
#include <sstream>

namespace dbx4 {

// Base exception class for all DBX4 errors
class DBX4Exception : public std::exception {
protected:
    std::string message_;
public:
    explicit DBX4Exception(const std::string& msg) : message_(msg) {}
    virtual const char* what() const noexcept override {
        return message_.c_str();
    }
    virtual ~DBX4Exception() = default;
};

// SQL parsing and validation errors
class SQLParseException : public DBX4Exception {
public:
    explicit SQLParseException(const std::string& msg) 
        : DBX4Exception("SQL Parse Error: " + msg) {}
};

class SQLValidationException : public DBX4Exception {
public:
    explicit SQLValidationException(const std::string& msg)
        : DBX4Exception("SQL Validation Error: " + msg) {}
};

// Transaction-related errors
class TransactionException : public DBX4Exception {
public:
    explicit TransactionException(const std::string& msg)
        : DBX4Exception("Transaction Error: " + msg) {}
};

class DeadlockException : public TransactionException {
public:
    DeadlockException()
        : TransactionException("Deadlock detected - transaction rolled back") {}
};

// Storage and data integrity errors
class StorageException : public DBX4Exception {
public:
    explicit StorageException(const std::string& msg)
        : DBX4Exception("Storage Error: " + msg) {}
};

class CorruptDataException : public StorageException {
public:
    explicit CorruptDataException(const std::string& location)
        : StorageException("Corrupt data detected at: " + location) {}
};

// Constraint and data validation errors
class ConstraintException : public DBX4Exception {
public:
    explicit ConstraintException(const std::string& msg)
        : DBX4Exception("Constraint Violation: " + msg) {}
};

class UniqueConstraintException : public ConstraintException {
public:
    explicit UniqueConstraintException(const std::string& column)
        : ConstraintException("Duplicate value in unique column: " + column) {}
};

class NotNullException : public ConstraintException {
public:
    explicit NotNullException(const std::string& column)
        : ConstraintException("NOT NULL constraint violated in column: " + column) {}
};

class PrimaryKeyException : public ConstraintException {
public:
    explicit PrimaryKeyException(const std::string& msg)
        : ConstraintException("Primary key violation: " + msg) {}
};

// Type and conversion errors
class TypeException : public DBX4Exception {
public:
    explicit TypeException(const std::string& msg)
        : DBX4Exception("Type Error: " + msg) {}
};

class TypeCastException : public TypeException {
public:
    explicit TypeCastException(const std::string& from, const std::string& to)
        : TypeException("Cannot cast from " + from + " to " + to) {}
};

class OutOfRangeException : public TypeException {
public:
    explicit OutOfRangeException(const std::string& value, const std::string& type)
        : TypeException("Value " + value + " is out of range for type " + type) {}
};

// Index and access errors
class IndexException : public DBX4Exception {
public:
    explicit IndexException(const std::string& msg)
        : DBX4Exception("Index Error: " + msg) {}
};

class IndexOutOfBoundsException : public IndexException {
public:
    explicit IndexOutOfBoundsException(size_t index, size_t size)
        : IndexException("Index " + std::to_string(index) + 
                        " out of bounds for size " + std::to_string(size)) {}
};

// I/O and system errors
class IOException : public DBX4Exception {
public:
    explicit IOException(const std::string& msg)
        : DBX4Exception("I/O Error: " + msg) {}
};

class FileNotFoundException : public IOException {
public:
    explicit FileNotFoundException(const std::string& path)
        : IOException("File not found: " + path) {}
};

// Configuration errors
class ConfigurationException : public DBX4Exception {
public:
    explicit ConfigurationException(const std::string& msg)
        : DBX4Exception("Configuration Error: " + msg) {}
};

class InvalidStateException : public DBX4Exception {
public:
    explicit InvalidStateException(const std::string& state, const std::string& reason)
        : DBX4Exception("Invalid state '" + state + "': " + reason) {}
};

} // namespace dbx4


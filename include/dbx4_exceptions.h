#pragma once
#include <exception>
#include <string>
namespace dbx4 {
class DBX4Exception : public std::exception {
protected:
    std::string message_;
public:
    explicit DBX4Exception(const std::string& msg) : message_(msg) {}
    virtual const char* what() const noexcept override { return message_.c_str(); }
    virtual ~DBX4Exception() = default;
};
class SQLParseException : public DBX4Exception {
public:
    explicit SQLParseException(const std::string& msg) : DBX4Exception("SQL Parse Error: " + msg) {}
};
class SQLValidationException : public DBX4Exception {
public:
    explicit SQLValidationException(const std::string& msg) : DBX4Exception("SQL Validation Error: " + msg) {}
};
class StorageException : public DBX4Exception {
public:
    explicit StorageException(const std::string& msg) : DBX4Exception("Storage Error: " + msg) {}
};
class ConstraintException : public DBX4Exception {
public:
    explicit ConstraintException(const std::string& msg) : DBX4Exception("Constraint Violation: " + msg) {}
};
class NotNullException : public ConstraintException {
public:
    explicit NotNullException(const std::string& column) : ConstraintException("NOT NULL constraint violated in column: " + column) {}
};
class UniqueConstraintException : public ConstraintException {
public:
    explicit UniqueConstraintException(const std::string& column) : ConstraintException("Duplicate value in unique column: " + column) {}
};
class TypeException : public DBX4Exception {
public:
    explicit TypeException(const std::string& msg) : DBX4Exception("Type Error: " + msg) {}
};
class TypeCastException : public TypeException {
public:
    explicit TypeCastException(const std::string& from, const std::string& to) : TypeException("Cannot cast from " + from + " to " + to) {}
};
class OutOfRangeException : public TypeException {
public:
    explicit OutOfRangeException(const std::string& value, const std::string& type) : TypeException("Value " + value + " is out of range for type " + type) {}
};
class ConfigurationException : public DBX4Exception {
public:
    explicit ConfigurationException(const std::string& msg) : DBX4Exception("Configuration Error: " + msg) {}
};
class InvalidStateException : public DBX4Exception {
public:
    explicit InvalidStateException(const std::string& state, const std::string& reason) : DBX4Exception("Invalid state '" + state + "': " + reason) {}
};
}

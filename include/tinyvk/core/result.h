/**
 * @file result.h
 * @brief Result type for error handling in TinyVK
 */

#pragma once

#include <string>
#include <utility>

namespace tvk {

enum class ErrorCode {
    None = 0,
    Unknown,
    OutOfMemory,
    DeviceLost,
    InitializationFailed,
    VulkanError,
    InvalidArgument,
    ResourceCreationFailed,
    ShaderCompilationFailed,
    FileNotFound,
    InvalidState
};

struct Error {
    ErrorCode code = ErrorCode::None;
    std::string message;

    Error() = default;
    Error(ErrorCode c, std::string msg = "") : code(c), message(std::move(msg)) {}

    bool IsOk() const { return code == ErrorCode::None; }
    operator bool() const { return !IsOk(); }
};

template<typename T>
class Result {
public:
    Result(T value) : _value(std::move(value)), _error() {}
    Result(Error error) : _value(), _error(std::move(error)) {}
    Result(ErrorCode code, std::string msg = "") : _value(), _error(code, std::move(msg)) {}

    bool IsOk() const { return _error.IsOk(); }
    bool IsError() const { return !IsOk(); }
    operator bool() const { return IsOk(); }

    T& Value() { return _value; }
    const T& Value() const { return _value; }
    T* operator->() { return &_value; }
    const T* operator->() const { return &_value; }
    T& operator*() { return _value; }
    const T& operator*() const { return _value; }

    const Error& GetError() const { return _error; }
    ErrorCode Code() const { return _error.code; }
    const std::string& Message() const { return _error.message; }

    T ValueOr(T default_value) const {
        return IsOk() ? _value : default_value;
    }

private:
    T _value;
    Error _error;
};

template<>
class Result<void> {
public:
    Result() : _error() {}
    Result(Error error) : _error(std::move(error)) {}
    Result(ErrorCode code, std::string msg = "") : _error(code, std::move(msg)) {}

    bool IsOk() const { return _error.IsOk(); }
    bool IsError() const { return !IsOk(); }
    operator bool() const { return IsOk(); }

    const Error& GetError() const { return _error; }
    ErrorCode Code() const { return _error.code; }
    const std::string& Message() const { return _error.message; }

private:
    Error _error;
};

inline Result<void> Ok() { return Result<void>(); }

template<typename T>
inline Result<T> Ok(T value) { return Result<T>(std::move(value)); }

inline Error Err(ErrorCode code, std::string msg = "") { return Error(code, std::move(msg)); }

#define TVK_TRY(expr) \
    do { \
        auto _result = (expr); \
        if (_result.IsError()) { \
            return _result.GetError(); \
        } \
    } while(0)

#define TVK_TRY_ASSIGN(var, expr) \
    auto _result_##var = (expr); \
    if (_result_##var.IsError()) { \
        return _result_##var.GetError(); \
    } \
    var = std::move(_result_##var.Value())

} // namespace tvk


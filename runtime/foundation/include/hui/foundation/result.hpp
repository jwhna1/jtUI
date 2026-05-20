#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace hui {

enum class StatusCode : std::uint32_t {
    Ok = 0,
    InvalidArgument = 1,
    NotImplemented = 2,
    PlatformError = 3,
};

class Status {
public:
    Status() = default;

    Status(StatusCode code, std::string message = {})
        : code_(code), message_(std::move(message)) {
    }

    [[nodiscard]] static Status invalid_argument(std::string message) {
        return Status {StatusCode::InvalidArgument, std::move(message)};
    }

    [[nodiscard]] static Status not_implemented(std::string message) {
        return Status {StatusCode::NotImplemented, std::move(message)};
    }

    [[nodiscard]] static Status platform_error(std::string message) {
        return Status {StatusCode::PlatformError, std::move(message)};
    }

    [[nodiscard]] bool is_ok() const noexcept {
        return code_ == StatusCode::Ok;
    }

    [[nodiscard]] StatusCode code() const noexcept {
        return code_;
    }

    [[nodiscard]] const std::string& message() const noexcept {
        return message_;
    }

    explicit operator bool() const noexcept {
        return is_ok();
    }

private:
    StatusCode code_ {StatusCode::Ok};
    std::string message_ {};
};

template <typename T>
class Result {
public:
    Result(T value)
        : value_(std::move(value)) {
    }

    Result(Status status)
        : status_(std::move(status)) {
    }

    [[nodiscard]] bool ok() const noexcept {
        return status_.is_ok();
    }

    [[nodiscard]] const Status& status() const noexcept {
        return status_;
    }

    [[nodiscard]] T& value() {
        if (!value_.has_value()) {
            throw std::logic_error("Result does not contain a value");
        }
        return *value_;
    }

    [[nodiscard]] const T& value() const {
        if (!value_.has_value()) {
            throw std::logic_error("Result does not contain a value");
        }
        return *value_;
    }

    explicit operator bool() const noexcept {
        return ok();
    }

private:
    std::optional<T> value_ {};
    Status status_ {};
};

}  // namespace hui

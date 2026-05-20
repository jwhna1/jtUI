#pragma once

#include <utility>

#include "hui/property/signal.hpp"

namespace hui {

template <typename T>
class Property {
public:
    Property() = default;

    explicit Property(T value)
        : value_(std::move(value)) {
    }

    [[nodiscard]] const T& get() const noexcept {
        return value_;
    }

    bool set(T value) {
        if (value_ == value) {
            return false;
        }

        value_ = std::move(value);
        changed_.emit(value_);
        return true;
    }

    [[nodiscard]] Signal<const T&>& changed() noexcept {
        return changed_;
    }

private:
    T value_ {};
    Signal<const T&> changed_ {};
};

}  // namespace hui

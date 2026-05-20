#pragma once

#include <functional>
#include <utility>

namespace hui {

class Command {
public:
    using Handler = std::function<void()>;

    Command() = default;

    explicit Command(Handler handler)
        : handler_(std::move(handler)) {
    }

    void set_handler(Handler handler) {
        handler_ = std::move(handler);
    }

    void execute() const {
        if (handler_) {
            handler_();
        }
    }

    [[nodiscard]] bool valid() const noexcept {
        return static_cast<bool>(handler_);
    }

    explicit operator bool() const noexcept {
        return valid();
    }

private:
    Handler handler_ {};
};

}  // namespace hui

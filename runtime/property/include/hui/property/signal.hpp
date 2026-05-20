#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace hui {

template <typename... Args>
class Signal {
public:
    using Slot = std::function<void(Args...)>;
    using ConnectionId = std::size_t;

    ConnectionId connect(Slot slot) {
        const ConnectionId id = next_id_++;
        slots_.push_back({id, std::move(slot)});
        return id;
    }

    void disconnect(ConnectionId id) {
        slots_.erase(
            std::remove_if(
                slots_.begin(),
                slots_.end(),
                [id](const auto& entry) { return entry.id == id; }),
            slots_.end());
    }

    void emit(Args... args) {
        for (const auto& slot : slots_) {
            slot.callback(args...);
        }
    }

private:
    struct Entry {
        ConnectionId id;
        Slot callback;
    };

    ConnectionId next_id_ {1};
    std::vector<Entry> slots_ {};
};

}  // namespace hui

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "hui/object/widget.hpp"

namespace hui {

class ComponentView {
public:
    void set_root(std::unique_ptr<Widget> root) {
        root_ = std::move(root);
    }

    [[nodiscard]] Widget* root() const noexcept {
        return root_.get();
    }

    void set_string(std::string key, std::string value) {
        string_properties_[std::move(key)] = std::move(value);
    }

    [[nodiscard]] std::string_view string(std::string_view key) const {
        const auto it = string_properties_.find(std::string {key});
        if (it == string_properties_.end()) {
            return {};
        }
        return it->second;
    }

private:
    std::unique_ptr<Widget> root_ {};
    std::unordered_map<std::string, std::string> string_properties_ {};
};

}  // namespace hui

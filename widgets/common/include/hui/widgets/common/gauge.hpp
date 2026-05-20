#pragma once

#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"

// jtUI Gauge
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

namespace hui {

class Gauge : public Widget {
  public:
    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "Gauge";
    }

    [[nodiscard]] float minimum() const noexcept {
        return minimum_;
    }

    [[nodiscard]] float maximum() const noexcept {
        return maximum_;
    }

    [[nodiscard]] float value() const noexcept {
        return value_;
    }

    void set_range(float minimum, float maximum) noexcept;

    void set_value(float value) noexcept;

    [[nodiscard]] const std::string& label() const noexcept {
        return label_;
    }

    void set_label(std::string label);

    void set_tone(theme::Tone tone) noexcept;

    [[nodiscard]] bool tick(float delta_seconds) override;

    void paint(PaintContext& context) const override;

  private:
    float minimum_{0.0F};
    float maximum_{1.0F};
    float value_{0.0F};
    float displayed_value_{0.0F};
    float animation_speed_{8.0F};
    bool initialized_{false};
    std::string label_{"Gauge"};
    theme::Tone tone_{theme::Tone::Accent};
};

} // namespace hui

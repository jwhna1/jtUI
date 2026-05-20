#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "hui/foundation/geometry.hpp"
#include "hui/layout/layout_types.hpp"
#include "hui/object/widget.hpp"

// jtUI FlexBox 容器
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>），创建于 2026-04-24
//
// 两遍 measure/arrange：
//   pass 1: 对每个 child 按其 FlexItem.main 策略确定主轴预算
//     - Auto：child.measure(available) 报的主轴尺寸
//     - Pixel：按固定 px
//     - Fill：先不给尺寸，收集总权重
//   pass 2: 把剩余主轴空间按 Fill 权重分配，依次 arrange 每个 child
// 交叉轴默认 stretch，如 FlexItem.cross_stretch = false 则走 measured 尺寸
// 不支持 wrap / flex-basis / min-max 边界，保持最小

namespace hui {

enum class FlexAlign : std::uint8_t {
    Start,
    Center,
    End,
    Stretch,
};

enum class FlexJustify : std::uint8_t {
    Start,
    Center,
    End,
    SpaceBetween,
};

struct FlexItem {
    Length main{Length::auto_value()};
    bool cross_stretch{true};
};

class FlexBox : public Widget {
  public:
    FlexBox() = default;

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "FlexBox";
    }

    void set_direction(LayoutDirection dir) noexcept {
        direction_ = dir;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }

    [[nodiscard]] LayoutDirection direction() const noexcept {
        return direction_;
    }

    void set_gap(float gap) noexcept {
        gap_ = gap;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }

    void set_padding(Edges padding) noexcept {
        padding_ = padding;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }

    void set_align(FlexAlign align) noexcept {
        align_ = align;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }

    void set_justify(FlexJustify justify) noexcept {
        justify_ = justify;
        mark_dirty(DirtyFlags::Layout | DirtyFlags::Paint);
    }

    // 追加子 widget；item 控制主轴策略和交叉轴 stretch
    Widget& append_flex(std::unique_ptr<Widget> child, FlexItem item = {});

    [[nodiscard]] SizeF measure(SizeF available) const override;

    void arrange(RectF frame_in) override;

  private:
    LayoutDirection direction_{LayoutDirection::Row};
    float gap_{0.0F};
    Edges padding_{};
    FlexAlign align_{FlexAlign::Stretch};
    FlexJustify justify_{FlexJustify::Start};
    std::vector<FlexItem> items_{};
};

} // namespace hui

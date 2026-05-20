// Package: hui::widgets::basic
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
//
// CodiconIcon: 用 @vscode/codicons 字体渲染单个 codicon glyph。
//
// 使用模式 (Tree / StatusBar / ide widget):
//   auto icon = std::make_unique<CodiconIcon>("folder");
//   icon->set_size_px(16.0F);
//   icon->set_color(theme::vscode::semantic().sidebar_fg);
//
// 跨平台行为:
//   Win32: platform/win32/font/codicon_font_loader 已把 ttf 注册为 family "codicon",
//          DirectWrite 直接渲染对应 codepoint glyph。
//   Linux / 未注册环境: family "codicon" 找不到 DirectWrite fallback 会返回空 glyph,
//          paint 视觉上画一个占位 stroke rect, 让开发者眼能看到 icon 位置。
//
// 命名: name 用 codicon 官方 css class 去掉 "codicon-" 前缀, 如 "folder" / "git-fetch"
// / "settings-gear"。完整列表见 codicon_codepoints.hpp 或 @vscode/codicons npm 包。
// 未知 name -> paint 仅画占位 rect。
#pragma once

#include <string>
#include <utility>

#include "hui/foundation/color.hpp"
#include "hui/foundation/codicon_font.hpp"
#include "hui/foundation/codicon_lookup.hpp"
#include "hui/object/widget.hpp"
#include "hui/render/paint_context.hpp"

namespace hui {

class CodiconIcon : public Widget {
public:
    CodiconIcon() = default;

    explicit CodiconIcon(std::string name)
        : name_(std::move(name)) {
    }

    [[nodiscard]] std::string_view type_name() const noexcept override {
        return "CodiconIcon";
    }

    void set_name(std::string name) {
        if (name_ == name) {
            return;
        }
        name_ = std::move(name);
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] const std::string& name() const noexcept {
        return name_;
    }

    void set_size_px(float size_px) noexcept {
        if (size_px <= 0.0F) {
            return;
        }
        size_px_ = size_px;
        mark_dirty(DirtyFlags::Layout);
    }

    [[nodiscard]] float size_px() const noexcept {
        return size_px_;
    }

    void set_color(Color color) noexcept {
        color_ = color;
        color_overridden_ = true;
        mark_dirty(DirtyFlags::Paint);
    }

    void clear_color_override() noexcept {
        color_overridden_ = false;
        mark_dirty(DirtyFlags::Paint);
    }

    [[nodiscard]] SizeF measure(SizeF /*available*/) const override {
        return SizeF{size_px_, size_px_};
    }

    void paint(PaintContext& context) const override {
        const auto codepoint_opt = foundation::codicon_codepoint_for(name_);
        const Color glyph_color = resolve_color();

        if (!codepoint_opt.has_value()) {
            // 未知 name: 画一个占位描边 rect, 提醒开发者写错图标名。
            context.stroke_rect(frame(), glyph_color, 1.0F);
            return;
        }

        std::string utf8;
        encode_utf8(*codepoint_opt, utf8);

        // codicon glyph 居中显示。font_size 与 widget 视觉尺寸相同 (1:1)。
        context.draw_text_with_font(
            frame(), std::move(utf8),
            std::string(foundation::kCodiconFontFamily),
            glyph_color,
            TextAlignment::Center,
            size_px_,
            /*bold=*/false);
    }

private:
    [[nodiscard]] Color resolve_color() const noexcept {
        if (color_overridden_) return color_;
        // 默认使用主文本色。调用方通常会 set_color 覆盖。
        return Color{0.92F, 0.92F, 0.92F, 1.0F};
    }

    // 把 char32_t codepoint (codicon 区间 0xEA60-0xF200) 编码为 UTF-8.
    // 这个区间在 U+0800 到 U+FFFF 之间, 占 3 字节。简化实现, 不处理 surrogate。
    static void encode_utf8(char32_t codepoint, std::string& out) {
        out.clear();
        if (codepoint < 0x80U) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint < 0x800U) {
            out.push_back(static_cast<char>(0xC0U | (codepoint >> 6U)));
            out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        } else if (codepoint < 0x10000U) {
            out.push_back(static_cast<char>(0xE0U | (codepoint >> 12U)));
            out.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
            out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        } else {
            out.push_back(static_cast<char>(0xF0U | (codepoint >> 18U)));
            out.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
            out.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
            out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        }
    }

    std::string name_{};
    float size_px_{16.0F};
    Color color_{};
    bool color_overridden_{false};
};

}  // namespace hui

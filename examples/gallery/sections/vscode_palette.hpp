// Package: jtUI gallery
// Author:   jtai团队 :（曾能混&tang先森）<jwhna1@gmail.com>
// https://jtai.cc
#pragma once

#include <memory>

#include "gallery_context.hpp"
#include "jtui/jtui.hpp"

namespace jtui::gallery {

class VsCodePaletteSection {
public:
    VsCodePaletteSection();
    ~VsCodePaletteSection();
    [[nodiscard]] std::unique_ptr<jtui::Panel> take_root();
    void apply_layout(const SectionLayout& area);
    void apply_labels(Locale locale);

private:
    std::unique_ptr<jtui::Panel> root_;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace jtui::gallery

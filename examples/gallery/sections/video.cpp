#include "sections/video.hpp"

#include <memory>
#include <utility>

#include "jtui/jtui.hpp"

namespace jtui::gallery {

struct VideoSection::Impl {
    jtui::Text* title {nullptr};
    jtui::Text* caption {nullptr};
    jtui::VideoPlayer* player {nullptr};
    jtui::Button* play_btn {nullptr};
    jtui::Button* stop_btn {nullptr};
};

VideoSection::VideoSection(std::string source_path)
    : root_(std::make_unique<jtui::Panel>()),
      impl_(std::make_unique<Impl>()),
      source_path_(std::move(source_path)) {
    root_->set_role(jtui::PanelRole::Surface);

    auto title = std::make_unique<jtui::Text>();
    title->set_preset(jtui::TextStylePreset::Heading);
    impl_->title = title.get();
    root_->append_child(std::move(title));

    auto caption = std::make_unique<jtui::Text>();
    caption->set_preset(jtui::TextStylePreset::Caption);
    caption->set_role(jtui::TextRole::Muted);
    impl_->caption = caption.get();
    root_->append_child(std::move(caption));

    auto player = std::make_unique<jtui::VideoPlayer>();
    impl_->player = player.get();
    if (!source_path_.empty()) {
        impl_->player->set_source(source_path_);
    }
    root_->append_child(std::move(player));

    auto play_btn = std::make_unique<jtui::Button>();
    play_btn->set_variant(jtui::ButtonVariant::Filled);
    play_btn->set_leading_icon("\xe2\x96\xb6");
    jtui::VideoPlayer* p = impl_->player;
    play_btn->on_clicked().connect([p]() {
        if (p->state() == jtui::VideoPlayer::PlayState::Playing) {
            p->pause();
        } else {
            p->play();
        }
    });
    impl_->play_btn = play_btn.get();
    root_->append_child(std::move(play_btn));

    auto stop_btn = std::make_unique<jtui::Button>();
    stop_btn->set_variant(jtui::ButtonVariant::Outlined);
    stop_btn->on_clicked().connect([p]() { p->stop(); });
    impl_->stop_btn = stop_btn.get();
    root_->append_child(std::move(stop_btn));
}

VideoSection::~VideoSection() = default;

std::unique_ptr<jtui::Panel> VideoSection::take_root() {
    return std::move(root_);
}

void VideoSection::apply_layout(const SectionLayout& area) {
    impl_->title->set_frame(jtui::RectF{area.x, area.y, area.width, 28.0F});
    impl_->caption->set_frame(jtui::RectF{area.x, area.y + 32.0F, area.width, 18.0F});

    const float player_h = area.height - 120.0F;
    impl_->player->set_frame(jtui::RectF{area.x, area.y + 64.0F, area.width, player_h});

    const float btn_y = area.y + 64.0F + player_h + 16.0F;
    impl_->play_btn->set_frame(jtui::RectF{area.x, btn_y, 140.0F, 40.0F});
    impl_->stop_btn->set_frame(jtui::RectF{area.x + 152.0F, btn_y, 120.0F, 40.0F});
}

void VideoSection::apply_labels(Locale loc) {
    const bool zh = loc == Locale::Chinese;
    impl_->title->set_content(zh ? "视频播放" : "Video Player");
    impl_->caption->set_content(zh
        ? "素材：" + source_path_
        : "Source: " + source_path_);
    impl_->play_btn->set_text(zh ? "播放 / 暂停" : "Play / Pause");
    impl_->stop_btn->set_text(zh ? "停止" : "Stop");
}

}  // namespace jtui::gallery

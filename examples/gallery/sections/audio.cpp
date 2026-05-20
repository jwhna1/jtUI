#include "sections/audio.hpp"

#include <memory>
#include <utility>

#include "jtui/jtui.hpp"

namespace jtui::gallery {

struct AudioSection::Impl {
    jtui::Text* title {nullptr};
    jtui::Text* caption {nullptr};
    jtui::WaveformView* waveform {nullptr};
    jtui::AudioPlayer* player {nullptr};
    jtui::Button* play_btn {nullptr};
    jtui::Button* stop_btn {nullptr};
    jtui::Text* volume_label {nullptr};
    jtui::Slider* volume {nullptr};
};

AudioSection::AudioSection(std::string source_path)
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

    auto waveform = std::make_unique<jtui::WaveformView>();
    impl_->waveform = waveform.get();
    root_->append_child(std::move(waveform));

    auto player = std::make_unique<jtui::AudioPlayer>();
    impl_->player = player.get();
    if (!source_path_.empty()) {
        impl_->player->set_source(source_path_);
        impl_->waveform->set_peaks(impl_->player->peaks());
    }
    jtui::AudioPlayer* ap = impl_->player;
    jtui::WaveformView* wv = impl_->waveform;
    ap->on_position_changed().connect([wv, ap](double pos) {
        const double dur = ap->duration();
        const float p = dur > 0.0 ? static_cast<float>(pos / dur) : 0.0F;
        wv->set_progress(p);
    });
    root_->append_child(std::move(player));

    auto play_btn = std::make_unique<jtui::Button>();
    play_btn->set_variant(jtui::ButtonVariant::Filled);
    play_btn->set_leading_icon("\xe2\x96\xb6");
    play_btn->on_clicked().connect([ap]() {
        if (ap->state() == jtui::AudioPlayer::PlayState::Playing) ap->pause();
        else ap->play();
    });
    impl_->play_btn = play_btn.get();
    root_->append_child(std::move(play_btn));

    auto stop_btn = std::make_unique<jtui::Button>();
    stop_btn->set_variant(jtui::ButtonVariant::Outlined);
    stop_btn->on_clicked().connect([ap]() { ap->stop(); });
    impl_->stop_btn = stop_btn.get();
    root_->append_child(std::move(stop_btn));

    auto vol_label = std::make_unique<jtui::Text>();
    vol_label->set_preset(jtui::TextStylePreset::Label);
    vol_label->set_role(jtui::TextRole::Secondary);
    impl_->volume_label = vol_label.get();
    root_->append_child(std::move(vol_label));

    auto volume = std::make_unique<jtui::Slider>();
    volume->set_value(0.8F);
    volume->on_changed().connect([ap](float v) { ap->set_volume(v); });
    impl_->volume = volume.get();
    root_->append_child(std::move(volume));
}

AudioSection::~AudioSection() = default;

std::unique_ptr<jtui::Panel> AudioSection::take_root() {
    return std::move(root_);
}

void AudioSection::apply_layout(const SectionLayout& area) {
    impl_->title->set_frame(jtui::RectF{area.x, area.y, area.width, 28.0F});
    impl_->caption->set_frame(jtui::RectF{area.x, area.y + 32.0F, area.width, 18.0F});

    impl_->waveform->set_frame(jtui::RectF{area.x, area.y + 64.0F, area.width, 140.0F});
    impl_->player->set_frame(jtui::RectF{area.x, area.y + 220.0F, area.width, 72.0F});

    const float btn_y = area.y + 308.0F;
    impl_->play_btn->set_frame(jtui::RectF{area.x, btn_y, 140.0F, 40.0F});
    impl_->stop_btn->set_frame(jtui::RectF{area.x + 152.0F, btn_y, 120.0F, 40.0F});

    impl_->volume_label->set_frame(jtui::RectF{area.x + 292.0F, btn_y + 10.0F, 80.0F, 20.0F});
    impl_->volume->set_frame(jtui::RectF{area.x + 372.0F, btn_y + 8.0F, 220.0F, 24.0F});
}

void AudioSection::apply_labels(Locale loc) {
    const bool zh = loc == Locale::Chinese;
    impl_->title->set_content(zh ? "音频播放" : "Audio Player");
    impl_->caption->set_content(
        (zh ? std::string{"素材："} : std::string{"Source: "}) + source_path_);
    impl_->play_btn->set_text(zh ? "播放 / 暂停" : "Play / Pause");
    impl_->stop_btn->set_text(zh ? "停止" : "Stop");
    impl_->volume_label->set_content(zh ? "音量" : "Volume");
}

}  // namespace jtui::gallery

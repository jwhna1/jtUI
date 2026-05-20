#include "sections/gauges.hpp"

#include <memory>
#include <utility>

#include "jtui/jtui.hpp"

namespace jtui::gallery {

struct GaugesSection::Impl {
    jtui::Text* title_linear {nullptr};
    jtui::Text* title_semi {nullptr};
    jtui::Text* title_radial {nullptr};
    jtui::Text* title_progress {nullptr};
    jtui::Text* title_metrics {nullptr};

    jtui::Gauge* linear_a {nullptr};
    jtui::Gauge* linear_b {nullptr};

    jtui::SemiGauge* semi_60 {nullptr};
    jtui::SemiGauge* semi_80 {nullptr};

    jtui::RadialGauge* radial_cpu {nullptr};
    jtui::RadialGauge* radial_mem {nullptr};
    jtui::RadialGauge* radial_stream {nullptr};

    jtui::ProgressBar* bar_accent {nullptr};
    jtui::ProgressBar* bar_warning {nullptr};
    jtui::ProgressBar* bar_danger {nullptr};
    jtui::ProgressBar* bar_indeterminate {nullptr};

    jtui::MetricCard* card_throughput {nullptr};
    jtui::MetricCard* card_latency {nullptr};
    jtui::MetricCard* card_sessions {nullptr};
};

GaugesSection::GaugesSection()
    : root_(std::make_unique<jtui::Panel>()),
      impl_(std::make_unique<Impl>()) {
    root_->set_role(jtui::PanelRole::Surface);

    auto make_title = [this]() {
        auto t = std::make_unique<jtui::Text>();
        t->set_preset(jtui::TextStylePreset::Label);
        t->set_role(jtui::TextRole::Secondary);
        jtui::Text* ptr = t.get();
        root_->append_child(std::move(t));
        return ptr;
    };
    impl_->title_linear = make_title();
    impl_->title_semi = make_title();
    impl_->title_radial = make_title();
    impl_->title_progress = make_title();
    impl_->title_metrics = make_title();

    // Linear
    {
        auto a = std::make_unique<jtui::Gauge>();
        a->set_value(0.62F);
        a->set_tone(jtui::theme::Tone::Accent);
        impl_->linear_a = a.get();
        root_->append_child(std::move(a));

        auto b = std::make_unique<jtui::Gauge>();
        b->set_value(0.41F);
        b->set_tone(jtui::theme::Tone::Warning);
        impl_->linear_b = b.get();
        root_->append_child(std::move(b));
    }

    // Semi
    {
        auto s60 = std::make_unique<jtui::SemiGauge>();
        s60->set_value(0.6F);
        s60->set_tone(jtui::theme::Tone::Accent);
        impl_->semi_60 = s60.get();
        root_->append_child(std::move(s60));

        auto s80 = std::make_unique<jtui::SemiGauge>();
        s80->set_value(0.8F);
        s80->set_tone(jtui::theme::Tone::Warning);
        impl_->semi_80 = s80.get();
        root_->append_child(std::move(s80));
    }

    // Radial
    {
        auto cpu = std::make_unique<jtui::RadialGauge>();
        cpu->set_value(0.62F);
        cpu->set_tone(jtui::theme::Tone::Accent);
        impl_->radial_cpu = cpu.get();
        root_->append_child(std::move(cpu));

        auto mem = std::make_unique<jtui::RadialGauge>();
        mem->set_value(0.74F);
        mem->set_tone(jtui::theme::Tone::Info);
        impl_->radial_mem = mem.get();
        root_->append_child(std::move(mem));

        auto st = std::make_unique<jtui::RadialGauge>();
        st->set_value(0.91F);
        st->set_tone(jtui::theme::Tone::Warning);
        impl_->radial_stream = st.get();
        root_->append_child(std::move(st));
    }

    // Progress
    {
        auto a = std::make_unique<jtui::ProgressBar>();
        a->set_value(0.72F);
        a->set_tone(jtui::theme::Tone::Accent);
        impl_->bar_accent = a.get();
        root_->append_child(std::move(a));

        auto w = std::make_unique<jtui::ProgressBar>();
        w->set_value(0.45F);
        w->set_tone(jtui::theme::Tone::Warning);
        impl_->bar_warning = w.get();
        root_->append_child(std::move(w));

        auto d = std::make_unique<jtui::ProgressBar>();
        d->set_value(0.2F);
        d->set_tone(jtui::theme::Tone::Danger);
        impl_->bar_danger = d.get();
        root_->append_child(std::move(d));

        auto ind = std::make_unique<jtui::ProgressBar>();
        ind->set_mode(jtui::ProgressBarMode::Indeterminate);
        ind->set_tone(jtui::theme::Tone::Info);
        impl_->bar_indeterminate = ind.get();
        root_->append_child(std::move(ind));
    }

    // Metric cards
    {
        auto t = std::make_unique<jtui::MetricCard>();
        t->set_value("18.4k");
        t->set_tone(jtui::theme::Tone::Accent);
        impl_->card_throughput = t.get();
        root_->append_child(std::move(t));

        auto l = std::make_unique<jtui::MetricCard>();
        l->set_value("24ms");
        l->set_tone(jtui::theme::Tone::Warning);
        impl_->card_latency = l.get();
        root_->append_child(std::move(l));

        auto s = std::make_unique<jtui::MetricCard>();
        s->set_value("326");
        s->set_tone(jtui::theme::Tone::Info);
        impl_->card_sessions = s.get();
        root_->append_child(std::move(s));
    }
}

GaugesSection::~GaugesSection() = default;

std::unique_ptr<jtui::Panel> GaugesSection::take_root() {
    return std::move(root_);
}

void GaugesSection::apply_layout(const SectionLayout& area) {
    const float col_w = (area.width - 40.0F) * 0.5F;
    const float col_gap = 40.0F;
    const float left_x = area.x;
    const float right_x = area.x + col_w + col_gap;
    float left_y = area.y;
    float right_y = area.y;

    // 左列：linear + semi + progress
    impl_->title_linear->set_frame(jtui::RectF{left_x, left_y, col_w, 20.0F});
    left_y += 24.0F;
    impl_->linear_a->set_frame(jtui::RectF{left_x, left_y, col_w, 72.0F});
    left_y += 80.0F;
    impl_->linear_b->set_frame(jtui::RectF{left_x, left_y, col_w, 72.0F});
    left_y += 96.0F;

    impl_->title_semi->set_frame(jtui::RectF{left_x, left_y, col_w, 20.0F});
    left_y += 24.0F;
    const float semi_w = (col_w - 16.0F) * 0.5F;
    impl_->semi_60->set_frame(jtui::RectF{left_x, left_y, semi_w, 150.0F});
    impl_->semi_80->set_frame(jtui::RectF{left_x + semi_w + 16.0F, left_y, semi_w, 150.0F});
    left_y += 170.0F;

    impl_->title_progress->set_frame(jtui::RectF{left_x, left_y, col_w, 20.0F});
    left_y += 24.0F;
    const float bar_h = 28.0F;
    for (jtui::ProgressBar* bar : {impl_->bar_accent, impl_->bar_warning,
                                   impl_->bar_danger, impl_->bar_indeterminate}) {
        bar->set_frame(jtui::RectF{left_x, left_y, col_w, bar_h});
        left_y += bar_h + 10.0F;
    }

    // 右列：radial + metrics
    impl_->title_radial->set_frame(jtui::RectF{right_x, right_y, col_w, 20.0F});
    right_y += 24.0F;
    const float rad_w = (col_w - 20.0F) / 3.0F;
    impl_->radial_cpu->set_frame(jtui::RectF{right_x, right_y, rad_w, 170.0F});
    impl_->radial_mem->set_frame(jtui::RectF{right_x + rad_w + 10.0F, right_y, rad_w, 170.0F});
    impl_->radial_stream->set_frame(
        jtui::RectF{right_x + (rad_w + 10.0F) * 2.0F, right_y, rad_w, 170.0F});
    right_y += 190.0F;

    impl_->title_metrics->set_frame(jtui::RectF{right_x, right_y, col_w, 20.0F});
    right_y += 24.0F;
    impl_->card_throughput->set_frame(jtui::RectF{right_x, right_y, rad_w, 112.0F});
    impl_->card_latency->set_frame(jtui::RectF{right_x + rad_w + 10.0F, right_y, rad_w, 112.0F});
    impl_->card_sessions->set_frame(
        jtui::RectF{right_x + (rad_w + 10.0F) * 2.0F, right_y, rad_w, 112.0F});
}

void GaugesSection::apply_labels(Locale loc) {
    const bool zh = loc == Locale::Chinese;

    impl_->title_linear->set_content(zh ? "线性仪表" : "Linear Gauges");
    impl_->title_semi->set_content(zh ? "半圆仪表" : "Semi Gauges");
    impl_->title_radial->set_content(zh ? "圆环仪表" : "Radial Gauges");
    impl_->title_progress->set_content(zh ? "进度条" : "Progress Bars");
    impl_->title_metrics->set_content(zh ? "指标卡片" : "Metric Cards");

    impl_->linear_a->set_label(zh ? "缓冲" : "Buffer");
    impl_->linear_b->set_label(zh ? "编码" : "Encoder");

    impl_->semi_60->set_label(zh ? "CPU" : "CPU");
    impl_->semi_80->set_label(zh ? "内存" : "Memory");

    impl_->radial_cpu->set_label(zh ? "CPU 负载" : "CPU Load");
    impl_->radial_mem->set_label(zh ? "内存水位" : "Memory Level");
    impl_->radial_stream->set_label(zh ? "流健康度" : "Stream Health");

    impl_->bar_accent->set_label(zh ? "导出进度 72%" : "Export 72%");
    impl_->bar_warning->set_label(zh ? "磁盘占用 45%" : "Disk 45%");
    impl_->bar_danger->set_label(zh ? "配额余额 20%" : "Quota 20%");
    impl_->bar_indeterminate->set_label(zh ? "同步中..." : "Syncing...");

    impl_->card_throughput->set_title(zh ? "吞吐量" : "Throughput");
    impl_->card_throughput->set_caption(zh ? "比上周 +12%" : "+12% vs last week");
    impl_->card_latency->set_title(zh ? "延迟" : "Latency");
    impl_->card_latency->set_caption(zh ? "端到端平均" : "End-to-end avg");
    impl_->card_sessions->set_title(zh ? "会话" : "Sessions");
    impl_->card_sessions->set_caption(zh ? "在线设备" : "Online devices");
}

}  // namespace jtui::gallery

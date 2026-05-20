// jtUI Gallery —— 完整组件展示应用。
// 布局：顶栏（logo + 主题切换 + 语言切换） + 侧 Tab 区 + 主内容区。
// 每个 section 一个类，分别构造独立子树，通过 set_visible 切换。

#include <array>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "gallery_context.hpp"
#include "jtui/jtui.hpp"

#include "_shared/about_dialog.hpp"

#include "sections/audio.hpp"
#include "sections/button_styles.hpp"
#include "sections/buttons.hpp"
#include "sections/gauges.hpp"
#include "sections/icons.hpp"
#include "sections/inputs.hpp"
#include "sections/palette.hpp"
#include "sections/video.hpp"
#include "sections/vscode_palette.hpp"

namespace {

constexpr float kWindowWidth = 1440.0F;
constexpr float kWindowHeight = 900.0F;

// 按顺序探测媒体文件。Windows 下从 exe 所在目录查起（CMake POST_BUILD 会把
// 根目录的 test.mp4/mp3 拷贝到这里），再试 assets 子目录，最后退回当前工作
// 目录。Linux 保底查项目源码目录 —— 框架 Linux 下 Application::run 直接 return，
// 路径怎么填实际不影响运行。
#if !defined(_WIN32)
std::string probe_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return f.good() ? path : std::string{};
}
#endif

std::string find_media_file(const std::string& basename) {
#if defined(_WIN32)
    wchar_t exe_buf[MAX_PATH] = {0};
    if (GetModuleFileNameW(nullptr, exe_buf, MAX_PATH) > 0) {
        std::wstring exe_path{exe_buf};
        const auto slash = exe_path.find_last_of(L"/\\");
        if (slash != std::wstring::npos) {
            const std::wstring exe_dir = exe_path.substr(0, slash + 1);
            const std::wstring wbase{basename.begin(), basename.end()};
            const std::wstring candidates[] = {
                exe_dir + wbase,
                exe_dir + L"assets\\" + wbase,
                wbase,
            };
            for (const auto& wp : candidates) {
                const DWORD attr = GetFileAttributesW(wp.c_str());
                if (attr == INVALID_FILE_ATTRIBUTES ||
                    (attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                    continue;
                }
                const int len = WideCharToMultiByte(
                    CP_UTF8, 0, wp.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (len <= 1) continue;
                std::string utf8(static_cast<std::size_t>(len - 1), '\0');
                WideCharToMultiByte(CP_UTF8, 0, wp.c_str(), -1, utf8.data(), len,
                                    nullptr, nullptr);
                return utf8;
            }
        }
    }
    // 实在都找不到，返回 basename 让 decoder 按当前目录再试一次 —— 就算失败，
    // VideoPlayer/AudioPlayer 会显示 "No source"，不会崩。
    return basename;
#else
    const std::string candidates[] = {
        "./" + basename,
        "./assets/" + basename,
        "/home/jz/jtUI/" + basename,
    };
    for (const auto& p : candidates) {
        auto ok = probe_file(p);
        if (!ok.empty()) return ok;
    }
    return basename;
#endif
}

int run_gallery() {
    jtui::Application app;

    jtui::WindowOptions options{};
    options.title = "jtUI Gallery";
    options.frameless = true;
    options.size = {kWindowWidth, kWindowHeight};
    jtui::Window& window = app.create_window(options);

    jtui::theme::Theme::set(jtui::theme::ThemeMode::Dark);
    jtui_about::register_about_strings();

    auto root = std::make_unique<jtui::Panel>();
    auto* root_ptr = root.get();
    root->set_role(jtui::PanelRole::Base);
    root->set_frame(jtui::RectF{0, 0, kWindowWidth, kWindowHeight});

    auto window_frame = std::make_unique<jtui::WindowFrame>();
    window_frame->set_frameless(true);
    window_frame->set_frame(jtui::RectF{0, 0, kWindowWidth, kWindowHeight});
    root->append_child(std::move(window_frame));

    auto title_bar = std::make_unique<jtui::TitleBar>("jtUI Gallery");
    title_bar->set_frame(jtui::RectF{0, 0, kWindowWidth, 48.0F});
    root->append_child(std::move(title_bar));

    // Header：logo / 主标题 / 模式切换 / 语言切换
    auto brand = std::make_unique<jtui::Text>();
    brand->set_preset(jtui::TextStylePreset::Title);
    brand->set_content("jtUI");
    brand->set_frame(jtui::RectF{40.0F, 64.0F, 120.0F, 40.0F});
    auto* brand_ptr = brand.get();
    (void)brand_ptr;
    root->append_child(std::move(brand));

    auto subtitle = std::make_unique<jtui::Text>();
    subtitle->set_preset(jtui::TextStylePreset::Caption);
    subtitle->set_role(jtui::TextRole::Secondary);
    subtitle->set_frame(jtui::RectF{40.0F, 102.0F, 400.0F, 18.0F});
    auto* subtitle_ptr = subtitle.get();
    root->append_child(std::move(subtitle));

    auto theme_switch = std::make_unique<jtui::Switch>();
    theme_switch->set_checked(true);
    theme_switch->set_frame(jtui::RectF{kWindowWidth - 580.0F, 72.0F, 220.0F, 34.0F});
    auto* theme_switch_ptr = theme_switch.get();
    root->append_child(std::move(theme_switch));

    auto lang_switch = std::make_unique<jtui::Switch>();
    lang_switch->set_frame(jtui::RectF{kWindowWidth - 340.0F, 72.0F, 220.0F, 34.0F});
    auto* lang_switch_ptr = lang_switch.get();
    root->append_child(std::move(lang_switch));

    // About 圆按钮（info codicon）—— 把 modal container 推迟到所有 section append
    // 完后再 build，让 modal z-order 最上层。这里先 build about button，container
    // 指针通过 shared holder 延迟注入（click 时已被赋值）。
    auto about_container_holder = std::make_shared<jtui::Panel*>(nullptr);

    constexpr float kAboutBtnSz = 36.0F;
    auto about_btn = std::make_unique<jtui::Button>("");
    about_btn->set_shape(jtui::ButtonShape::Circle);
    about_btn->set_colors(
        jtui::Color::from_hex("#161616"),
        jtui::Color::from_hex("#202020"),
        jtui::Color::from_hex("#121212"),
        jtui::Color::from_hex("#FB923C"));
    about_btn->set_border(jtui::Color::from_hex("#2A2A2A"), 1.0F);
    about_btn->set_leading_codicon("info");
    about_btn->set_frame(jtui::RectF{
        kWindowWidth - 580.0F - 8.0F - kAboutBtnSz, 71.0F,
        kAboutBtnSz, kAboutBtnSz});
    about_btn->on_clicked().connect([about_container_holder]() {
        if (*about_container_holder != nullptr) {
            (*about_container_holder)->set_visible(true);
        }
    });
    root->append_child(std::move(about_btn));

    // 侧向 Tab 导航
    auto tabs = std::make_unique<jtui::Tabs>();
    tabs->set_frame(jtui::RectF{40.0F, 140.0F, kWindowWidth - 80.0F, 48.0F});
    auto* tabs_ptr = tabs.get();
    root->append_child(std::move(tabs));

    // 六个 section。每个 section 的 root 都挂 root 下，apply_layout 共用同一块
    // content area，set_visible 决定当前显示。
    const jtui::RectF content_frame{
        40.0F, 208.0F, kWindowWidth - 80.0F, kWindowHeight - 240.0F};
    const jtui::gallery::SectionLayout content_layout{
        content_frame.x + 24.0F,
        content_frame.y + 24.0F,
        content_frame.width - 48.0F,
        content_frame.height - 48.0F,
    };

    jtui::gallery::ButtonsSection buttons_section;
    jtui::gallery::ButtonStylesSection button_styles_section;
    jtui::gallery::InputsSection inputs_section;
    jtui::gallery::GaugesSection gauges_section;
    jtui::gallery::VideoSection video_section{find_media_file("test.mp4")};
    jtui::gallery::AudioSection audio_section{find_media_file("test.mp3")};
    jtui::gallery::PaletteSection palette_section;
    jtui::gallery::VsCodePaletteSection vscode_palette_section;
    jtui::gallery::IconsSection icons_section;

    std::vector<jtui::Panel*> section_roots;

    auto install_section = [&](std::unique_ptr<jtui::Panel> sec) {
        sec->set_frame(content_frame);
        jtui::Panel* ptr = sec.get();
        section_roots.push_back(ptr);
        root->append_child(std::move(sec));
        return ptr;
    };

    jtui::Panel* buttons_root = install_section(buttons_section.take_root());
    jtui::Panel* button_styles_root = install_section(button_styles_section.take_root());
    jtui::Panel* inputs_root = install_section(inputs_section.take_root());
    jtui::Panel* gauges_root = install_section(gauges_section.take_root());
    jtui::Panel* video_root = install_section(video_section.take_root());
    jtui::Panel* audio_root = install_section(audio_section.take_root());
    jtui::Panel* palette_root = install_section(palette_section.take_root());
    jtui::Panel* vscode_palette_root = install_section(vscode_palette_section.take_root());
    jtui::Panel* icons_root = install_section(icons_section.take_root());
    (void)buttons_root;
    (void)button_styles_root;
    (void)inputs_root;
    (void)gauges_root;
    (void)video_root;
    (void)audio_root;
    (void)palette_root;
    (void)vscode_palette_root;
    (void)icons_root;

    buttons_section.apply_layout(content_layout);
    button_styles_section.apply_layout(content_layout);
    inputs_section.apply_layout(content_layout);
    gauges_section.apply_layout(content_layout);
    video_section.apply_layout(content_layout);
    audio_section.apply_layout(content_layout);
    palette_section.apply_layout(content_layout);
    vscode_palette_section.apply_layout(content_layout);
    icons_section.apply_layout(content_layout);

    auto state_locale = std::make_shared<jtui::gallery::Locale>(jtui::gallery::Locale::English);

    auto show_only = [&section_roots](std::size_t idx) {
        for (std::size_t i = 0; i < section_roots.size(); ++i) {
            section_roots[i]->set_visible(i == idx);
        }
    };
    show_only(0);
    tabs_ptr->set_selected_index(0);

    auto apply_labels = [&, subtitle_ptr, theme_switch_ptr, lang_switch_ptr, tabs_ptr, state_locale]() {
        const bool zh = *state_locale == jtui::gallery::Locale::Chinese;
        subtitle_ptr->set_content(zh
            ? "组件库 / 主题 / 媒体播放 / 国际化"
            : "Component library / Theme / Media / i18n");
        theme_switch_ptr->set_label(zh ? "深色模式" : "Dark mode");
        lang_switch_ptr->set_label(zh ? "中文界面" : "Chinese UI");

        const std::vector<std::string> tab_items = zh
            ? std::vector<std::string>{"按钮", "按钮风格", "文本框", "仪表盘", "视频", "音频", "调色板", "VS Code 色卡", "图标"}
            : std::vector<std::string>{"Buttons", "Button Styles", "Inputs", "Gauges", "Video", "Audio", "Palette", "VSCode Palette", "Codicons"};
        tabs_ptr->set_items(tab_items);

        buttons_section.apply_labels(*state_locale);
        button_styles_section.apply_labels(*state_locale);
        inputs_section.apply_labels(*state_locale);
        gauges_section.apply_labels(*state_locale);
        video_section.apply_labels(*state_locale);
        audio_section.apply_labels(*state_locale);
        palette_section.apply_labels(*state_locale);
        vscode_palette_section.apply_labels(*state_locale);
        icons_section.apply_labels(*state_locale);
    };

    apply_labels();

    // Tab 选中切换：直接挂 on_changed 信号，section 切换即时响应。
    // 关键修复：之前用每帧 return true 的 TabWatcher 轮询，让 animation timer
    // 永不停，整个 app 每 16ms 强制全树 tick + 可能的全窗 repaint —— 这是
    // "UI 卡"的大根源之一。用 Signal 之后 timer 在无动画时自然关。
    tabs_ptr->on_changed().connect([show_only, root_ptr](std::size_t idx) {
        show_only(idx);
        root_ptr->invalidate(jtui::DirtyFlags::Paint);
    });

    // 主题切换
    theme_switch_ptr->on_changed().connect([](bool checked) {
        jtui::theme::Theme::set(checked ? jtui::theme::ThemeMode::Dark
                                       : jtui::theme::ThemeMode::Light);
    });
    jtui::theme::Theme::on_changed().connect([root_ptr](jtui::theme::ThemeMode) {
        root_ptr->invalidate(jtui::DirtyFlags::Paint);
    });

    // 语言切换
    lang_switch_ptr->on_changed().connect([state_locale, apply_labels, root_ptr](bool checked) {
        *state_locale = checked ? jtui::gallery::Locale::Chinese
                                  : jtui::gallery::Locale::English;
        apply_labels();
        root_ptr->invalidate(jtui::DirtyFlags::Paint);
    });

    // About modal —— 推迟到所有 widget append 完之后，让 modal z-order 最上层。
    // 用暖橘 accent + 黑底配色（gallery 一次性 build，切主题不会动态更新 modal 色）。
    jtui_about::AboutColors about_c{};
    about_c.scrim          = jtui::Color{0.0F, 0.0F, 0.0F, 0.62F};
    about_c.bg_panel       = jtui::Color::from_hex("#0F0F0F");
    about_c.border         = jtui::Color::from_hex("#1F1F1F");
    about_c.border_strong  = jtui::Color::from_hex("#2A2A2A");
    about_c.text_strong    = jtui::Color::from_hex("#FFFFFF");
    about_c.text_primary   = jtui::Color::from_hex("#F5F5F5");
    about_c.text_secondary = jtui::Color::from_hex("#A8A8A8");
    about_c.text_muted     = jtui::Color::from_hex("#6B6B6B");
    about_c.accent         = jtui::Color::from_hex("#FB923C");
    about_c.accent_hover   = jtui::Color::from_hex("#FDA45F");
    about_c.accent_pressed = jtui::Color::from_hex("#EA8224");
    about_c.accent_soft    = jtui::Color::from_hex("#3D1F0A");
    about_c.accent_fg      = jtui::Color::from_hex("#1A0F05");

    *about_container_holder = jtui_about::build_about_modal(
        *root, /*open=*/false, about_c,
        /*on_close=*/[]() {},
        kWindowWidth, kWindowHeight);

    window.set_content(std::move(root));
    return app.run();
}

}  // namespace

#if defined(_WIN32)
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    return run_gallery();
}
#else
int main() {
    return run_gallery();
}
#endif

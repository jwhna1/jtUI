#pragma once

// jtUI 公共 API facade
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）
//
// 对外名字 jtUI；源码内部命名空间 hui::（历史代号 hybrid-ui）。Host 应用 / examples
// / 未来 binding 层应该只 #include "jtui/jtui.hpp" 并使用 jtui:: 命名空间，不要
// 摸 hui:: 内部类型 —— 这条边界让我们能在不破坏外部 API 的前提下重构 hui:: 内部。
//
// 本文件做的事：
//   1) using-declaration 把 hui:: 常用类型暴露在 jtui:: 下
//   2) namespace alias `jtui::theme = hui::theme`
//   3) #include 集中所有 widget / runtime 公共头，host 一个 include 拿全
//
// 没暴露在 jtui:: 的（典型例子：DirectTextSession / WindowRuntimeAccess /
// EventDispatcher 内部辅助函数 / FFI struct）属于"内部实现细节"，host 不该用。
// 出现 host 真需要某符号但 jtui:: 没暴露时，先评估是否值得加入公共边界，再加。

#include "hui/app/application.hpp"
#include "hui/app/component_view.hpp"
#include "hui/app/window.hpp"
#include "hui/event/dispatcher.hpp"
#include "hui/event/events.hpp"
#include "hui/foundation/color.hpp"
#include "hui/foundation/geometry.hpp"
#include "hui/foundation/i18n.hpp"
#include "hui/foundation/image_loader.hpp"
#include "hui/foundation/pixel_buffer.hpp"
#include "hui/foundation/result.hpp"
#include "hui/object/dirty_flags.hpp"
#include "hui/object/element.hpp"
#include "hui/object/event.hpp"
#include "hui/object/node.hpp"
#include "hui/object/widget.hpp"
#include "hui/property/command.hpp"
#include "hui/property/property.hpp"
#include "hui/property/realtime_source.hpp"
#include "hui/property/signal.hpp"
#include "hui/render/paint_context.hpp"
#include "hui/theme/theme.hpp"
#include "hui/theme/token_override.hpp"
#include "hui/widgets/basic/flex_box.hpp"
#include "hui/widgets/basic/icon.hpp"
#include "hui/widgets/basic/list_view.hpp"
#include "hui/widgets/basic/panel.hpp"
#include "hui/widgets/basic/scroll_view.hpp"
#include "hui/widgets/basic/text.hpp"
#include "hui/widgets/basic/title_bar.hpp"
#include "hui/widgets/basic/window_frame.hpp"
#include "hui/widgets/common/avatar.hpp"
#include "hui/widgets/common/badge.hpp"
#include "hui/widgets/common/button.hpp"
#include "hui/widgets/common/card.hpp"
#include "hui/widgets/common/checkbox.hpp"
#include "hui/widgets/common/chip.hpp"
#include "hui/widgets/common/dialog.hpp"
#include "hui/widgets/common/dropdown.hpp"
#include "hui/widgets/common/folder_card.hpp"
#include "hui/widgets/common/gauge.hpp"
#include "hui/widgets/common/metric_card.hpp"
#include "hui/widgets/common/popover.hpp"
#include "hui/widgets/common/progress_bar.hpp"
#include "hui/widgets/common/search_input.hpp"
#include "hui/widgets/common/sidebar_nav.hpp"
#include "hui/widgets/common/radial_gauge.hpp"
#include "hui/widgets/common/semi_gauge.hpp"
#include "hui/widgets/common/slider.hpp"
#include "hui/widgets/common/switch.hpp"
#include "hui/widgets/common/tabs.hpp"
#include "hui/widgets/common/text_input.hpp"
#include "hui/widgets/common/toolbar.hpp"
#include "hui/widgets/common/tooltip.hpp"
#include "hui/widgets/media/audio_player.hpp"
#include "hui/widgets/media/level_meter.hpp"
#include "hui/widgets/media/realtime_canvas.hpp"
#include "hui/widgets/media/timeline.hpp"
#include "hui/widgets/media/video_player.hpp"
#include "hui/widgets/media/video_view.hpp"
#include "hui/widgets/media/waveform_view.hpp"

namespace jtui {

// theme 子命名空间整体复用 hui::theme（含 colors() / radius() / spacing() /
// Theme::set / Tone / TokenOverride 等）。host 用 jtui::theme::colors() 等。
namespace theme = hui::theme;

// i18n 子命名空间 —— jtui::i18n::tr() / set_locale() / register_entries() / Locale。
namespace i18n  = hui::i18n;

// image 子命名空间 —— jtui::image::load(path) / load_from_memory() 把 PNG/JPG/...
// 解码成与 PaintContext::draw_texture 兼容的 BGRA8 PixelBuffer。
namespace image = hui::image;

// foundation
using hui::Color;
using hui::PointF;
using hui::RectF;
using hui::SizeF;
using hui::AlphaMode;
using hui::PixelBuffer;
using hui::PixelFormat;

// 事件 / dirty
using hui::DirtyFlags;
using hui::Event;
using hui::EventBase;
using hui::EventKind;
using hui::EventPhase;
using hui::EventState;
using hui::EventDispatcher;
using hui::FocusEvent;
using hui::KeyEvent;
using hui::MouseButton;
using hui::MouseEvent;
using hui::TextInputEvent;
using hui::event_base;

// 对象模型
using hui::Element;
using hui::Node;
using hui::Widget;
using hui::WindowAction;

// 属性
using hui::Command;
using hui::LatestValue;
using hui::Property;
using hui::RealtimeRingBuffer;
using hui::RealtimeSource;
using hui::Signal;

// 渲染
using hui::DrawCommand;
using hui::DrawCommandKind;
using hui::PaintContext;
using hui::TextAlignment;

// app
using hui::Application;
using hui::ComponentView;
using hui::Shortcut;
using hui::ShortcutId;
using hui::Window;
using hui::WindowOptions;

// basic widgets
using hui::CodiconIcon;
using hui::Edges;
using hui::FlexAlign;
using hui::FlexBox;
using hui::FlexItem;
using hui::FlexJustify;
using hui::IListModel;
using hui::LayoutDirection;
using hui::Length;
using hui::ListView;
template <typename T>
using VectorListModel = hui::VectorListModel<T>;
using hui::Panel;
using hui::PanelRole;
using hui::ScrollView;
using hui::Text;
using hui::TextRole;
using hui::TextRun;
using hui::TextStylePreset;
using hui::TitleBar;
using hui::WindowFrame;

// common widgets
using hui::Avatar;
using hui::AvatarGroup;
using hui::AvatarShape;
using hui::AvatarStatus;
using hui::Badge;
using hui::BadgeTone;
using hui::Button;
using hui::ButtonShape;
using hui::ButtonSize;
using hui::ButtonVariant;
using hui::Card;
using hui::CardPadding;
using hui::Checkbox;
using hui::CheckState;
using hui::Chip;
using hui::Dialog;
using hui::DialogHitTarget;
using hui::Dropdown;
using hui::FolderCard;
using hui::Gauge;
using hui::MetricCard;
using hui::Popover;
using hui::ProgressBar;
using hui::ProgressBarMode;
using hui::SearchInput;
using hui::SidebarNav;
using hui::RadialGauge;
using hui::SemiGauge;
using hui::Slider;
using hui::Switch;
using hui::Tabs;
using hui::TextInput;
using hui::TextInputState;
using hui::Toolbar;
using hui::ToolbarOrientation;
using hui::Tooltip;

// media widgets
using hui::AudioPlayer;
using hui::LevelMeter;
using hui::RealtimeCanvas;
using hui::Timeline;
using hui::VideoPlayer;
using hui::VideoView;
using hui::WaveformView;

}  // namespace jtui

#include "hui/c_api/hui.h"

#include <new>
#include <string>
#include <utility>

#include "hui/app/application.hpp"

struct hui_application {
    hui::Application impl;
};

struct hui_window {
    hui::Window* impl;
};

hui_application* hui_application_create(void) {
    return new (std::nothrow) hui_application {};
}

void hui_application_destroy(hui_application* application) {
    delete application;
}

hui_window* hui_application_create_window(
    hui_application* application,
    const hui_window_desc* desc) {
    if (application == nullptr || desc == nullptr) {
        return nullptr;
    }

    hui::WindowOptions options {};
    if (desc->title != nullptr) {
        options.title = desc->title;
    }
    options.size.width = static_cast<float>(desc->width);
    options.size.height = static_cast<float>(desc->height);
    options.resizable = (desc->flags & HUI_WINDOW_FLAG_RESIZABLE) != 0;
    options.frameless = (desc->flags & HUI_WINDOW_FLAG_FRAMELESS) != 0;

    hui::Window& window = application->impl.create_window(std::move(options));
    hui_window* handle = new (std::nothrow) hui_window {};
    if (handle == nullptr) {
        return nullptr;
    }

    handle->impl = &window;
    return handle;
}

void hui_window_destroy(hui_window* window) {
    delete window;
}

hui_result_code hui_window_set_title(hui_window* window, const char* title) {
    if (window == nullptr || window->impl == nullptr || title == nullptr) {
        return HUI_RESULT_INVALID_ARGUMENT;
    }

    window->impl->set_title(std::string {title});
    return HUI_RESULT_OK;
}

hui_result_code hui_window_set_visible(hui_window* window, int visible) {
    if (window == nullptr || window->impl == nullptr) {
        return HUI_RESULT_INVALID_ARGUMENT;
    }
    window->impl->set_visible(visible != 0);
    return HUI_RESULT_OK;
}

hui_result_code hui_window_invalidate(hui_window* window) {
    if (window == nullptr || window->impl == nullptr) {
        return HUI_RESULT_INVALID_ARGUMENT;
    }
    // window 没有"整窗失效"接口；通过 content widget 的 invalidate(Paint) 让脏区
    // 系统下一帧重画。content 未挂时 silently no-op（host 还没 attach 内容）。
    if (auto* content = window->impl->content()) {
        content->invalidate(hui::DirtyFlags::Paint);
    }
    return HUI_RESULT_OK;
}

int hui_application_window_count(const hui_application* application) {
    if (application == nullptr) {
        return 0;
    }
    return static_cast<int>(application->impl.windows().size());
}

int hui_application_run(hui_application* application) {
    if (application == nullptr) {
        return -1;
    }

    return application->impl.run();
}

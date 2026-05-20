#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum hui_result_code {
    HUI_RESULT_OK = 0,
    HUI_RESULT_INVALID_ARGUMENT = 1,
    HUI_RESULT_NOT_IMPLEMENTED = 2,
    HUI_RESULT_PLATFORM_ERROR = 3
} hui_result_code;

typedef enum hui_window_flags {
    HUI_WINDOW_FLAG_NONE = 0,
    HUI_WINDOW_FLAG_RESIZABLE = 1 << 0,
    HUI_WINDOW_FLAG_FRAMELESS = 1 << 1
} hui_window_flags;

typedef struct hui_application hui_application;
typedef struct hui_window hui_window;

typedef struct hui_window_desc {
    const char* title;
    int width;
    int height;
    int flags;
} hui_window_desc;

hui_application* hui_application_create(void);
void hui_application_destroy(hui_application* application);

hui_window* hui_application_create_window(
    hui_application* application,
    const hui_window_desc* desc);

void hui_window_destroy(hui_window* window);
hui_result_code hui_window_set_title(hui_window* window, const char* title);
hui_result_code hui_window_set_visible(hui_window* window, int visible);
hui_result_code hui_window_invalidate(hui_window* window);

/** 当前 application 已创建的 window 数量。 */
int hui_application_window_count(const hui_application* application);

int hui_application_run(hui_application* application);

#ifdef __cplusplus
}
#endif

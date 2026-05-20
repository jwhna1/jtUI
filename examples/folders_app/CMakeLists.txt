# folders_app —— jtUI Folders 子品牌（桌面文件管理风格演示）
# 维护：曾能混 <jwhna1@gmail.com>

set(FOLDERS_APP_SOURCES
    main.cpp
)

if(WIN32)
    add_executable(folders_app WIN32 ${FOLDERS_APP_SOURCES})
    if(MINGW)
        target_compile_options(folders_app PRIVATE -municode)
        target_link_options(folders_app PRIVATE -municode)
    endif()
else()
    add_executable(folders_app ${FOLDERS_APP_SOURCES})
endif()

target_include_directories(folders_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(
    folders_app
    PRIVATE
        jtui::api
)

hui_configure_target(folders_app)

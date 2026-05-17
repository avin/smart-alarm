cmake_minimum_required(VERSION 3.24)

project(SmartAlarm VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.8.3 REQUIRED COMPONENTS Core Gui Widgets Multimedia Test)

if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

set(SMART_ALARM_CORE_SOURCES
    src/core/notification.cpp
    src/core/schedule_evaluator.cpp
    src/core/schedule_formatter.cpp
    src/core/time_input_normalizer.cpp
    src/core/notification_validator.cpp
    src/storage/config_store.cpp
    src/audio/sound_pattern_parser.cpp
    src/audio/tone_generator.cpp
    src/audio/audio_player.cpp
    src/audio/audio_queue.cpp
    src/audio/sound_preset_registry.cpp
    src/app/minute_scheduler.cpp
    src/app/app_controller.cpp
    src/ui/time_edit.cpp
    src/ui/date_edit.cpp
    src/ui/day_of_week_selector.cpp
    src/ui/color_palette_widget.cpp
    src/ui/position_picker_widget.cpp
    src/ui/slide_to_dismiss.cpp
    src/ui/notification_popup.cpp
    src/ui/popup_manager.cpp
    src/ui/notification_table_model.cpp
    src/ui/notification_actions_delegate.cpp
    src/ui/notification_editor_dialog.cpp
    src/ui/global_settings_dialog.cpp
    src/ui/main_window.cpp
    src/ui/tray_controller.cpp
)

add_library(smart_alarm_lib STATIC ${SMART_ALARM_CORE_SOURCES})
target_include_directories(smart_alarm_lib PUBLIC src)
target_link_libraries(smart_alarm_lib PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Multimedia)

add_executable(SmartAlarm src/main.cpp)
target_link_libraries(SmartAlarm PRIVATE smart_alarm_lib)

enable_testing()
add_subdirectory(tests)

cmake_minimum_required(VERSION 3.24)

project(SmartAlarm VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.8.3 REQUIRED COMPONENTS Core Gui Widgets Multimedia Network Test)
include(GNUInstallDirs)

if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

set(SMART_ALARM_WINDOWS_RESOURCES)
if(WIN32)
    enable_language(RC)
    set(SMART_ALARM_WINDOWS_RESOURCES src/assets/windows/app.rc)
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
    src/app/command_client.cpp
    src/app/command_server.cpp
    src/ui/time_edit.cpp
    src/ui/date_edit.cpp
    src/ui/day_of_week_selector.cpp
    src/ui/color_palette_widget.cpp
    src/ui/position_picker_widget.cpp
    src/ui/slide_to_dismiss.cpp
    src/ui/notification_popup.cpp
    src/ui/popup_manager.cpp
    src/ui/notification_table_model.cpp
    src/ui/notification_state_delegate.cpp
    src/ui/notification_actions_delegate.cpp
    src/ui/notification_editor_dialog.cpp
    src/ui/global_settings_dialog.cpp
    src/ui/lucide_icons.cpp
    src/ui/app_icon.cpp
    src/ui/main_window.cpp
    src/ui/tray_controller.cpp
)

add_library(smart_alarm_lib STATIC ${SMART_ALARM_CORE_SOURCES})
target_include_directories(smart_alarm_lib PUBLIC src)
target_link_libraries(smart_alarm_lib PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Multimedia Qt6::Network)

add_executable(SmartAlarm src/main.cpp src/resources.qrc ${SMART_ALARM_WINDOWS_RESOURCES})
target_link_libraries(SmartAlarm PRIVATE smart_alarm_lib)
set_target_properties(SmartAlarm PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Smart Alarm"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.smartalarm.SmartAlarm"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
)

install(TARGETS SmartAlarm
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES LICENSE CLI.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY agents/skills/smart-alarm-cli
    DESTINATION ${CMAKE_INSTALL_DATADIR}/SmartAlarm/agents/skills
)
install(FILES installer/scripts/update_user_path.ps1
    DESTINATION ${CMAKE_INSTALL_DATADIR}/SmartAlarm/installer
)

qt_generate_deploy_app_script(
    TARGET SmartAlarm
    OUTPUT_SCRIPT SMART_ALARM_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${SMART_ALARM_DEPLOY_SCRIPT})

enable_testing()
add_subdirectory(tests)

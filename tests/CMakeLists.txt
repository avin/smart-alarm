function(add_smart_alarm_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE smart_alarm_lib Qt6::Test)
    add_test(NAME ${target} COMMAND ${target})
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    get_filename_component(CXX_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_target_property(QT_CORE_LOCATION Qt6::Core IMPORTED_LOCATION_RELEASE)
    if(NOT QT_CORE_LOCATION)
        get_target_property(QT_CORE_LOCATION Qt6::Core IMPORTED_LOCATION)
    endif()
    get_filename_component(QT_LIB_DIR "${QT_CORE_LOCATION}" DIRECTORY)
    set_tests_properties(${target} PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${QT_BIN_DIR};PATH=path_list_prepend:${CXX_COMPILER_DIR};LD_LIBRARY_PATH=path_list_prepend:${QT_LIB_DIR};DYLD_LIBRARY_PATH=path_list_prepend:${QT_LIB_DIR}"
    )
endfunction()

add_smart_alarm_test(test_time_input_normalizer core/test_time_input_normalizer.cpp)
add_smart_alarm_test(test_schedule_evaluator core/test_schedule_evaluator.cpp)
add_smart_alarm_test(test_sound_pattern_parser audio/test_sound_pattern_parser.cpp)
add_smart_alarm_test(test_tone_generator audio/test_tone_generator.cpp)
add_smart_alarm_test(test_config_store storage/test_config_store.cpp)

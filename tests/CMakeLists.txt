set(
    TEST_FLAGS
    $<$<CXX_COMPILER_ID:Clang,GNU>:
        $<BUILD_INTERFACE:-fsanitize=address,undefined>>)
find_package(Qt5 COMPONENTS Core Test Widgets REQUIRED)
add_library(test_flags INTERFACE)
target_compile_options(test_flags INTERFACE ${TEST_FLAGS})
target_include_directories(
    test_flags INTERFACE
    "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/src")
target_link_options(test_flags INTERFACE ${TEST_FLAGS})
target_link_libraries(test_flags INTERFACE warning_flags Qt5::Test)

add_executable(tests_cmd_command cmd/command.cpp)
set_property(TARGET tests_cmd_command PROPERTY OUTPUT_NAME command)
set_property(
    TARGET tests_cmd_command
    PROPERTY RUNTIME_OUTPUT_DIRECTORY cmd)
target_link_libraries(tests_cmd_command PRIVATE test_flags)
add_test(NAME tests_cmd_command COMMAND cmd/command)

add_executable(
    tests_conf_conf
    "${PROJECT_SOURCE_DIR}/src/conf/conf.cpp" conf/conf.cpp)
set_property(TARGET tests_conf_conf PROPERTY OUTPUT_NAME conf)
set_property(
    TARGET tests_conf_conf
    PROPERTY RUNTIME_OUTPUT_DIRECTORY conf)
target_link_libraries(tests_conf_conf PRIVATE test_flags)
add_test(NAME tests_conf_conf COMMAND conf/conf)

add_executable(
    tests_ui_panel
    "${PROJECT_SOURCE_DIR}/src/ui/panel.cpp" ui/panel.cpp)
set_property(TARGET tests_ui_panel PROPERTY OUTPUT_NAME panel)
set_property(
    TARGET tests_ui_panel
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ui)
target_link_libraries(tests_ui_panel PRIVATE test_flags Qt5::Widgets)
add_test(NAME tests_ui_panel COMMAND ui/panel)

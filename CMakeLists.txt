cmake_minimum_required(VERSION 3.10)
project(CoopControllerTests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Find GoogleTest
find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    pkg_check_modules(GTest googletest)
endif()

# Compiler flags for testing
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-missing-field-initializers -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Optional coverage instrumentation
option(COOP_ENABLE_COVERAGE "Enable coverage instrumentation flags" OFF)
if(COOP_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -O0 -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Source files
set(LIB_SOURCES
    lib/MockSensorManager.cpp
    lib/MockPumpController.cpp
    lib/MockLightController.cpp
    lib/MockSettingsManager.cpp
    lib/MockWebServer.cpp
    lib/MockWiFi.cpp
    lib/Logger.cpp
    lib/WifiController.cpp
    lib/SunriseSunset.cpp
    lib/MockEmailManager.cpp
    lib/MockTelegramManager.cpp
    lib/MockSystemMetrics.cpp
    lib/MockPushbuttonController.cpp
    lib/MockAPIRequestQueue.cpp
)

set(TEST_UTILS_SOURCES
    test_utils/TestUtils.cpp
)

set(TEST_COMMON_SOURCES
    test/test_common/CommonTestFixture.cpp
)

function(add_coop_test target sourceFile)
    add_executable(${target}
        ${TEST_COMMON_SOURCES}
        ${TEST_UTILS_SOURCES}
        ${LIB_SOURCES}
        ${sourceFile}
    )

    target_include_directories(${target} PRIVATE lib test_utils test/test_common)

    if(GTest_FOUND)
        target_link_libraries(${target} PRIVATE GTest::gtest_main Threads::Threads)
    elseif(GTest_LIBRARIES)
        target_link_libraries(${target} PRIVATE ${GTest_LIBRARIES} Threads::Threads)
    endif()
endfunction()

# Create test executables
add_coop_test(sensor_manager_test test/test_desktop/test_sensor_manager.cpp)
add_coop_test(pump_controller_test test/test_desktop/test_pump_controller.cpp)
add_coop_test(light_controller_test test/test_desktop/test_light_controller.cpp)
add_coop_test(settings_manager_test test/test_desktop/test_settings_manager.cpp)
add_coop_test(logger_test test/test_desktop/test_logger.cpp)
add_coop_test(wifi_controller_test test/test_desktop/test_wifi_controller.cpp)
add_coop_test(sunrise_sunset_test test/test_desktop/test_sunrise_sunset.cpp)
add_coop_test(email_manager_test test/test_desktop/test_email_manager.cpp)
add_coop_test(telegram_manager_test test/test_desktop/test_telegram_manager.cpp)
add_coop_test(system_metrics_test test/test_desktop/test_system_metrics.cpp)
add_coop_test(pushbutton_controller_test test/test_desktop/test_pushbutton_controller.cpp)
add_coop_test(api_request_queue_test test/test_desktop/test_api_request_queue.cpp)
add_coop_test(monitoring_integration_test test/test_desktop/test_monitoring_integration.cpp)

# Add test targets
add_test(NAME SensorManagerTest COMMAND sensor_manager_test)
add_test(NAME PumpControllerTest COMMAND pump_controller_test)
add_test(NAME LightControllerTest COMMAND light_controller_test)
add_test(NAME SettingsManagerTest COMMAND settings_manager_test)
add_test(NAME LoggerTest COMMAND logger_test)
add_test(NAME WifiControllerTest COMMAND wifi_controller_test)
add_test(NAME SunriseSunsetTest COMMAND sunrise_sunset_test)
add_test(NAME EmailManagerTest COMMAND email_manager_test)
add_test(NAME TelegramManagerTest COMMAND telegram_manager_test)
add_test(NAME SystemMetricsTest COMMAND system_metrics_test)
add_test(NAME PushbuttonControllerTest COMMAND pushbutton_controller_test)
add_test(NAME APIRequestQueueTest COMMAND api_request_queue_test)
add_test(NAME MonitoringIntegrationTest COMMAND monitoring_integration_test)

# Custom test target
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS
        sensor_manager_test
        pump_controller_test
        light_controller_test
        settings_manager_test
        logger_test
        wifi_controller_test
        sunrise_sunset_test
        email_manager_test
        telegram_manager_test
        system_metrics_test
        pushbutton_controller_test
        api_request_queue_test
        monitoring_integration_test
)

# Coverage target
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    find_program(GCOVR_EXECUTABLE gcovr)

    if(GCOVR_EXECUTABLE)
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${GCOVR_EXECUTABLE} -r ${CMAKE_CURRENT_SOURCE_DIR} --exclude-directories ".*/_deps" --html --html-details -o coverage/index.html
            COMMAND ${GCOVR_EXECUTABLE} -r ${CMAKE_CURRENT_SOURCE_DIR} --exclude-directories ".*/_deps" --print-summary
            DEPENDS
                sensor_manager_test
                pump_controller_test
                light_controller_test
                settings_manager_test
                logger_test
                wifi_controller_test
                sunrise_sunset_test
                email_manager_test
                telegram_manager_test
                system_metrics_test
                pushbutton_controller_test
                api_request_queue_test
                monitoring_integration_test
            COMMENT "Generating code coverage report (coverage/index.html)"
        )
    else()
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND gcov ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp
            COMMAND gcov ${CMAKE_CURRENT_SOURCE_DIR}/test_utils/*.cpp
            COMMAND gcov ${CMAKE_CURRENT_SOURCE_DIR}/test/test_common/*.cpp
            COMMAND gcov ${CMAKE_CURRENT_SOURCE_DIR}/test/test_desktop/*.cpp
            DEPENDS
                sensor_manager_test
                pump_controller_test
                light_controller_test
                settings_manager_test
                logger_test
                wifi_controller_test
                sunrise_sunset_test
                email_manager_test
                telegram_manager_test
                system_metrics_test
                pushbutton_controller_test
                api_request_queue_test
                monitoring_integration_test
            COMMENT "Generating code coverage report"
        )
    endif()
endif()

# Install target
install(TARGETS
    sensor_manager_test
    pump_controller_test
    light_controller_test
    settings_manager_test
    logger_test
    wifi_controller_test
    sunrise_sunset_test
    email_manager_test
    telegram_manager_test
    system_metrics_test
    pushbutton_controller_test
    api_request_queue_test
    monitoring_integration_test
    RUNTIME DESTINATION bin
)

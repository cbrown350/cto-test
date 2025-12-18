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
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTest googletest)
endif()

# Compiler flags for testing
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-missing-field-initializers -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Source files
set(LIB_SOURCES
    lib/MockSensorManager.cpp
    lib/MockPumpController.cpp
    lib/MockLightController.cpp
    lib/MockSettingsManager.cpp
    lib/MockWebServer.cpp
    lib/MockWiFi.cpp
)

set(TEST_UTILS_SOURCES
    test_utils/TestUtils.cpp
)

set(TEST_COMMON_SOURCES
    test/test_common/CommonTestFixture.cpp
)

set(TEST_DESKTOP_SOURCES
    test/test_desktop/SensorManagerTest.cpp
    test/test_desktop/PumpControllerTest.cpp
    test/test_desktop/LightControllerTest.cpp
)

# Create test executables
add_executable(sensor_manager_test ${TEST_COMMON_SOURCES} ${TEST_UTILS_SOURCES} ${LIB_SOURCES} test/test_desktop/SensorManagerTest.cpp)
add_executable(pump_controller_test ${TEST_COMMON_SOURCES} ${TEST_UTILS_SOURCES} ${LIB_SOURCES} test/test_desktop/PumpControllerTest.cpp)
add_executable(light_controller_test ${TEST_COMMON_SOURCES} ${TEST_UTILS_SOURCES} ${LIB_SOURCES} test/test_desktop/LightControllerTest.cpp)

# Link libraries
if(GTest_FOUND)
    target_link_libraries(sensor_manager_test PRIVATE GTest::gtest_main GTest::gmock_main)
    target_link_libraries(pump_controller_test PRIVATE GTest::gtest_main GTest::gmock_main)
    target_link_libraries(light_controller_test PRIVATE GTest::gtest_main GTest::gmock_main)
elseif(GTest_LIBRARIES)
    target_link_libraries(sensor_manager_test PRIVATE ${GTest_LIBRARIES} Threads::Threads)
    target_link_libraries(pump_controller_test PRIVATE ${GTest_LIBRARIES} Threads::Threads)
    target_link_libraries(light_controller_test PRIVATE ${GTest_LIBRARIES} Threads::Threads)
endif()

# Include directories
target_include_directories(sensor_manager_test PRIVATE lib test_utils test/test_common)
target_include_directories(pump_controller_test PRIVATE lib test_utils test/test_common)
target_include_directories(light_controller_test PRIVATE lib test_utils test/test_common)

# Add test targets
add_test(NAME SensorManagerTest COMMAND sensor_manager_test)
add_test(NAME PumpControllerTest COMMAND pump_controller_test)
add_test(NAME LightControllerTest COMMAND light_controller_test)

# Custom test target
add_custom_target(test
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS sensor_manager_test pump_controller_test light_controller_test
)

# Coverage target
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_custom_target(coverage
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND gcov ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp
        COMMAND gcov ${CMAKE_CURRENT_SOURCE_DIR}/test_utils/*.cpp
        COMMAND gcov ${CMAKE_CURRENT_SOURCE_DIR}/test/test_common/*.cpp
        COMMAND gcov ${CMAKE_CURRENT_SOURCE_DIR}/test/test_desktop/*.cpp
        DEPENDS sensor_manager_test pump_controller_test light_controller_test
        COMMENT "Generating code coverage report"
    )
endif()

# Install target
install(TARGETS sensor_manager_test pump_controller_test light_controller_test
    RUNTIME DESTINATION bin
)
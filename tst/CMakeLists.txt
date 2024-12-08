cmake_minimum_required(VERSION 3.24)
project(search_engine_tests)

set(CMAKE_CXX_STANDARD 17)

# Подключаем GoogleTest
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)

set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Создаём исполняемый файл для тестов
add_executable(
        hello_test
        hello_test.cc
        ${CMAKE_SOURCE_DIR}/src/InvertedIndex.cpp
        ${CMAKE_SOURCE_DIR}/src/SearchServer.cpp
)

# Подключаем директории с заголовками
target_include_directories(hello_test PRIVATE
        ${MY_INCLUDE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
        )

# Подключаем библиотеки
target_link_libraries(hello_test PRIVATE
        nlohmann_json::nlohmann_json
        GTest::gtest_main
        )

# Автоматическое обнаружение тестов
include(GoogleTest)
gtest_discover_tests(hello_test)

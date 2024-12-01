cmake_minimum_required(VERSION 3.24)
project(search_engine VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(VERSION_APP ${search_engine_VERSION_MAJOR}.${search_engine_VERSION_MINOR})
set(MY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# Подключаем nlohmann/json через FetchContent
include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/archive/refs/heads/develop.zip
)
FetchContent_MakeAvailable(nlohmann_json)

# Переменная для директории с заголовками nlohmann/json
set(NLOHMANN_JSON_INCLUDE_DIR ${nlohmann_json_SOURCE_DIR})

# Подключаем исходники и тесты
add_subdirectory(src)
add_subdirectory(tst)

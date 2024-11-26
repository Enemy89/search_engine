cmake_minimum_required(VERSION 3.24)
project(search_engine VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(VERSION_APP ${search_engine_VERSION_MAJOR}.${search_engine_VERSION_MINOR})
set(MY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# Указываем путь к библиотеке nlohmann_json
set(NLOHMANN_JSON_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/nlohmann_json/include)

# Подключаем исходники и тесты
add_subdirectory(src)
add_subdirectory(tst)

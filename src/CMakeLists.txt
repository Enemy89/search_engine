cmake_minimum_required(VERSION 3.24)
project(search_engine_src)

set(CMAKE_CXX_STANDARD 17)

# Определяем версию как макрос
set(VERSION_APP ${search_engine_VERSION_MAJOR}.${search_engine_VERSION_MINOR})
add_definitions(-DVERSION_APP="${VERSION_APP}")

# Создаём исполняемый файл
add_executable(
        search_engine
        main.cpp
        SearchServer.cpp
        InvertedIndex.cpp
        ConverterJSON.cpp
)

# Указываем пути к заголовочным файлам
target_include_directories(search_engine PRIVATE ${MY_INCLUDE_DIR})

# Подключаем nlohmann/json
target_link_libraries(search_engine PRIVATE nlohmann_json::nlohmann_json)
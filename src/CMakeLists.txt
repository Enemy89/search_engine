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
        ${MY_INCLUDE_DIR}/SearchServer.h
        InvertedIndex.cpp
        ${MY_INCLUDE_DIR}/InvertedIndex.h
        ConverterJSON.cpp
        ${MY_INCLUDE_DIR}/ConverterJSON.h
)

# Указываем директории для заголовочных файлов
target_include_directories(search_engine PRIVATE ${MY_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})

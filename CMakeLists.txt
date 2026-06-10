cmake_minimum_required(VERSION 3.22)

project(cpp_cmake_project VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(sbus_reader
    SBUS_Reader.cpp
)

# target_include_directories(sbus_reader
#     PUBLIC
#         ${PROJECT_SOURCE_DIR}/include
# )

add_executable(read
    main.cpp
)

target_link_libraries(read
    PRIVATE
        sbus_reader
)

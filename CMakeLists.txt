cmake_minimum_required(VERSION 3.22)

project(cpp_cmake_project VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # add_compile_options(-Wall -Wextra -Wpedantic -Werror)
  add_link_options("-Wl,-z,relro,-z,now,-z,defs")
endif()

add_library(sbus_reader
    SBUS_Reader.cpp
)

add_executable(read
    main.cpp
)

target_link_libraries(read
    PRIVATE
        sbus_reader
)

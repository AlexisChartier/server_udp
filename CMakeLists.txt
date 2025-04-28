cmake_minimum_required(VERSION 3.10)
project(server_udp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(server_udp ${SOURCES})
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

target_include_directories(server_udp PRIVATE ${PQXX_INCLUDE_DIRS})
target_link_libraries(server_udp ${PQXX_LIBRARIES})
cmake_minimum_required(VERSION 3.16)
project(server_udp)



find_package(PostgreSQL REQUIRED)
find_package(octomap REQUIRED)
find_package(ZLIB REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIRS})
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pour debug : meilleure lisibilité des erreurs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Inclure le dossier include/ dans l'arborescence
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/sudp
    ${CMAKE_SOURCE_DIR}/include/sudp/core
    ${CMAKE_SOURCE_DIR}/include/sudp/db 
    ${CMAKE_SOURCE_DIR}/include/sudp/net 
    ${CMAKE_SOURCE_DIR}/include/sudp/util
    )
link_directories(usr/local/lib)

# Compiler tous les .cpp
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# Création de l'exécutable
add_executable(server_udp ${SRC_FILES})
target_compile_features(server_udp PRIVATE cxx_std_17)
target_link_libraries(server_udp PRIVATE ${PostgreSQL_LIBRARIES})
target_link_libraries(server_udp PRIVATE octomap)
target_link_libraries(server_udp PRIVATE ZLIB::ZLIB)

# Pour ASIO (header-only si standalone, sinon trouve Boost Asio si besoin)
target_link_libraries(server_udp PRIVATE pthread)
target_compile_definitions(server_udp PRIVATE ASIO_STANDALONE)

cmake_minimum_required(VERSION 3.16)
project(server_udp_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
#target_include_directories()
# Trouver libpqxx
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Trouver Boost optionnel (pour lockfree queue)
find_package(Boost OPTIONAL_COMPONENTS lockfree)

# Compiler tous les .cpp
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# Création de l'exécutable
add_executable(server_udp ${SRC_FILES})

# Lien avec libpqxx + dépendances (ex: libpq)
target_include_directories(server_udp PRIVATE ${PQXX_INCLUDE_DIRS})
target_link_libraries(server_udp PRIVATE ${PQXX_LIBRARIES})

# Pour ASIO (header-only si standalone, sinon trouve Boost Asio si besoin)
target_link_libraries(server_udp PRIVATE pthread)

# Définir un flag si Boost Lockfree est trouvé
if (Boost_FOUND)
    target_compile_definitions(server_udp PRIVATE SUDP_HAS_BOOST_LOCKFREE=1)
endif()
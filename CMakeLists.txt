cmake_minimum_required(VERSION 3.14)
project(udp_server_cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Pour debug : meilleure lisibilité des erreurs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Inclure le dossier include/ dans l'arborescence
include_directories(${CMAKE_SOURCE_DIR}/include)

# Trouver libpqxx
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Trouver Boost optionnel (pour lockfree queue)
find_package(Boost OPTIONAL_COMPONENTS lockfree)

# Compiler tous les .cpp
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# Création de l'exécutable
add_executable(udp_server ${SRC_FILES})

# Lien avec libpqxx + dépendances (ex: libpq)
target_include_directories(udp_server PRIVATE ${PQXX_INCLUDE_DIRS})
target_link_libraries(udp_server PRIVATE ${PQXX_LIBRARIES})

# Pour ASIO (header-only si standalone, sinon trouve Boost Asio si besoin)
target_link_libraries(udp_server PRIVATE pthread)

# Définir un flag si Boost Lockfree est trouvé
if (Boost_FOUND)
    target_compile_definitions(udp_server PRIVATE SUDP_HAS_BOOST_LOCKFREE=1)
endif()
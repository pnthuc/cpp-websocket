cmake_minimum_required(VERSION 3.14)
project(ExternalServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32 AND NOT MSVC)
    set(Boost_NO_SYSTEM_PATHS ON) 
endif()

find_package(Boost REQUIRED) 

find_package(nlohmann_json CONFIG)
if(NOT nlohmann_json_FOUND)
    find_path(JSON_INCLUDE_DIR NAMES nlohmann/json.hpp)
    if(JSON_INCLUDE_DIR)
        add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
        set_target_properties(nlohmann_json::nlohmann_json PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${JSON_INCLUDE_DIR}")
    endif()
endif()

add_executable(external_server external_server.cpp)

target_link_libraries(external_server PRIVATE Boost::boost)

if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(external_server PRIVATE nlohmann_json::nlohmann_json)
endif()

if(UNIX)
    target_link_libraries(external_server PRIVATE pthread)
elseif(WIN32)
    target_link_libraries(external_server PRIVATE ws2_32 mswsock)
    target_compile_definitions(external_server PRIVATE _WIN32_WINNT=0x0601) 
endif()
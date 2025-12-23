cmake_minimum_required(VERSION 3.15)
project(cpp-websocket LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system filesystem thread)
find_package(OpenCV REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(features
    src/utils/keylogger.cpp
    src/utils/screen.cpp
    src/utils/lib.cpp
    src/utils/screenshot.cpp
    src/utils/list_app.cpp
    src/utils/list_proc.cpp
    src/utils/camera.cpp
    src/utils/base64.cpp
    src/utils/global.cpp
    src/utils/terminal.cpp
    src/utils/announce.cpp
    src/utils/control.cpp
)

target_include_directories(features PUBLIC 
    ${CMAKE_SOURCE_DIR}/include       
    ${Boost_INCLUDE_DIRS}              
    ${OpenCV_INCLUDE_DIRS}             
)
target_compile_definitions(features PUBLIC FMT_HEADER_ONLY)

target_link_libraries(features PRIVATE
    Boost::system
    Boost::boost
    Boost::filesystem
    Boost::thread
    ${OpenCV_LIBS} 
    ws2_32
    gdi32
    user32
    ntdll
    wininet
    psapi     
    shlwapi
)

add_executable(main src/main.cpp)

target_link_libraries(main PRIVATE features)

if(MINGW)
    get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)

    set(MINGW_RUNTIME_DLLS
        "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
        "${MINGW_BIN_DIR}/libstdc++-6.dll"
        "${MINGW_BIN_DIR}/libwinpthread-1.dll"
    )

    add_custom_command(
        TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MINGW_RUNTIME_DLLS}
        $<TARGET_FILE_DIR:main>
        COMMENT "Copying MinGW runtime DLLs..."
    )
endif()
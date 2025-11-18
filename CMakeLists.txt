cmake_minimum_required(VERSION 3.12)
project(beast_demo LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

# Tìm các thư viện
find_package(Boost REQUIRED COMPONENTS system filesystem thread)
find_package(OpenCV REQUIRED)
find_package(nlohmann_json REQUIRED)

# Tạo library features
add_library(features
    src/utils/keylogger.cpp
    src/utils/screen.cpp
    src/utils/lib.cpp
    src/utils/screenshot.cpp
    src/utils/list_app.cpp
    src/utils/list_proc.cpp
    src/utils/shutdown.cpp
    src/utils/camera.cpp
    src/utils/restart.cpp
    src/utils/base64.cpp
    src/utils/global.cpp
    src/utils/terminal.cpp
    src/utils/announce.cpp
)

# Include directories
target_include_directories(features PUBLIC 
    ${CMAKE_SOURCE_DIR}/include        # thư mục include riêng của project
    ${Boost_INCLUDE_DIRS}              # Boost
    ${OpenCV_INCLUDE_DIRS}             # OpenCV
    C:/Users/pnthu/vcpkg/installed/x64-mingw-dynamic/include  # spdlog & nlohmann-json
    C:/Users/pnthu/vcpkg/installed/x64-mingw-dynamic/include/opencv4
)
target_compile_definitions(features PUBLIC FMT_HEADER_ONLY)
# Link libraries cho features
target_link_libraries(features PRIVATE
    Boost::system
    Boost::boost
    Boost::filesystem
    Boost::thread
    # fmt::fmt
    ${OpenCV_LIBS}  # OpenCV MinGW
    ws2_32
    gdi32
    user32
    ntdll
    wininet
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

    # 3. Thêm một lệnh "sau khi build" để copy các file này
    add_custom_command(
        TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MINGW_RUNTIME_DLLS}
        $<TARGET_FILE_DIR:main>
        COMMENT "Copying MinGW runtime DLLs..."
    )
endif()

# cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/Users/pnthu/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic -DCMAKE_BUILD_TYPE=Release
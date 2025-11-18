#pragma once
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

// extern std::atomic<bool> g_camera;
extern std::atomic<bool> g_camera_active;
extern std::atomic<bool> g_camera_should_stop;
extern std::atomic<bool> g_screen;
extern std::atomic<bool> g_keylogger; 
extern std::mutex ws_mutex;

extern std::mutex g_control_mutex;

extern std::thread camera_thread;
extern std::thread screen_thread;
extern std::thread keylogger_thread; 

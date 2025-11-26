#pragma once
#include "lib.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shellapi.h>
#include <vector>
#include <string>

#pragma comment(lib, "Psapi.lib")

struct AppInfo {
    std::string name;
    std::string installPath; // Đường dẫn thư mục cài đặt
    std::string execPath;    // Đường dẫn file exe chính
    bool isRunning;
    std::vector<DWORD> pids; // Vẫn giữ để check running, nhưng không gửi về client
};

void list_applications(websocket::stream<tcp::socket>& ws);
void start_application_exec(const std::string& appName);
// [NEW] Stop bằng tên file exe
void stop_application_by_name(const std::string& exeName, websocket::stream<tcp::socket>& ws);
// Giữ hàm cũ cho tab Processes
void stop_application_exec(DWORD pid, websocket::stream<tcp::socket>& ws);
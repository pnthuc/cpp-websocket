#include "list_proc.h"
#include "encode.h" 

void list_processes(websocket::stream<tcp::socket>& ws) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        sendMsg(ws, "text", "Error", "Failed to create snapshot");
        file_logger->error("Failed to create process snapshot.");
        return;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    std::vector<std::pair<DWORD, std::string>> processes;

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            std::string exeName = WinEncoding::ToUtf8(pe32.szExeFile);
            processes.emplace_back(pe32.th32ProcessID, exeName);
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);

    std::string binaryData;
    constexpr size_t NAME_WIDTH = 40;
    constexpr size_t PID_WIDTH  = 8;

    if (!processes.empty()) {
        binaryData += "--- Process List ---\n";
        binaryData += "Name";
        if (NAME_WIDTH > 4) binaryData.append(NAME_WIDTH - 4, ' ');
        binaryData += "PID\n";
        binaryData += std::string(NAME_WIDTH + PID_WIDTH, '-') + "\n";

        for (const auto& app : processes) {
            std::string name = app.second;
            std::string pid  = std::to_string(app.first);
            if (name.size() > NAME_WIDTH) name = name.substr(0, NAME_WIDTH);

            std::string name_padded = name;
            if (name_padded.size() < NAME_WIDTH)
                name_padded.append(NAME_WIDTH - name_padded.size(), ' ');

            std::string pid_padded;
            if (pid.size() < PID_WIDTH)
                pid_padded.append(PID_WIDTH - pid.size(), ' ');
            pid_padded += pid;

            binaryData += name_padded + pid_padded + "\n";
        }
        sendMsg(ws, "binary", "Processes", std::vector<BYTE>(binaryData.begin(), binaryData.end()));
        file_logger->info("Listed processes.");
    } else {
        sendMsg(ws, "text", "Info", "No processes found.");
        file_logger->info("No processes found.");
    }
}

void handle_kill_process(const nlohmann::json& data, websocket::stream<tcp::socket>& ws) {
    try {
        if (!data.contains("pid")) {
            sendMsg(ws, "text", "Error", "PID not provided.");
            return;
        }

        uint32_t pid = 0;
        bool pid_ok = false;
        if (data["pid"].is_number_integer()) {
            pid = data["pid"].get<uint32_t>();
            pid_ok = true;
        } else if (data["pid"].is_string()) {
            try {
                pid = static_cast<uint32_t>(std::stoul(data["pid"].get<std::string>()));
                pid_ok = true;
            } catch (...) {}
        }

        if (!pid_ok) {
            sendMsg(ws, "text", "Error", "Invalid PID format.");
            return;
        }

        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
        if (hProcess == NULL) {
            sendMsg(ws, "text", "Error", "Failed to open process PID " + std::to_string(pid));
            file_logger->error("Failed to open process PID {} for termination.", pid);
            return;
        } else {
            if (TerminateProcess(hProcess, 0)) {
                sendMsg(ws, "text", "Info", "Terminated PID " + std::to_string(pid));
                file_logger->info("Killed process PID {}", pid);
            } else {
                sendMsg(ws, "text", "Error", "Failed to terminate PID " + std::to_string(pid));
                file_logger->error("Failed to terminate process PID {}", pid);
            }
            CloseHandle(hProcess);
        }
    } catch (const std::exception& e) {
        if(file_logger) file_logger->error("Error killing process: {}", e.what());
        sendMsg(ws, "text", "Error", std::string("Exception: ") + e.what());
    }
}
#include "list_proc.h"

void list_processes(websocket::stream<tcp::socket>& ws) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        sendMsg(ws, "text", "Error", "Failed to create snapshot");
        return;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    std::vector<std::pair<DWORD, std::string>> processes;

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            int len = WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, nullptr, 0, nullptr, nullptr);
            if (len > 0) {
                std::vector<char> buffer(len);
                WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, buffer.data(), len, nullptr, nullptr);
                std::string exeName(buffer.data());
                processes.emplace_back(pe32.th32ProcessID, exeName);
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);

    std::string binaryData;
    constexpr size_t NAME_WIDTH = 40;
    constexpr size_t PID_WIDTH  = 8;

    if (!processes.empty()) {
        binaryData += "--- Cac Processes ---\n";
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
    } else {
        sendMsg(ws, "text", "Info", "No processes found.");
    }
}
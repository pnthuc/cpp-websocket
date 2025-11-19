#include "list_app.h"
#include "encode.h" 

std::map<DWORD, std::string> getParentProcesses() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        file_logger->error("Failed to create process snapshot for listing applications.");
        return {};
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    std::set<DWORD> parentPIDs;
    std::map<DWORD, ProcessInfo> allProcesses;

    if (!Process32FirstW(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return {};
    }

    do {
        ProcessInfo info;
        info.pid = pe32.th32ProcessID;
        info.parentPid = pe32.th32ParentProcessID;
        
        info.name = WinEncoding::ToUtf8(pe32.szExeFile);
        allProcesses[info.pid] = info;

        if (info.parentPid > 0)
            parentPIDs.insert(info.parentPid);

    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);

    std::map<DWORD, std::string> rootProcesses;

    for (const auto& pair : allProcesses) {
        DWORD currentPID = pair.first;
        const ProcessInfo& info = pair.second;

        if (parentPIDs.count(currentPID))
            if (info.pid > 4 && !info.name.empty())
                rootProcesses[info.pid] = info.name;
    }

    return rootProcesses;
}

void list_applications(websocket::stream<tcp::socket>& ws) {
    std::map<DWORD, std::string> applications = getParentProcesses();
    std::string binaryData;

    constexpr size_t NAME_WIDTH = 40;
    constexpr size_t PID_WIDTH  = 8;

    binaryData += "--- Parent Processes (Apps) ---\n";
    binaryData += "Name";
    if (NAME_WIDTH > 4) binaryData.append(NAME_WIDTH - 4, ' ');
    binaryData += "PID\n";
    binaryData += std::string(NAME_WIDTH + PID_WIDTH, '-') + "\n";
    
    for (const auto& app : applications) {
        std::string name = app.second;
        std::string pid = std::to_string(app.first);

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

    if (!binaryData.empty()) {
        sendMsg(ws, "binary", "Applications", 
                std::vector<BYTE>(binaryData.begin(), binaryData.end()));
        file_logger->info("Listed applications.");
    } else {
        sendMsg(ws, "text", "Info", "No applications found.");
        file_logger->info("No applications found.");
    }
}
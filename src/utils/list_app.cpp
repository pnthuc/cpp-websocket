#include "list_app.h"

std::string wstring_to_string(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
                                          (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
                        (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
    return strTo;
}

std::map<DWORD, std::string> getParentProcesses() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateToolhelp32Snapshot failed!" << std::endl;
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
        info.name = wstring_to_string(pe32.szExeFile);
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

// === Hàm gửi danh sách ứng dụng qua WebSocket ===
void list_applications(websocket::stream<tcp::socket>& ws) {
    std::map<DWORD, std::string> applications = getParentProcesses();
    std::string binaryData;

    constexpr size_t NAME_WIDTH = 40;
    constexpr size_t PID_WIDTH  = 8;

    binaryData += "--- Cac Ung Dung Goc (Process Cha) ---\n";
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
    } else {
        sendMsg(ws, "text", "Info", "No applications found.");
    }
}
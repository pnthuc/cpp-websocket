#include "list_proc.h"
#include "encode.h" 
#include "lib.h"

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "advapi32.lib") 

bool EnableDebugPrivilege() {
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
        return false;

    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = luid;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bool result = AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    CloseHandle(hToken);
    return result;
}

std::string get_memory_usage(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    
    if (NULL == hProcess) {
        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    }

    if (NULL == hProcess) return "N/A"; 

    PROCESS_MEMORY_COUNTERS pmc;
    std::string result = "0 MB";

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        double memMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << memMB << " MB";
        result = ss.str();
    }

    CloseHandle(hProcess);
    return result;
}

void list_processes(websocket::stream<tcp::socket>& ws) {
    EnableDebugPrivilege();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        sendMsg(ws, "text", "Error", "Failed to create snapshot");
        return;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    json procArray = json::array(); 

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == 0) continue; 

            std::string exeName = WinEncoding::ToUtf8(pe32.szExeFile);
            std::string ram = get_memory_usage(pe32.th32ProcessID);

            json item;
            item["name"] = exeName;
            item["pid"] = pe32.th32ProcessID;
            item["memory"] = ram;
            
            procArray.push_back(item);

        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);

    sendMsg(ws, "json", "Processes", procArray.dump());
    
    if (file_logger) file_logger->info("Listed {} processes.", procArray.size());
}

void handle_kill_process(const nlohmann::json& data, websocket::stream<tcp::socket>& ws) {
    EnableDebugPrivilege(); 

    try {
        if (!data.contains("pid")) return;
        
        DWORD pid = 0;
        if (data["pid"].is_number()) pid = data["pid"].get<DWORD>();
        else if (data["pid"].is_string()) pid = std::stoul(data["pid"].get<std::string>());
        
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess == NULL) {
            sendMsg(ws, "text", "Error", "Access Denied or PID not found: " + std::to_string(pid));
            return;
        }
        
        if (TerminateProcess(hProcess, 0)) {
            sendMsg(ws, "text", "Info", "Killed process PID " + std::to_string(pid));
        } else {
            sendMsg(ws, "text", "Error", "Failed to kill PID " + std::to_string(pid));
        }
        CloseHandle(hProcess);
    } catch (const std::exception& e) {
        sendMsg(ws, "text", "Error", std::string("Exception: ") + e.what());
    }
}
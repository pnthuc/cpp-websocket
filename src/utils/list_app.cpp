#include "list_app.h"
#include "encode.h"
#include "lib.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

// --- UTILS ---

std::wstring ToLowerW(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

std::wstring CleanPath(std::wstring path) {
    if (path.empty()) return L"";

    if (path.front() == L'\"') {
        size_t q = path.find(L'\"', 1);
        if (q != std::wstring::npos) path = path.substr(1, q - 1);
    }

    std::wstring lower = ToLowerW(path);
    size_t exePos = lower.find(L".exe");
    if (exePos != std::wstring::npos) {
        path = path.substr(0, exePos + 4);
    } else {
        size_t comma = path.find(L',');
        if (comma != std::wstring::npos) path = path.substr(0, comma);
    }
    return path;
}

struct AppEntry {
    std::wstring displayName;
    std::wstring execPath;
    bool isRunning = false;
};

// --- REGISTRY LOGIC ---

void ScanRegistryKey(HKEY hRoot, const wchar_t* subKey, std::vector<AppEntry>& apps, std::set<std::wstring>& seenNames) {
    HKEY hKey;
    if (RegOpenKeyExW(hRoot, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return;

    DWORD index = 0;
    WCHAR keyName[256];
    DWORD keyNameLen = 256;

    while (RegEnumKeyExW(hKey, index++, keyName, &keyNameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        keyNameLen = 256; 
        std::wstring fullPath = std::wstring(subKey) + L"\\" + keyName;
        HKEY hSubKey;

        if (RegOpenKeyExW(hRoot, fullPath.c_str(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
            WCHAR buf[1024];
            DWORD size = sizeof(buf);
            DWORD type;

            if (RegQueryValueExW(hSubKey, L"DisplayName", NULL, &type, (LPBYTE)buf, &size) == ERROR_SUCCESS) {
                std::wstring name = buf;
                
                if (!name.empty() && seenNames.find(name) == seenNames.end()) {
                    
                    DWORD sysComp = 0; 
                    DWORD dwSz = sizeof(sysComp);
                    if (RegQueryValueExW(hSubKey, L"SystemComponent", NULL, NULL, (LPBYTE)&sysComp, &dwSz) == ERROR_SUCCESS && sysComp == 1) {
                        RegCloseKey(hSubKey);
                        continue;
                    }

                    if (name.find(L"KB") == 0 && name.length() > 2 && isdigit(name[2])) {
                        RegCloseKey(hSubKey);
                        continue;
                    }

                    AppEntry app;
                    app.displayName = name;
                    app.execPath = L"";

                    size = sizeof(buf);
                    if (RegQueryValueExW(hSubKey, L"DisplayIcon", NULL, &type, (LPBYTE)buf, &size) == ERROR_SUCCESS) {
                        std::wstring iconPath = CleanPath(buf);
                        if (iconPath.find(L".exe") != std::wstring::npos) {
                            app.execPath = iconPath;
                        }
                    }

                    if (app.execPath.empty()) {
                         size = sizeof(buf);
                         if (RegQueryValueExW(hSubKey, L"InstallLocation", NULL, &type, (LPBYTE)buf, &size) == ERROR_SUCCESS) {
                         }
                    }

                    apps.push_back(app);
                    seenNames.insert(name);
                }
            }
            RegCloseKey(hSubKey);
        }
    }
    RegCloseKey(hKey);
}

// --- CHECK RUNNING PROCESSES ---
struct ProcShort {
    std::wstring name;
    std::wstring path;
};

std::vector<ProcShort> GetRunningProcessList() {
    std::vector<ProcShort> list;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return list;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(hSnap, &pe)) {
        do {
            ProcShort p;
            p.name = ToLowerW(pe.szExeFile);
            
            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
            if (hProc) {
                WCHAR pathBuf[MAX_PATH];
                DWORD size = MAX_PATH;
                if (QueryFullProcessImageNameW(hProc, 0, pathBuf, &size)) {
                    p.path = ToLowerW(pathBuf);
                }
                CloseHandle(hProc);
            }
            if (p.path.empty()) p.path = p.name; // Fallback

            list.push_back(p);
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return list;
}

// --- MAIN FUNCTIONS ---

void list_applications(websocket::stream<tcp::socket>& ws) {
    std::vector<AppEntry> apps;
    std::set<std::wstring> seenNames;

    ScanRegistryKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", apps, seenNames);
    
    ScanRegistryKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", apps, seenNames);
    
    ScanRegistryKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall", apps, seenNames);

    auto runningProcs = GetRunningProcessList();
    json arr = json::array();

    for (auto& app : apps) {
        if (!app.execPath.empty()) {
            std::wstring lowerExec = ToLowerW(app.execPath);
            for (const auto& p : runningProcs) {
                if (p.path == lowerExec) {
                    app.isRunning = true; 
                    break;
                }
                if (fs::path(lowerExec).filename() == p.name) {
                    app.isRunning = true;
                    break;
                }
            }
        }

        json j;
        j["name"] = WinEncoding::ToUtf8(app.displayName);
        j["is_running"] = app.isRunning;
        j["exec_path"] = WinEncoding::ToUtf8(app.execPath);
        arr.push_back(j);
    }

    sendMsg(ws, "json", "Applications", arr.dump());
    if (file_logger) file_logger->info("Listed {} apps (Fast Mode).", arr.size());
}

void start_application_exec(const std::string& appPathUtf8) {
    if (appPathUtf8.empty()) return;
    std::wstring wPath = WinEncoding::FromUtf8(appPathUtf8);
    
    std::wstring wDir = L"";
    size_t lastSlash = wPath.find_last_of(L"\\");
    if (lastSlash != std::wstring::npos) wDir = wPath.substr(0, lastSlash);

    ShellExecuteW(NULL, L"open", wPath.c_str(), NULL, wDir.empty() ? NULL : wDir.c_str(), SW_SHOWNORMAL);
}

void stop_application_by_name(const std::string& exeNameUtf8, websocket::stream<tcp::socket>& ws) {
    std::string cmd = "taskkill /F /IM \"" + exeNameUtf8 + "\" /T";
    system(cmd.c_str());
    sendMsg(ws, "text", "Info", "Sent kill command for: " + exeNameUtf8);
}

void stop_application_exec(DWORD pid, websocket::stream<tcp::socket>& ws) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess) {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        sendMsg(ws, "text", "Info", "Terminated PID " + std::to_string(pid));
    } else {
        sendMsg(ws, "text", "Error", "Failed to terminate PID " + std::to_string(pid));
    }
}
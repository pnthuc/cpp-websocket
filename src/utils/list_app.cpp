#include "list_app.h"
#include "encode.h"
#include <algorithm>
#include <regex>
#include <vector>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

std::string toLowerCase(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}
std::string cleanString(const std::string& input) {
    std::string output = toLowerCase(input);
    output = std::regex_replace(output, std::regex("[^a-z0-9]"), "");
    return output;
}
std::string WideToBytes(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
std::wstring ToLowerW(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

struct AppInfoW {
    std::wstring name;
    std::wstring installLocation;
    bool isRunning;
    std::vector<DWORD> pids;
    std::wstring execPath; 
};

struct ProcessData {
    DWORD pid;
    std::wstring exeName;
    std::wstring fullPath;
};

void start_application_exec(const std::string& appName) {
    HINSTANCE result = ShellExecuteA(NULL, "open", appName.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if ((intptr_t)result > 32) {
        file_logger->info("Started: {}", appName);
    } else {
        file_logger->error("Start failed: {}", appName);
    }
}

void stop_application_by_name(const std::string& exeName, websocket::stream<tcp::socket>& ws) {
    std::string cmd = "taskkill /F /IM \"" + exeName + "\" /T";
    
    int result = system(cmd.c_str());
    
    if (result == 0) {
        sendMsg(ws, "text", "Info", "Stopped application: " + exeName);
        file_logger->info("Stopped application {} using TaskKill /IM", exeName);
    } else {
        sendMsg(ws, "text", "Error", "Failed to stop application: " + exeName);
        file_logger->error("Failed to stop application {}", exeName);
    }
}

void stop_application_exec(DWORD pid, websocket::stream<tcp::socket>& ws) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) {
        sendMsg(ws, "text", "Error", "Cannot open PID " + std::to_string(pid));
        return;
    }
    TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    sendMsg(ws, "text", "Info", "Stopped PID " + std::to_string(pid));
}

std::vector<ProcessData> GetExtendedRunningProcesses() {
    std::vector<ProcessData> procs;
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) return procs;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hProcessSnap, &pe32)) {
        do {
            ProcessData data;
            data.pid = pe32.th32ProcessID;
            data.exeName = ToLowerW(pe32.szExeFile);
            data.fullPath = L"";

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                WCHAR path[MAX_PATH];
                if (GetModuleFileNameExW(hProcess, NULL, path, MAX_PATH)) {
                    data.fullPath = ToLowerW(path);
                }
                CloseHandle(hProcess);
            }
            procs.push_back(data);
        } while (Process32NextW(hProcessSnap, &pe32));
    }
    CloseHandle(hProcessSnap);
    return procs;
}

void ScanRegistryKey(HKEY hRootKey, const wchar_t* subKey, REGSAM samDesired, std::vector<AppInfoW>& apps) {
    HKEY hKey;
    if (RegOpenKeyExW(hRootKey, subKey, 0, KEY_READ | samDesired, &hKey) != ERROR_SUCCESS) return;

    DWORD index = 0;
    WCHAR keyName[255];
    DWORD keyNameLen = 255;

    while (RegEnumKeyExW(hKey, index, keyName, &keyNameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        HKEY hSubKey;
        std::wstring fullPath = std::wstring(subKey) + L"\\" + keyName;

        if (RegOpenKeyExW(hRootKey, fullPath.c_str(), 0, KEY_READ | samDesired, &hSubKey) == ERROR_SUCCESS) {
            WCHAR displayName[1024];
            DWORD dataSize = sizeof(displayName);
            
            DWORD systemComponent = 0;
            DWORD scSize = sizeof(systemComponent);
            if (RegQueryValueExW(hSubKey, L"SystemComponent", NULL, NULL, (LPBYTE)&systemComponent, &scSize) == ERROR_SUCCESS) {
                if (systemComponent == 1) { RegCloseKey(hSubKey); keyNameLen = 255; index++; continue; }
            }

            if (RegQueryValueExW(hSubKey, L"DisplayName", NULL, NULL, (LPBYTE)displayName, &dataSize) == ERROR_SUCCESS) {
                AppInfoW app;
                app.name = displayName;
                app.isRunning = false;

                WCHAR valBuffer[1024];
                DWORD valSize = sizeof(valBuffer);
                if (RegQueryValueExW(hSubKey, L"InstallLocation", NULL, NULL, (LPBYTE)valBuffer, &valSize) == ERROR_SUCCESS) {
                    std::wstring loc = valBuffer;
                    loc.erase(std::remove(loc.begin(), loc.end(), '\"'), loc.end());
                    if (!loc.empty() && loc.back() == L'\\') loc.pop_back();
                    app.installLocation = ToLowerW(loc);
                } else {
                    app.installLocation = L"";
                }

                valSize = sizeof(valBuffer);
                if (RegQueryValueExW(hSubKey, L"DisplayIcon", NULL, NULL, (LPBYTE)valBuffer, &valSize) == ERROR_SUCCESS) {
                    std::wstring icon = valBuffer;
                    size_t comma = icon.find_last_of(L',');
                    if (comma != std::wstring::npos) icon = icon.substr(0, comma);
                    icon.erase(std::remove(icon.begin(), icon.end(), '\"'), icon.end());
                    
                    if (icon.length() > 4 && icon.substr(icon.length() - 4) == L".exe") {
                        app.execPath = ToLowerW(icon);
                    }
                }

                if (app.name.length() > 2 && app.name.find(L"Update") == std::wstring::npos) {
                    apps.push_back(app);
                }
            }
            RegCloseKey(hSubKey);
        }
        keyNameLen = 255;
        index++;
    }
    RegCloseKey(hKey);
}

void GetInstalledApps(std::vector<AppInfoW>& apps) {
    const wchar_t* uninstallPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    ScanRegistryKey(HKEY_CURRENT_USER, uninstallPath, 0, apps);
    ScanRegistryKey(HKEY_LOCAL_MACHINE, uninstallPath, KEY_WOW64_64KEY, apps);
    ScanRegistryKey(HKEY_LOCAL_MACHINE, uninstallPath, KEY_WOW64_32KEY, apps);
}

std::wstring GetHardcodedExe(const std::wstring& appName) {
    std::wstring lower = ToLowerW(appName);
    if (lower.find(L"microsoft edge") != std::wstring::npos) return L"msedge.exe";
    if (lower.find(L"google chrome") != std::wstring::npos) return L"chrome.exe";
    if (lower.find(L"firefox") != std::wstring::npos) return L"firefox.exe";
    if (lower.find(L"visual studio code") != std::wstring::npos) return L"code.exe";
    if (lower.find(L"zalo") != std::wstring::npos) return L"zalo.exe";
    return L"";
}

void MatchApps(std::vector<AppInfoW>& apps, const std::vector<ProcessData>& procs) {
    for (auto& app : apps) {
        std::wstring lowerName = ToLowerW(app.name);
        std::wstring hardCodedExe = GetHardcodedExe(app.name);

        for (const auto& proc : procs) {
            bool matched = false;

            if (!hardCodedExe.empty() && proc.exeName == hardCodedExe) matched = true;

            if (!matched && !app.execPath.empty() && !proc.fullPath.empty()) {
                if (proc.fullPath == app.execPath) matched = true;
            }

            if (!matched && !app.installLocation.empty() && !proc.fullPath.empty()) {
                if (proc.fullPath.find(app.installLocation) != std::wstring::npos) matched = true;
            }

            if (!matched) {
                std::wstring procBase = proc.exeName;
                size_t pos = procBase.find(L".exe");
                if (pos != std::wstring::npos) procBase = procBase.substr(0, pos);
                if (procBase.length() > 3 && lowerName.find(procBase) != std::wstring::npos) matched = true;
            }

            if (matched) {
                app.isRunning = true;
                app.pids.push_back(proc.pid);
                if (app.execPath.empty() && !proc.fullPath.empty()) {
                    app.execPath = proc.fullPath;
                }
            }
        }
    }
}

void list_applications(websocket::stream<tcp::socket>& ws) {
    std::vector<AppInfoW> appsW;
    GetInstalledApps(appsW);

    std::sort(appsW.begin(), appsW.end(), [](const AppInfoW& a, const AppInfoW& b) { return a.name < b.name; });
    auto last = std::unique(appsW.begin(), appsW.end(), [](const AppInfoW& a, const AppInfoW& b) { return a.name == b.name; });
    appsW.erase(last, appsW.end());

    std::vector<ProcessData> procs = GetExtendedRunningProcesses();
    MatchApps(appsW, procs);

    json jsonList = json::array();
    for (const auto& app : appsW) {
        std::string sName = WideToBytes(app.name);
        
        std::string sPath = WideToBytes(app.execPath.empty() ? app.installLocation : app.execPath);

        json item;
        item["name"] = sName;
        item["exec_path"] = sPath;
        item["is_running"] = app.isRunning;
        jsonList.push_back(item);
    }

    sendJsonMsg(ws, "json", "Applications", jsonList);
    file_logger->info("Listed {} apps.", appsW.size());
}
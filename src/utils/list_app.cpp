#include "list_app.h"
#include "encode.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shlwapi.h>
#include <algorithm>
#include <vector>
#include <set>
#include <filesystem>
#include <map>

#pragma comment(lib, "shlwapi.lib")

namespace fs = std::filesystem;

// --- CẤU HÌNH ---
const std::vector<std::wstring> BLACKLIST = {
    L"uninstall", L"uninst", L"setup", L"update", 
    L"helper", L"config", L"launcher", L"report", L"crash"
};

std::wstring ToLowerW(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

// Kiểm tra xem file có phải rác không
bool IsJunkFile(const std::wstring& name) {
    std::wstring lower = ToLowerW(name);
    if (lower.find(L".exe") == std::wstring::npos) return true; // Phải là exe
    for (const auto& w : BLACKLIST) {
        if (lower.find(w) != std::wstring::npos) return true;
    }
    return false;
}

std::wstring CleanPath(std::wstring path) {
    if (path.empty()) return L"";
    size_t comma = path.find(L',');
    if (comma != std::wstring::npos) path = path.substr(0, comma);
    path.erase(std::remove(path.begin(), path.end(), L'\"'), path.end());
    return path;
}

// Logic mới: Quét nhanh (Non-recursive) và chấm điểm
std::wstring FindBestExeInFolder(std::wstring folderPath, std::wstring appName) {
    try {
        if (folderPath.empty() || !fs::exists(folderPath) || !fs::is_directory(folderPath)) 
            return L"";

        std::wstring bestMatch = L"";
        int maxScore = -1;
        uintmax_t maxFileSize = 0;
        std::wstring lowerAppName = ToLowerW(appName);

        // Chỉ quét thư mục gốc, KHÔNG đệ quy để tránh lag
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.is_regular_file()) {
                std::wstring filename = entry.path().filename().wstring();
                
                if (IsJunkFile(filename)) continue;

                int currentScore = 0;
                std::wstring lowerFilename = ToLowerW(filename);

                // Tiêu chí 1: Tên file chứa tên App (Ví dụ: "chrome" trong "Google Chrome")
                // Tách tên App thành các từ khóa
                if (lowerAppName.find(L" ") != std::wstring::npos) {
                     // Logic đơn giản: Nếu tên file chứa 1 phần của tên App
                     // Ví dụ App: "Visual Studio Code", File: "code.exe" -> Khớp "code"
                     if (lowerAppName.find(lowerFilename.substr(0, lowerFilename.length()-4)) != std::wstring::npos) {
                         currentScore += 50;
                     }
                } else {
                    if (lowerFilename.find(lowerAppName) != std::wstring::npos) currentScore += 50;
                }

                // Tiêu chí 2: Dung lượng file (File chính thường > 100KB)
                uintmax_t fSize = entry.file_size();
                if (fSize > 1024 * 1024) currentScore += 20; // > 1MB
                else if (fSize < 50 * 1024) currentScore -= 10; // < 50KB (thường là rác)

                // Cập nhật Best Match
                // Nếu điểm bằng nhau, lấy file nặng hơn
                if (currentScore > maxScore || (currentScore == maxScore && fSize > maxFileSize)) {
                    maxScore = currentScore;
                    maxFileSize = fSize;
                    bestMatch = entry.path().wstring();
                }
            }
        }
        return bestMatch;
    } catch (...) { return L""; }
}

struct AppEntry {
    std::wstring displayName;
    std::wstring displayIcon;
    std::wstring installLocation;
    bool isRunning = false;
    std::wstring finalExecPath;
};

struct RunningProc {
    std::wstring exeName;
    std::wstring fullPath;
};

std::vector<RunningProc> GetAllRunningProcs() {
    std::vector<RunningProc> list;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return list;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            RunningProc p;
            p.exeName = ToLowerW(pe32.szExeFile);
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                WCHAR path[MAX_PATH];
                if (GetModuleFileNameExW(hProcess, NULL, path, MAX_PATH)) {
                    p.fullPath = ToLowerW(path);
                }
                CloseHandle(hProcess);
            }
            list.push_back(p);
        } while (Process32NextW(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return list;
}

void ScanRegistry(HKEY hRoot, const wchar_t* subKey, std::vector<AppEntry>& apps) {
    HKEY hKey;
    if (RegOpenKeyExW(hRoot, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return;

    DWORD index = 0;
    WCHAR keyName[255];
    DWORD keyNameLen = 255;

    while (RegEnumKeyExW(hKey, index++, keyName, &keyNameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        keyNameLen = 255;
        std::wstring fullPath = std::wstring(subKey) + L"\\" + keyName;
        HKEY hSubKey;

        if (RegOpenKeyExW(hRoot, fullPath.c_str(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
            WCHAR nameBuf[512];
            DWORD size = sizeof(nameBuf);
            
            if (RegQueryValueExW(hSubKey, L"DisplayName", NULL, NULL, (LPBYTE)nameBuf, &size) == ERROR_SUCCESS) {
                AppEntry app;
                app.displayName = nameBuf;

                WCHAR locBuf[1024];
                DWORD locSize = sizeof(locBuf);
                if (RegQueryValueExW(hSubKey, L"InstallLocation", NULL, NULL, (LPBYTE)locBuf, &locSize) == ERROR_SUCCESS) {
                    app.installLocation = CleanPath(locBuf);
                }

                WCHAR iconBuf[1024];
                DWORD iconSize = sizeof(iconBuf);
                if (RegQueryValueExW(hSubKey, L"DisplayIcon", NULL, NULL, (LPBYTE)iconBuf, &iconSize) == ERROR_SUCCESS) {
                    app.displayIcon = CleanPath(iconBuf);
                }

                // --- LOGIC LẤY PATH TỐI ƯU ---
                std::wstring candidate = L"";

                // 1. Nếu DisplayIcon là EXE xịn, dùng luôn (Nhanh nhất)
                if (!app.displayIcon.empty() && 
                    app.displayIcon.find(L".exe") != std::wstring::npos && 
                    fs::exists(app.displayIcon) && 
                    !IsJunkFile(fs::path(app.displayIcon).filename().wstring())) 
                {
                    candidate = app.displayIcon;
                }
                
                // 2. Nếu chưa có, quét nhanh thư mục InstallLocation
                if (candidate.empty() && !app.installLocation.empty()) {
                    candidate = FindBestExeInFolder(app.installLocation, app.displayName);
                }

                // [QUAN TRỌNG] Nếu không tìm thấy EXE, vẫn hiện tên App để user biết
                // Nhưng field exec_path sẽ rỗng, nút Launch sẽ bị vô hiệu hoá ở Client (hoặc báo lỗi)
                app.finalExecPath = candidate;
                
                // Loại bỏ mấy cái tên quá ngắn hoặc rác hệ thống
                if (app.displayName.length() > 3 && app.displayName.find(L"Update") == std::wstring::npos) {
                    apps.push_back(app);
                }
            }
            RegCloseKey(hSubKey);
        }
    }
    RegCloseKey(hKey);
}

void start_application_exec(const std::string& appPath) {
    if (appPath.empty()) return;
    ShellExecuteA(NULL, "open", appPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void stop_application_by_name(const std::string& exeName, websocket::stream<tcp::socket>& ws) {
    std::string cmd = "taskkill /F /IM \"" + exeName + "\" /T";
    system(cmd.c_str());
    sendMsg(ws, "text", "Info", "Stop command sent: " + exeName);
}

void stop_application_exec(DWORD pid, websocket::stream<tcp::socket>& ws) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess) {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        sendMsg(ws, "text", "Info", "Stopped PID " + std::to_string(pid));
    }
}

void list_applications(websocket::stream<tcp::socket>& ws) {
    std::vector<AppEntry> installedApps;
    const wchar_t* uninstallPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    
    // Chỉ quét Local Machine và Wow6432Node là đủ 90% app rồi.
    // Quét Current User đôi khi ra rác, có thể bỏ nếu muốn nhanh hơn nữa.
    ScanRegistry(HKEY_LOCAL_MACHINE, uninstallPath, installedApps);
    ScanRegistry(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", installedApps);
    ScanRegistry(HKEY_CURRENT_USER, uninstallPath, installedApps);

    std::vector<RunningProc> runningProcs = GetAllRunningProcs();
    json jsonArray = json::array();
    std::set<std::wstring> processedNames;

    for (auto& app : installedApps) {
        if (processedNames.count(app.displayName)) continue;
        processedNames.insert(app.displayName);

        std::wstring lowerExec = ToLowerW(app.finalExecPath);
        
        // Check Running
        if (!lowerExec.empty()) {
            for (const auto& proc : runningProcs) {
                if (lowerExec == proc.fullPath) {
                    app.isRunning = true;
                    break;
                }
                // Fallback: Check tên file
                if (fs::path(lowerExec).filename().wstring() == proc.exeName) {
                    app.isRunning = true;
                    break;
                }
            }
        }

        json item;
        item["name"] = WinEncoding::ToUtf8(app.displayName);
        item["is_running"] = app.isRunning;
        item["exec_path"] = WinEncoding::ToUtf8(app.finalExecPath); 
        jsonArray.push_back(item);
    }

    sendMsg(ws, "json", "Applications", jsonArray.dump());
    if (file_logger) file_logger->info("Listed {} apps.", jsonArray.size());
}
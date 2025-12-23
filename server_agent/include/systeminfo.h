#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <iomanip>
class SystemMonitor {
private:
    unsigned long long fileTimeToInt64(const FILETIME& ft) {
        return (((unsigned long long)(ft.dwHighDateTime)) << 32) | ((unsigned long long)ft.dwLowDateTime);
    }

    unsigned long long preIdleTime = 0;
    unsigned long long preKernelTime = 0;
    unsigned long long preUserTime = 0;

public:
    SystemMonitor() {
        FILETIME idle, kernel, user;
        GetSystemTimes(&idle, &kernel, &user);
        preIdleTime = fileTimeToInt64(idle);
        preKernelTime = fileTimeToInt64(kernel);
        preUserTime = fileTimeToInt64(user);
    }

    double getCpuUsage() {
        FILETIME idle, kernel, user;
        GetSystemTimes(&idle, &kernel, &user);

        unsigned long long idleTime = fileTimeToInt64(idle);
        unsigned long long kernelTime = fileTimeToInt64(kernel);
        unsigned long long userTime = fileTimeToInt64(user);

        unsigned long long idleDelta = idleTime - preIdleTime;
        unsigned long long kernelDelta = kernelTime - preKernelTime;
        unsigned long long userDelta = userTime - preUserTime;

        preIdleTime = idleTime;
        preKernelTime = kernelTime;
        preUserTime = userTime;

        unsigned long long totalSys = kernelDelta + userDelta;
        
        if (totalSys == 0) return 0.0;

        return (double)(totalSys - idleDelta) * 100.0 / totalSys;
    }

    struct RamInfo {
        int percent;
        double usedGB;
        double totalGB;
    };

    RamInfo getRamUsage() {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);

        double totalPhysGB = memInfo.ullTotalPhys / (1024.0 * 1024 * 1024);
        double availPhysGB = memInfo.ullAvailPhys / (1024.0 * 1024 * 1024);
        double usedPhysGB = totalPhysGB - availPhysGB;

        return RamInfo{ (int)memInfo.dwMemoryLoad, usedPhysGB, totalPhysGB };
    }

    std::string getSystemUptime() {
        unsigned long long tick = GetTickCount64(); 
        unsigned long long seconds = tick / 1000;
        
        int days = seconds / 86400;
        int hours = (seconds % 86400) / 3600;
        int minutes = (seconds % 3600) / 60;
        int secs = seconds % 60;

        std::string uptime = std::to_string(days) + "d " + 
                             std::to_string(hours) + "h " + 
                             std::to_string(minutes) + "m " + 
                             std::to_string(secs) + "s";
        return uptime;
    }
};
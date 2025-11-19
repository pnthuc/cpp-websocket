#pragma once
#include "lib.h"
#include <tlhelp32.h>

struct ProcessInfo {
    DWORD pid;
    DWORD parentPid;
    std::string name;
};

void list_applications(websocket::stream<tcp::socket>& ws);
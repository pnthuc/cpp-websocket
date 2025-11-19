#pragma once
#include "lib.h"
#include <tlhelp32.h>

void list_processes(websocket::stream<tcp::socket>& ws);
void handle_kill_process(const nlohmann::json& data, websocket::stream<tcp::socket>& ws);
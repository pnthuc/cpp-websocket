#pragma once
#include "lib.h"

void executeCommand(const std::string& command, const std::string& path, websocket::stream<tcp::socket>& ws);
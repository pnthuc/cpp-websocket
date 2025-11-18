#pragma once
#include "lib.h"

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

struct ProcessInfo {
    DWORD pid;
    DWORD parentPid;
    std::string name;
};

void list_applications(websocket::stream<tcp::socket>& ws);
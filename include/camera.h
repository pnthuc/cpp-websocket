#pragma once
#include "lib.h"
#include "global.h"
using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

// extern std::atomic<bool> g_camera;
// extern std::mutex ws_mutex;
// extern std::thread camera_thread;

void startCamera(std::shared_ptr<websocket::stream<tcp::socket>> ws_ptr);
void stopCamera();

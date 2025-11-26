#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <opencv2/opencv.hpp>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <vector>
#include <string>

#include "base64.h"
#include "global.h"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

// Gửi tin nhắn dạng Text (String) -> Dùng cho main.cpp
void sendMsg(websocket::stream<tcp::socket>& ws,
             const std::string& type,
             const std::string& what,
             const std::string& data);

// Gửi tin nhắn dạng Binary (ảnh, file)
void sendMsg(websocket::stream<tcp::socket>& ws,
             const std::string& type,
             const std::string& what,
             const std::vector<BYTE>& buffer);

// [ĐỔI TÊN] Gửi JSON Object -> Để tránh trùng lặp với String
void sendJsonMsg(websocket::stream<tcp::socket>& ws,
             const std::string& type,
             const std::string& what,
             const json& data);
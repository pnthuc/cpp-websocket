#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <atomic>
#include "base64.h"
#include <ctime>
#include <mutex>
#include <memory>
#include <filesystem>
#include <algorithm>
#include "global.h"
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std;
using nlohmann::json;

void sendMsg(websocket::stream<tcp::socket>& ws,
                    const std::string& type,
                    const std::string& what,
                    const std::string& data);

void sendMsg(websocket::stream<tcp::socket>& ws,
                    const std::string& type,
                    const std::string& what,
                    const std::vector<BYTE>& buffer);
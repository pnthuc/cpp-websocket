#include "lib.h"
#include <ctime>
#include <mutex>

void sendMsg(websocket::stream<tcp::socket>& ws,
             const std::string& type,
             const std::string& what,
             const std::string& data)
{
    std::lock_guard<std::mutex> lock(ws_mutex);
    
    json msg = {
        {"type", type},
        {"what", what},
        {"message", data},
        {"timestamp", std::time(nullptr)}
    };

    std::string payload = msg.dump();
    ws.text(true);
    ws.write(boost::asio::buffer(payload));
}

void sendMsg(websocket::stream<tcp::socket>& ws,
             const std::string& type,
             const std::string& what,
             const std::vector<BYTE>& buffer)
{
    std::lock_guard<std::mutex> lock(ws_mutex);
    
    std::string encoded = base64_encode(buffer.data(), buffer.size());

    json msg = {
        {"type", type},
        {"what", what},
        {"message", encoded},
        {"timestamp", std::time(nullptr)}
    };

    std::string payload = msg.dump();
    ws.text(true);
    ws.write(boost::asio::buffer(payload));
}

// 3. [ĐỔI TÊN] Gửi JSON Object
void sendJsonMsg(websocket::stream<tcp::socket>& ws,
             const std::string& type,
             const std::string& what,
             const json& data)
{
    std::lock_guard<std::mutex> lock(ws_mutex);

    // Dump json data ra string để client dễ parse
    std::string stringData = data.dump();

    json msg = {
        {"type", type},
        {"what", what},
        {"message", stringData}, 
        {"timestamp", std::time(nullptr)}
    };

    std::string payload = msg.dump();
    ws.text(true);
    ws.write(boost::asio::buffer(payload));
}
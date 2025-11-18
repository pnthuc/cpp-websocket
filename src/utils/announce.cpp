#include "announce.h"

const std::string EXTERNAL_SERVER_HOST = "192.168.1.151"; // IP External Server
const std::string EXTERNAL_SERVER_PORT = "5000";      // Port External Server
const int MY_LISTENING_PORT = 8080;                   
const std::string MY_DEVICE_ID = "PC_LAB_01";         // Tên máy này

void announce_once() {
    try {
        net::io_context ioc;
        tcp::resolver resolver{ioc};
        websocket::stream<tcp::socket> ws{ioc};

        std::cout << "[ANNOUNCE] Dang ket noi External Server..." << std::endl;
        auto const results = resolver.resolve(EXTERNAL_SERVER_HOST, EXTERNAL_SERVER_PORT);
        net::connect(ws.next_layer(), results);
        ws.handshake(EXTERNAL_SERVER_HOST, "/");

        // 2. Gửi thông tin báo danh
        json payload = {
            {"type", "ANNOUNCE"},
            {"device_id", MY_DEVICE_ID},
            {"machine_name", "Windows Target"},
            {"port", MY_LISTENING_PORT}
        };

        ws.write(net::buffer(payload.dump()));
        std::cout << "[ANNOUNCE] Da gui thong tin bao danh thanh cong!" << std::endl;
        
        ws.close(websocket::close_code::normal);
    } 
    catch (std::exception& e) {
        std::cerr << "[ANNOUNCE ERROR] Khong the bao danh: " << e.what() << std::endl;
    }
}
#include "announce.h"
const std::string EXTERNAL_SERVER_HOST = "192.168.1.183"; // IP EXTERNAL SERVER, WARN: CHANGE THIS VALUE
const std::string EXTERNAL_SERVER_PORT = "5000";      
const int MY_LISTENING_PORT = 8080;                   
const std::string MY_DEVICE_NAME = getenv("COMPUTERNAME") ? getenv("COMPUTERNAME") : "UnknownDevice";        
const std::string MY_DEVICE_ID = MY_DEVICE_NAME + "_" + std::to_string(MY_LISTENING_PORT);
void announce_once() {
    try {
        net::io_context ioc;
        tcp::resolver resolver{ioc};
        websocket::stream<tcp::socket> ws{ioc};

        file_logger->info("Connecting to External Server...");
        auto const results = resolver.resolve(EXTERNAL_SERVER_HOST, EXTERNAL_SERVER_PORT);
        net::connect(ws.next_layer(), results);
        ws.handshake(EXTERNAL_SERVER_HOST, "/");

        json payload = {
            {"type", "ANNOUNCE"},
            {"device_id", MY_DEVICE_ID},
            {"machine_name", MY_DEVICE_NAME}, 
            {"port", MY_LISTENING_PORT}
        };

        ws.write(net::buffer(payload.dump()));
        file_logger->info("Sent announcement information successfully! Keeping connection alive...");

        beast::flat_buffer buffer;
        for(;;) {
            ws.read(buffer);
            buffer.consume(buffer.size());
        }
    } 
    catch (std::exception& e) {
        file_logger->error("Announce connection lost: {}", e.what());
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
        announce_once(); 
    }
}
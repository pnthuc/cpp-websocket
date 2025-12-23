#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

struct DeviceInfo {
    std::string id;
    std::string ip;
    int port;
    std::string name;
    websocket::stream<tcp::socket>* ws_session; 
};

std::map<std::string, DeviceInfo> connected_devices;
std::mutex devices_mutex;

void sendJson(websocket::stream<tcp::socket>& ws, json data) {
    try {
        ws.text(true);
        ws.write(net::buffer(data.dump()));
    } catch (...) {}
}

void do_session(tcp::socket socket) {
    auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
    std::string current_device_id = "";

    try {
        ws->accept();
        
        std::string remote_ip = ws->next_layer().remote_endpoint().address().to_string();
        for (;;) {
            beast::flat_buffer buffer;
            ws->read(buffer);
            std::string msg = beast::buffers_to_string(buffer.data());
            
            try {
                json data = json::parse(msg);
                std::string type = data["type"];

                if (type == "ANNOUNCE") {
                    std::string device_id = data["device_id"];
                    int port = data["port"];
                    std::string name = data.value("machine_name", "Unknown");

                    std::lock_guard<std::mutex> lock(devices_mutex);
                    connected_devices[device_id] = {device_id, remote_ip, port, name, ws.get()};
                    current_device_id = device_id;

                    std::cout << "[+] New Device: " << name << " (" << remote_ip << ":" << port << ")\n";
                }
                else if (type == "GET_LIST") {
                    json response = {{"type", "DEVICE_LIST"}, {"devices", json::array()}};
                    
                    std::lock_guard<std::mutex> lock(devices_mutex);
                    for (auto it = connected_devices.begin(); it != connected_devices.end(); ) {
                        response["devices"].push_back({
                            {"id", it->second.id},
                            {"ip", it->second.ip},
                            {"port", it->second.port},
                            {"name", it->second.name}
                        });
                        ++it;
                    }
                    sendJson(*ws, response);
                }
            } catch (std::exception& e) {
                std::cerr << "JSON Error: " << e.what() << std::endl;
            }
        }
    } catch (beast::system_error const& se) {
        if (se.code() != websocket::error::closed)
            std::cerr << "Error: " << se.code().message() << std::endl;
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    if (!current_device_id.empty()) {
        std::lock_guard<std::mutex> lock(devices_mutex);
        connected_devices.erase(current_device_id);
        std::cout << "[-] Device Disconnected: " << current_device_id << "\n";
    }
}

int main() {
    try {
        auto const address = net::ip::make_address("0.0.0.0");
        unsigned short port = 5000;

        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {address, port}};
        
        std::cout << "External Server listening on port " << port << "...\n";

        for (;;) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread(do_session, std::move(socket)).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
#include "camera.h"

void startCamera(std::shared_ptr<websocket::stream<tcp::socket>> ws_ptr) {

    std::lock_guard<std::mutex> lock(g_control_mutex);
    
    if (g_camera_active.load()) return;

    g_camera_active = true;
    g_camera_should_stop = false;

    camera_thread = std::thread([ws_ptr]() {

        bool camera_ok = false;
        cv::VideoCapture cap;

        try {
            cap.open(0);
            if (cap.isOpened()) {
                camera_ok = true;
            } else {
                std::cerr << "Cannot open camera\n";
            }

            cv::Mat frame;
            std::vector<BYTE> buf;

            while (!g_camera_should_stop.load()) {
                if (camera_ok){
                    cap >> frame;
                    if (frame.empty()) break;

                    buf.clear();
                    cv::imencode(".jpg", frame, buf);

                    try {
                        // std::lock_guard<std::mutex> lock(ws_mutex);
                        sendMsg(*ws_ptr, "binary", "Camera", buf);
                    } catch (std::exception& e) {
                        std::cerr << "WebSocket write error: " << e.what() << "\n";
                        break;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if (camera_ok) {
                cap.release();
            }
        } catch (std::exception& e) {
            std::cerr << "Camera thread exception: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "Camera thread unknown exception\n";
        }

        g_camera_active = false; 
        std::cout << "Camera thread stopped.\n";
    });
}

void stopCamera() {

    std::lock_guard<std::mutex> lock(g_control_mutex);

    if (!g_camera_active.load())
        return;

    g_camera_should_stop = true;

    std::thread temp_thread = std::move(camera_thread);

    try {
        std::thread([t = std::move(temp_thread)]() mutable {

            try {
                if (t.joinable()) {
                    t.join();
                    std::cout << "Reaper: Camera thread joined." << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Reaper: Exception caught joining camera thread: " << e.what() << std::endl;        
            } catch (...) {
                std::cerr << "Reaper: Unknown exception caught joining camera thread." << std::endl;        
            }
        }).detach();
    } catch(const std::system_error& e) {
        std::cerr << "Failed to spawn reaper thread for camera: " << e.what() << std::endl;
        try {
            if (temp_thread.joinable()) {
                temp_thread.join();
            }
        } catch (...) { /* Bỏ qua lỗi join() lồng */ }
    } catch(...) {
        std::cerr << "Unknown error spawning reaper thread for camera." << std::endl;
        try {
            if (temp_thread.joinable()) {
                temp_thread.join();
            }
        } catch (...) { /* Bỏ qua lỗi join() lồng */ }
    }
}
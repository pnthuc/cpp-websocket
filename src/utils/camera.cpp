#include "camera.h"

void startCamera(std::shared_ptr<websocket::stream<tcp::socket>> ws_ptr) {
    std::lock_guard<std::mutex> lock(g_control_mutex);
    if (g_camera_active.load()) return;

    g_camera_active = true;
    g_camera_should_stop = false;

    camera_thread = std::thread([ws_ptr]() {
        bool camera_ok = false;
        cv::VideoCapture cap;
        const std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 100};

        try {
            cap.open(0);
            if (cap.isOpened())
                camera_ok = true;
            else 
                file_logger->error("Cannot open camera.");

            cv::Mat frame;
            std::vector<BYTE> buf;
            while (!g_camera_should_stop.load()) {
                if (camera_ok){
                    cap >> frame;
                    if (frame.empty()) break;

                    buf.clear();
                    cv::imencode(".jpg", frame, buf, params);

                    try {
                        sendMsg(*ws_ptr, "binary", "Camera", buf);
                    } catch (std::exception& e) {
                        file_logger->error("WebSocket write error: {}", e.what());
                        break;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            if (camera_ok) 
                cap.release();
        } catch (std::exception& e) {
            file_logger->error("Camera thread exception: {}", e.what());
        } catch (...) {
            file_logger->error("Camera thread unknown exception.");
        }

        g_camera_active = false; 
        file_logger->info("Camera thread stopped.");
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
                    file_logger->info("Reaper: Camera thread joined.");
                }
            } catch (const std::exception& e) {
                file_logger->error("Reaper: Exception caught joining camera thread: {}", e.what());
            } catch (...) {
                file_logger->error("Reaper: Unknown exception caught joining camera thread.");
            }
        }).detach();
    } catch(const std::system_error& e) {
        file_logger->error("Failed to spawn reaper thread for camera: {}", e.what());
        try {
            if (temp_thread.joinable()) 
                temp_thread.join();
        } catch (...) {}
    } catch(...) {
        file_logger->error("Unknown error spawning reaper thread for camera.");
        try {
            if (temp_thread.joinable()) 
                temp_thread.join();
        } catch (...) {}
    }
}
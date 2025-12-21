#include "screen.h"
#include <iostream>

bool IsScreenCapturable() {
    HDESK hDesktop = OpenInputDesktop(0, FALSE, DESKTOP_SWITCHDESKTOP);
    if (hDesktop == NULL) return false;
    CloseDesktop(hDesktop);
    return true;
}

cv::Mat captureScreenMat() {
    if (!IsScreenCapturable()) return cv::Mat();

    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);

    int width = desktop.right;
    int height = desktop.bottom;

    if (width == 0 || height == 0) return cv::Mat();

    HDC hScreenDC = GetDC(nullptr);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY | CAPTUREBLT);

    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    cv::Mat mat(height, width, CV_8UC3);
    GetDIBits(hMemoryDC, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(nullptr, hScreenDC);

    return mat;
}

void startScreen(std::shared_ptr<websocket::stream<tcp::socket>> ws_ptr) {
    std::lock_guard<std::mutex> lock(g_control_mutex);
    if (g_screen.load()) return; 
    g_screen = true;

    screen_thread = std::thread([ws_ptr]() {
        file_logger->info("Screen thread started (Realtime Lossless PNG).");
        
        const auto FRAME_INTERVAL = std::chrono::milliseconds(33); 
        
        const std::vector<int> params = {cv::IMWRITE_PNG_COMPRESSION, 1}; 

        try {
            std::vector<BYTE> buf;

            while (g_screen.load()) {
                auto start_time = std::chrono::steady_clock::now();

                if (!IsScreenCapturable()) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue; 
                }

                cv::Mat frame = captureScreenMat();
                if (frame.empty()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                buf.clear();
                
                cv::imencode(".png", frame, buf, params);

                try {
                    sendMsg(*ws_ptr, "binary", "Screen", buf);
                } catch (std::exception& e) {
                    file_logger->error("WebSocket write error: {}", e.what());
                    break;
                }

                auto end_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                if (elapsed < FRAME_INTERVAL) {
                    std::this_thread::sleep_for(FRAME_INTERVAL - elapsed);
                }
            }
        } catch (std::exception& e) {
            file_logger->error("Screen exception: {}", e.what());
        }

        g_screen = false;
    });
}

void stopScreen() {
    std::lock_guard<std::mutex> lock(g_control_mutex);
    if (!g_screen.load()) 
        return;

    g_screen = false;
    std::thread temp_thread = std::move(screen_thread); 

    std::thread([t = std::move(temp_thread)]() mutable { 
        try {
            if (t.joinable()) {
                t.join(); 
                file_logger->info("Reaper: Screen thread joined.");
            }
        } catch (const std::exception& e) {
            file_logger->error("Reaper: Exception caught joining screen thread: {}", e.what());
        } catch (...) {
            file_logger->error("Reaper: Unknown exception caught joining screen thread.");
        }
    }).detach(); 
}
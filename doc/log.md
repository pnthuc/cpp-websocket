**[Người dùng]:** Tôi được thầy giao đồ án môn Mạng Máy Tính và đề bài như sau:

> "Viết chương trình ứng dụng mạng mô hình Client-Server dùng giao thức WebSocket bằng ngôn ngữ C++, mục đích là điều khiển và giám sát các máy tính trong cùng một mạng LAN. Các chức năng bắt buộc gồm:
> 
> 1. Liệt kê, Khởi chạy và Dừng các Ứng dụng (Applications).
>     
> 2. Liệt kê và Dừng các Tiến trình (Processes).
>     
> 3. Chụp ảnh màn hình (Screenshot) gửi về máy điều khiển.
>     
> 4. Keylogger (ghi lại phím nhấn trên máy bị điều khiển).
>     
> 5. Xem Webcam (video stream ngắn).
>     
> 6. Shutdown/Reset máy từ xa."
>     

Tôi nắm chắc kiến thức C++ cơ bản và lập trình hướng đối tượng, nhưng chưa bao giờ làm việc với Lập trình mạng (Network Programming) hay WebSocket cả. Bạn hãy đóng vai trò là một mentor, hướng dẫn tôi từng bước từ con số 0. Tôi cần một định hướng rõ ràng trước khi bắt tay vào code.

**[AI]:** Chào bạn, đừng quá lo lắng. Đề tài này tuy thách thức nhưng là cơ hội tuyệt vời để bạn hiểu sâu về cách hệ thống hoạt động. Bản chất đây là một phần mềm RAT (Remote Administration Tool) thu nhỏ.

Với yêu cầu của thầy, chúng ta cần xác định rõ kiến trúc hệ thống ngay từ đầu để tránh phải đập đi xây lại sau này. Mình đề xuất mô hình **Client-Server 3 lớp** (nhưng trong phạm vi đồ án này ta có thể gộp lại thành 2 phần chính):

1. **Agent (Server - Máy bị điều khiển):** Phần này **bắt buộc phải viết bằng C++** như đề bài yêu cầu.
    
    - _Tại sao C++?_ Vì chỉ có C++ (hoặc C) mới đủ mạnh và "gần phần cứng" để thực hiện các tác vụ can thiệp sâu như: Hook bàn phím (Keylogger), gọi Windows API để tắt Process, chụp màn hình tốc độ cao và xử lý luồng video từ Webcam mà không gây lag máy.
        
    - Nó sẽ đóng vai trò là WebSocket Server (hoặc Client tùy mô hình kết nối), chạy ngầm và chờ lệnh.
        
2. **Dashboard (Client - Máy Admin):** Phần này dùng để gửi lệnh và xem kết quả.
    
    - Viết một ứng dụng viết bằng **HTML/JavaScript** chạy trên trình duyệt web
        
Bạn thấy hướng đi này có ổn không?

**[Người dùng]:** Hướng đi này ổn. Vấn đề bây giờ là: Thư viện WebSocket nào cho C++ là tốt nhất. 
Tôi có google thử "C++ WebSocket library" thì nó ra rất nhiều kết quả: uWebSockets, websocketpp, Boost.Beast, Poco, QtNetwork... Cái nào cũng có ưu và nhược điểm. Hãy gợi ý cho tôi, tôi cần một thư viện đáp ứng:

1. Ổn định (đừng crash lung tung).
    
2. Đủ tài liệu để tra cứu (vì tôi mới học).
    
3. Cài đặt trên Windows không quá phức tạp (mình dùng Visual Studio/MinGW).
    
**[AI]:** Đây là câu hỏi "triệu đô" khi bắt đầu dự án C++ mạng. Hãy cùng mổ xẻ các ứng cử viên sáng giá nhất nhé:

1. **`uWebSockets`**:
    
    - _Ưu điểm:_ Hiệu năng "khủng khiếp", được coi là nhanh nhất hiện nay.
        
    - _Nhược điểm:_ Code thuần C++ khá khó đọc với người mới. Việc tích hợp vào Windows đôi khi gặp rắc rối về dependencies (thư viện phụ thuộc). Tài liệu hơi kiệm lời.
        
2. **`websocketpp`**:
    
    - _Ưu điểm:_ Rất phổ biến vài năm trước, chỉ cần include header là chạy (header-only), không cần build phức tạp.
        
    - _Nhược điểm:_ Dựa trên thư viện Boost.Asio phiên bản cũ. Dự án này hiện tại ít được cập nhật bảo trì.
        
3. **`Boost.Beast`**:
    
    - _Ưu điểm:_ Đây là "ngôi sao sáng" hiện nay. Nó nằm chính thức trong bộ thư viện **Boost** (được ví là thư viện chuẩn mở rộng của C++). Nó sử dụng `Boost.Asio` hiện đại, hỗ trợ mô hình **Asynchronous (Bất đồng bộ)** cực mạnh.
        
    - _Tại sao cần Async?_ Hãy tưởng tượng máy Agent đang stream video webcam liên tục. Nếu dùng model đồng bộ (Synchronous), chương trình sẽ bị "đơ" khi đang gửi video, không thể nhận lệnh "Stop Webcam" từ bạn. Với Async của Boost.Beast, nó có thể vừa gửi video, vừa lắng nghe lệnh Stop cùng lúc mượt mà.
        
    - _Nhược điểm:_ Cú pháp hơi dài dòng, dùng nhiều template, mới nhìn sẽ hơi "sốc".
        
**Lời khuyên của mình:** Hãy chọn **Boost.Beast**. Tuy đường vào hơi dốc, nhưng một khi đã nắm được bạn sẽ có một nền tảng tư duy lập trình mạng rất vững chắc. Hơn nữa, ghi "Thành thạo Boost C++" vào CV xin việc là một điểm cộng cực lớn.

**[Người dùng]:** Tôi vừa thử tải Boost về và include thử thư viện trên Visual Studio Code nhưng nó báo lỗi include, thiếu file thư viện .lib. Giúp tôi cài đặt lại Boost(tôi dùng compiler MINGW 15.2.0) và phác thảo cấu trúc thư mục dự án (Project Structure) và cấu hình giúp tôi file CMakeList.txt để link các thư viện.

**[AI]:** Chào mừng bạn đến với "địa ngục dependency" của C++ trên Windows! Đừng lo, mình có "thuốc giải" đây. Đừng bao giờ cài thủ công copy-paste file lib nếu không muốn phát điên. Hãy dùng **vcpkg** - trình quản lý gói xịn xò của Microsoft.

**Quy trình cài đặt chuẩn chỉ:**

1. Tải và cài đặt vcpkg (clone từ Github về).
    
2. Mở CMD/PowerShell, chạy lệnh thần thánh này:
    
    vcpkg install boost-beast boost-thread opencv4 nlohmann-json spdlog --triplet=x64-mingw-dynamic
    
    - (Mình thêm luôn `opencv4` cho webcam, `nlohmann-json` để xử lý dữ liệu JSON, và `spdlog` để ghi log cho sướng).
        
3. Pha một tách cà phê và đợi. Nó sẽ tự tải source về và build everything cho bạn.
    

Về cấu trúc dự án:

Mình đề xuất mô hình tách biệt mã nguồn và header để dễ quản lý khi dự án phình to:

```
SystemMonitor/
├── CMakeLists.txt       # File cấu hình build tự động
├── build/...
├── src/                 # Chứa mã nguồn (.cpp)
│   ├── main.cpp         # Điểm khởi chạy, chứa vòng lặp chính
│   └── utils/         # Các chức năng (Webcam, App, Process...)
│       ├── camera.cpp
│       ├── list_proc.cpp
│       └── ...
├── include/             # Chứa file header (.h)
│   ├── screen.h
│   ├── camera.h
│   └── ...
└── web/                 # Chứa giao diện Frontend
    ├── index.html
    └── script.js
```

Mình sẽ soạn cho bạn file `CMakeLists.txt` để nó tự động tìm các thư viện đã cài bằng vcpkg và link vào dự án.

**[Người dùng]:** 

Giờ tôi muốn test xem mọi thứ có hoạt động không. Bạn viết giúp tôi một chương trình C++ "Hello World" với WebSocket đơn giản nhất có thể.

- **Yêu cầu:** Mở cổng 8080.
    
- Khi có ai kết nối vào (ví dụ từ trình duyệt), in ra Console: "New Client Connected".
    
- Gửi lại cho Client câu: "Hello from C++ Agent".
    
- Sau đó đóng kết nối cũng được.
    
    Tôi cần đảm bảo thư viện đã được link đúng trước khi viết chức năng phức tạp.
    
**[AI]:** Chính xác, luôn bắt đầu nhỏ. Đây là code `main.cpp` tối giản sử dụng Boost.Beast để tạo một **Echo Server** cơ bản:

[AI cung cấp đoạn code C++ mẫu]

Giải thích nhanh:

- `net::io_context ioc`: Bộ não quản lý các tác vụ I/O.
    
- `tcp::acceptor`: Chuyên đứng nghe ở cổng 8080.
    
- `ws.accept()`: Thực hiện bắt tay (Handshake) WebSocket để nâng cấp từ HTTP lên WebSocket.
    
- `ws.write(...)`: Gửi dữ liệu đi.
    

Bạn tạo file `main.cpp`, copy code này vào, sau đó dùng CMake để build. Nếu thấy console hiện dòng "Server listening on 8080" là thành công bước đầu!

**[Người dùng]:** Build thành công, Server đã chạy và không bị crash.

Nhưng làm sao để kiểm tra kết nối, gõ ws://localhost:8080 lên Edge thì nó báo lỗi không truy cập được.

Bạn giúp tôi viết nhanh một file index.html đơn giản làm Client test với. Có 1 nút "Connect", 1 ô nhập text và 1 nút "Send".

**[AI]:**  Đúng rồi, trình duyệt không cho gõ trực tiếp giao thức ws://. Bạn cần một trang web làm Client.

Đây là file index.html dùng HTML5 thuần (không cần framework gì cả):

[AI cung cấp mã nguồn HTML/JS]

Cơ chế hoạt động:

- `const ws = new WebSocket('ws://localhost:8080');` -> Dòng này sẽ mở kết nối tới Server C++ của bạn.
    
- `ws.onopen`: Khi kết nối thành công, nó sẽ báo "Connected".
    
- `ws.onmessage`: Khi C++ gửi "Hello from C++ Agent", hàm này sẽ bắt được và in ra màn hình web.
    
Bạn lưu file này lại, mở trực tiếp bằng trình duyệt (double click). Bấm nút "Connect" và quan sát cả 2 màn hình (Web và Console C++) nhé.

**[Người dùng]:** Bây giờ tập trung làm 2 chức năng quản lý hệ thống: Process (Tiến trình) và Application (Ứng dụng).

Bắt đầu với Process trước. Tôi muốn lấy danh sách tất cả process đang chạy (PID, Tên, RAM sử dụng). Windows có lệnh nào hỗ trợ cái này không hay phải dùng `system("tasklist")` rồi parse chuỗi? Tôi muốn dùng API native cho nhanh.

**[AI]:** Dùng `system("tasklist")` rất chậm và thiếu chuyên nghiệp (lại còn bị lộ cửa sổ đen CMD). Chúng ta sẽ dùng API **Windows ToolHelp32**.

- Thư viện: `<tlhelp32.h>`, `<psapi.h>`.
    
- Hàm chính: `CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)`. Hàm này giống như chụp một bức ảnh nhanh (snapshot) toàn bộ bảng tiến trình hệ thống tại thời điểm gọi.
    
- Sau đó dùng vòng lặp `Process32First` và `Process32Next` để duyệt qua từng thằng.
    
- Để lấy RAM usage, ta dùng hàm `GetProcessMemoryInfo`.
    
Mình sẽ viết cho bạn file `list_proc.cpp` bao gồm hàm `list_processes()` trả về chuỗi JSON để dễ gửi qua WebSocket nhé.

**[Người dùng]:** Giờ đến phần Start/Stop Process và List Applications.

Stop thì dùng TerminateProcess. Nhưng còn List Applications thì tôi dựa vào List Process nhưng nó chỉ hiển thị các Application đang chạy. Thầy yêu cầu hiện cả những phần mềm đã cài (ví dụ Chrome, Word...) để có thể Start nó lên từ xa.

**[AI]:**  Đúng vậy, Process là "động" (đang chạy), còn Application là "tĩnh" (nằm trên ổ cứng). Để lấy danh sách phần mềm đã cài, nơi uy tín nhất là Windows Registry.

Hầu hết phần mềm khi cài đặt sẽ ghi danh vào khóa:

`HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall`

(Và cả `HKEY_CURRENT_USER` nữa).

Chúng ta sẽ viết hàm `ScanRegistry` trong `list_app.cpp`:

1. Dùng `RegOpenKeyEx` để mở khóa Uninstall.
    
2. Dùng `RegEnumKeyEx` để duyệt qua từng phần mềm.
    
3. Đọc các giá trị: `DisplayName` (Tên), `DisplayIcon` (Icon/File thực thi), `InstallLocation`.
    
4. Lưu ý: Registry chứa rất nhiều rác (Driver, Update patch...), sau này ta sẽ phải lọc bớt.
    

**[Người dùng]:** Giờ đến Keylogger.

Tôi thấy trên mạng có hướng dẫn dùng hàm `GetAsyncKeyState` trong vòng lặp `while(true)`. Cách này có ổn không? Tôi nghĩ nó sẽ tốn CPU.

**[AI]:** Cách `GetAsyncKeyState` là cách "nông dân" và rất tệ. Nó bắt bạn phải polling liên tục (vòng lặp vô tận), gây nóng máy và dễ bị sót phím nếu gõ nhanh.

Cách chuyên nghiệp (và cũng là cách các phần mềm gián điệp hay dùng) là Windows Hooks.

- Hàm: `SetWindowsHookEx(WH_KEYBOARD_LL, ...)`
    
- Cơ chế: Ta đăng ký với hệ điều hành: "Này Windows, mỗi khi có ai nhấn phím, hãy gọi hàm của tôi trước khi gửi phím đó cho ứng dụng khác".
    
- Ưu điểm: Không tốn CPU (chỉ chạy khi có phím nhấn), bắt được phím toàn cục (Global) kể cả khi ứng dụng C++ đang ẩn.
    
    Mình sẽ viết module `keylogger.cpp` theo cơ chế Hook này. Dữ liệu phím sẽ được gửi realtime về Web socket thay vì ghi file text (để Admin thấy ngay lập tức).
    

**[Người dùng]:** Còn **Shutdown/Reset** chỉ cần gọi lệnh `system` là xong phải không.

**[AI]:** Chính xác.

- Shutdown: `system("shutdown /s /t 0 /f")` (/f để buộc đóng các app đang treo).
    
- Restart: system("shutdown /r /t 0 /f").
    
    Mình đã gói nó vào file header shutdown.h cho gọn code. Tuần này tiến độ rất tốt!
    

**[Người dùng]:** Bây giờ đến `Webcam`, tôi nghĩ phải dùng thư viện `thread` của C++ để chia ra nhiều luồng công việc bởi vì nếu tôi mở nhiều tab(nhiều client) thì mỗi session phải chạy trên một luồng riêng hoặc trong lúc tôi thực hiện thuật toán quay `Webcam` thì đoạn lệnh này phải được thực hiện song song với luồng công việc chính.

Tiếp theo là làm sao để truyền video từ C++ sang Web qua `WebSocket`. Tôi biết `WebSocket` chỉ gửi được `text` hoặc `binary`, không `stream` được kiểu `RTP/RTSP`.

**[AI]:** Tư duy của bạn rất đúng. Chúng ta phải sử dụng thư viện `thread` để chạy một lúc nhiều công việc khác nhau, tôi sẽ cập nhật vào code. Đúng vậy chúng ta không stream video theo chuẩn phức tạp. Chúng ta sẽ làm kiểu `"Stop Motion"` tốc độ cao.

Chiến thuật MJPEG qua WebSocket:

1. **Backend (C++):**
    
    - Dùng thư viện **OpenCV** (`cv::VideoCapture`) để chụp liên tục các khung hình (frames) từ Webcam.
        
    - Mỗi frame là một ảnh to (`cv::Mat`). Ta nén nó lại thành định dạng `.jpg` (để giảm dung lượng từ vài MB xuống vài chục KB).
        
    - Mã hóa ảnh JPG sang chuỗi văn bản **Base64**.
        
    - Gửi chuỗi này qua WebSocket.
        
2. **Frontend (JS):**
    
    - Nhận chuỗi Base64.
        
    - Gán vào thuộc tính `src` của thẻ `<img>`: `img.src = "data:image/jpeg;base64," + data;`.
        
    - Trình duyệt sẽ tự render ảnh mới thay thế ảnh cũ -> Tạo thành hiệu ứng video.
        

**[Người dùng]:** Tôi đã làm theo hướng dẫn. Nó chạy được rồi.

Tuy nhiên nó khá lag và hơi delay. Hình ảnh cứ giật giật. Task Manager báo C++ ăn CPU khá nhiều.

**[AI]:** Đây là vấn đề kinh điển. Hãy làm một bài toán nhỏ:

- Ảnh HD (720p) nén JPG ~ 100KB.
    
- 30 FPS -> 3MB/s ~ 24Mbps upload liên tục.
    
- Cộng thêm chi phí mã hóa `Base64` (làm tăng 33% dung lượng) -> Gần 30-40Mbps.
    
- Việc mã hóa Base64 và gửi liên tục trên 1 luồng (`Thread`) chính đang làm nghẽn socket.
    
Giải pháp tối ưu: Thuật toán `"Smart Diff"` (So sánh thông minh).

Tại sao phải gửi cả bức ảnh khi bạn chỉ chớp mắt?

1. Lưu lại frame vừa gửi (`lastFrame`).
    
2. Lấy frame hiện tại (`currentFrame`).
    
3. Dùng `cv::absdiff` để tính sự khác biệt giữa 2 frame.
    
4. Tính trung bình độ sai lệch màu sắc. Nếu nhỏ hơn một ngưỡng (Threshold) -> Coi như ảnh tĩnh -> **Không gửi gì cả**.
    
5. Ngoài ra, giảm chất lượng nén JPEG (`cv::IMWRITE_JPEG_QUALITY`) xuống mức 70-80.
    
**[Người dùng]:** Áp dụng luôn cho chức năng `Screenshot`, còn về phần `Screen` thì bạn làm như quay `Webcam`, `Screenshot` liên tục và gửi qua . Nhưng `OpenCV` chụp màn hình `Desktop` có vẻ không hỗ trợ tốt.

**[AI]:** Đúng rồi, `cv::VideoCapture(0)` chỉ bắt webcam. Để chụp màn hình Desktop trên Windows, ta phải dùng **GDI API** (Graphics Device Interface).

- Quy trình: Lấy Handle của màn hình (`GetDC(NULL)`) -> Tạo `CompatibleBitmap` -> Dùng hàm `BitBlt` để copy pixel từ màn hình vào Bitmap -> Chuyển Bitmap sang `cv::Mat` của OpenCV -> Nén JPG -> Gửi đi.
    
- Code `screen.cpp` mình cung cấp sẽ kết hợp GDI (để chụp nhanh) và OpenCV (để nén ảnh & Smart Diff).
    
**[Người dùng]:** Tôi đang mở rộng ra điều khiển chuột/phím từ xa (biến nó thành TeamViewer) và Quản lý file (File Explorer).

**[AI]:** Hoàn toàn khả thi và cực kỳ ấn tượng!

1. **Remote Desktop Control:**
    
    - **Web (JS):** Bắt sự kiện `mousemove`, `mousedown`, `keydown` trên thẻ `<img>` hiển thị màn hình. Tính toán tọa độ tỉ lệ (vì màn hình Admin có thể nhỏ hơn màn hình Agent). Gửi tọa độ (x, y) về Server.
        
    - **C++ (Agent):** Nhận gói tin JSON `{x: 500, y: 300, click: left}`. Dùng hàm `SendInput` của Windows để di chuyển con trỏ chuột thật và click.
        
2. **File Explorer:**
    
    - Dùng thư viện `std::filesystem` (C++17) để duyệt cây thư mục.
        
    - Gửi danh sách file (Tên, Size, Loại) về Web để render thành giao diện như Windows Explorer.
        
    - **Download:** Mở file dạng Binary -> Đọc vào buffer -> Base64 -> Gửi về -> Web tạo `Blob` để tải xuống.
        

**[Người dùng]:** Tôi đang gặp một vấn đề thực tế. Hiện tại mình phải hardcode IP của máy Agent vào Web (ws://192.168.1.10:8080). Nếu mình mang máy Agent sang phòng khác, IP đổi thành .15 là Web tịt ngòi.

Làm sao để Web tự biết IP của Agent?

**[AI]:** Bạn đang chạm đến vấn đề Service Discovery.

Để giải quyết triệt để (như TeamViewer), ta cần một Server thứ 3 đứng giữa gọi là Signaling Server (Máy chủ tín hiệu).

- **Signaling Server (`external_server.cpp`):** Chạy trên một máy có IP cố định (hoặc chính là máy Admin). Nó lắng nghe ở cổng 5000.
    
- **Agent:** Khi khởi động, việc đầu tiên là kết nối đến Server 5000 và hét lên: "Tôi là máy PC-Kế-Toán, IP của tôi là X, tôi đang lắng nghe ở cổng 8080".
    
- Web Admin: Kết nối đến Server 5000 hỏi: "Có ai đang online không?". Server trả về danh sách. Admin chọn một máy và kết nối trực tiếp (P2P) tới IP đó.
    
    Mình sẽ giúp bạn viết thêm module `external_server.cpp` và hàm `announce.cpp` cho Agent.
    
**[Người dùng]:** Giờ tôi muốn thêm cái Terminal (CMD) của server ngay trên Web.

**[AI]:** Ok, ta sẽ dùng hàm `_popen` (Pipe Open).

- Nó sẽ mở một tiến trình con `cmd.exe`.
    
- Ta ghi lệnh vào đường ống đầu vào (stdin) và đọc kết quả từ đầu ra (stdout).
    
- Kết quả đọc được gửi thẳng về Web.
    
- Module `terminal.cpp` đã sẵn sàng.
    

**[Người dùng]:** VIết lại UI của web cho đẹp, hiện đại, background xanh biển, tông màu tối, bổ sung thêm `systeminfo` của server hiện thị ngay trên web và một nút `Điều khiển` bấm vào sẽ hiện thỉ cửa sổ .

**[AI]:** Đồng ý. Chúng ta sẽ dùng **TailwindCSS** để style nhanh. Mình sẽ thiết kế theo phong cách **Glassmorphism** (kính mờ), tông màu tối (Dark Mode/Cyberpunk) cho đúng chất "Hacker".

- Thêm các biểu đồ (Chart) hiển thị CPU/RAM real-time (vẽ bằng CSS hoặc Canvas).
    
- Menu bên trái để chuyển đổi giữa các tab: Screen, Camera, Files, Terminal...
    
Tôi cũng đã bổ sung các chức năng bạn yêu cầu.

**[Người dùng]:** Chức năng List App hiện ra một đống rác: `uninstall.exe`, `helper.exe`, `icon.ico`... Bấm nút "`Start App`" thì nó chỉ mở thư mục lên chứ không chạy phần mềm.

**[AI]:** Nguyên nhân do `Registry` lưu đường dẫn không đồng nhất. Có cái trỏ vào file exe, có cái trỏ vào thư mục cài đặt, có cái chứa cả tham số dòng lệnh.

Giải pháp nâng cấp (`list_app.cpp` v2):

1. **Smart Filter:** Tạo Blacklist loại bỏ các tên file chứa "`uninstall`", "`setup`", "`update`".
    
2. **Logic "Săn tìm":** Nếu đường dẫn là thư mục, code sẽ tự động quét bên trong thư mục đó. Nó sẽ tìm file `.exe` nào có dung lượng lớn nhất và tên gần giống tên App nhất -> Chọn đó là file chạy chính.
    
3. **Clean Path:** Viết hàm để cắt bỏ các tham số thừa (ví dụ `chrome.exe --no-sandbox` -> `chrome.exe`).
    
**[Người dùng]:** Build lại lần cuối để nộp. Lại lỗi `ld.exe: cannot open output file main.exe: Permission denied`. 

**[AI]:** File thực thi cũ của bạn có thể đang chạy ngầm (do lần trước tắt không sạch hoặc bị crash nhưng process chưa chết). Windows khóa file đang chạy nên trình biên dịch không thể ghi đè file mới.

- **Fix:** Mở Task Manager, tìm file đó và End Task. Hoặc chạy lệnh `taskkill /F /IM main.exe` trong CMD.
    
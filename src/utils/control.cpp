#include "control.h"
void MoveMouse(double x, double y) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    // Windows dùng toạ độ 0-65535 cho toàn màn hình
    input.mi.dx = (LONG)(x * 65535.0);
    input.mi.dy = (LONG)(y * 65535.0);
    SendInput(1, &input, sizeof(INPUT));
}

void ClickMouse(int btn, int action) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    
    if (btn == 0) { // Left
        input.mi.dwFlags = (action == 0) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    } else if (btn == 1) { // Right
        input.mi.dwFlags = (action == 0) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    } else if (btn == 2) { // Middle
        input.mi.dwFlags = (action == 0) ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    }
    
    SendInput(1, &input, sizeof(INPUT));
}

void PressKey(int key, int action) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = (WORD)key;
    input.ki.dwFlags = (action == 1) ? KEYEVENTF_KEYUP : 0;
    
    SendInput(1, &input, sizeof(INPUT));
}
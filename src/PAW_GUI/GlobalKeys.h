#ifndef GLOBALKEYS_H
#define GLOBALKEYS_H

#include <windows.h>
#include <functional>

#define VK_MEDIA_NEXT_TRACK 0xB0
#define VK_MEDIA_PREV_TRACK 0xB1
#define VK_MEDIA_STOP       0xB2
#define VK_MEDIA_PLAY_PAUSE 0xB3

class GlobalKeys {
public:
    static GlobalKeys& instance() {
        static GlobalKeys instance;
        return instance;
    }

    void setCallback(std::function<void(int)> cb) {
        callback = cb;
    }

    void install() {
        if (!hook) {
            hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookProc, GetModuleHandle(NULL), 0);
        }
    }

    void uninstall() {
        if (hook) {
            UnhookWindowsHookEx(hook);
            hook = NULL;
        }
    }

private:
    HHOOK hook = NULL;
    std::function<void(int)> callback;

    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION) {
            KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                int key = p->vkCode;
                if (key == VK_MEDIA_NEXT_TRACK ||
                    key == VK_MEDIA_PREV_TRACK ||
                    key == VK_MEDIA_STOP ||
                    key == VK_MEDIA_PLAY_PAUSE)
                {
                    if (instance().callback) {
                        instance().callback(key);
                        return 1; 
                    }
                }
            }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
};

#endif
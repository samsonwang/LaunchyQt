
#include "QHotKeyP.h"
#include <Winuser.h>

QHotkeyPrivate::NativeKeyMap QHotkeyPrivate::s_keyMap[] = {
    {Qt::Key_Escape,      VK_ESCAPE},
    {Qt::Key_Tab,         VK_TAB},
    {Qt::Key_Backtab,     0},
    {Qt::Key_Backspace,   VK_BACK},
    {Qt::Key_Return,      VK_RETURN},
    {Qt::Key_Enter,       VK_RETURN},
    {Qt::Key_Insert,      VK_INSERT},
    {Qt::Key_Delete,      VK_DELETE},
    {Qt::Key_Pause,       VK_PAUSE},
    {Qt::Key_Print,       VK_SNAPSHOT},
    {Qt::Key_SysReq,      0},
    {Qt::Key_Clear,       VK_CLEAR},
    {Qt::Key_Home,        VK_HOME},
    {Qt::Key_End,         VK_END},
    {Qt::Key_Left,        VK_LEFT},
    {Qt::Key_Up,          VK_UP},
    {Qt::Key_Right,       VK_RIGHT},
    {Qt::Key_Down,        VK_DOWN},
    {Qt::Key_PageUp,      VK_PRIOR},
    {Qt::Key_PageDown,    VK_NEXT},
    {Qt::Key_Shift,       VK_SHIFT},
    {Qt::Key_Control,     VK_CONTROL},
    {Qt::Key_Meta,        VK_LWIN},
    {Qt::Key_Alt,         VK_MENU},
    {Qt::Key_CapsLock,    VK_CAPITAL},
    {Qt::Key_NumLock,     VK_NUMLOCK},
    {Qt::Key_ScrollLock,  VK_SCROLL},

    {Qt::Key_F1,          VK_F1},
    {Qt::Key_F2,          VK_F2},
    {Qt::Key_F3,          VK_F3},
    {Qt::Key_F4,          VK_F4},
    {Qt::Key_F5,          VK_F5},
    {Qt::Key_F6,          VK_F6},
    {Qt::Key_F7,          VK_F7},
    {Qt::Key_F8,          VK_F8},
    {Qt::Key_F9,          VK_F9},
    {Qt::Key_F10,         VK_F10},
    {Qt::Key_F11,         VK_F11},
    {Qt::Key_F12,         VK_F12},
    {Qt::Key_F13,         VK_F13},
    {Qt::Key_F14,         VK_F14},
    {Qt::Key_F15,         VK_F15},
    {Qt::Key_F16,         VK_F16},
    {Qt::Key_F17,         VK_F17},
    {Qt::Key_F18,         VK_F18},
    {Qt::Key_F19,         VK_F19},
    {Qt::Key_F20,         VK_F20},
    {Qt::Key_F21,         VK_F21},
    {Qt::Key_F22,         VK_F22},
    {Qt::Key_F23,         VK_F23},
    {Qt::Key_F24,         VK_F24},
    {Qt::Key_F25,         0},
    {Qt::Key_F26,         0},
    {Qt::Key_F27,         0},
    {Qt::Key_F28,         0},
    {Qt::Key_F29,         0},
    {Qt::Key_F30,         0},
    {Qt::Key_F31,         0},
    {Qt::Key_F32,         0},
    {Qt::Key_F33,         0},
    {Qt::Key_F34,         0},
    {Qt::Key_F35,         0},

    {Qt::Key_Super_L,     0},
    {Qt::Key_Super_R,     0},
    {Qt::Key_Menu,        0},
    {Qt::Key_Hyper_L,     0},
    {Qt::Key_Hyper_R,     0},
    {Qt::Key_Help,        0},
    {Qt::Key_Direction_L, 0},
    {Qt::Key_Direction_R, 0},
    {Qt::Key_Space,       VK_SPACE},

    {Qt::Key_0,           0x30},
    {Qt::Key_1,           0x31},
    {Qt::Key_2,           0x32},
    {Qt::Key_3,           0x33},
    {Qt::Key_4,           0x34},
    {Qt::Key_5,           0x35},
    {Qt::Key_6,           0x36},
    {Qt::Key_7,           0x37},
    {Qt::Key_8,           0x38},
    {Qt::Key_9,           0x39},

    {Qt::Key_A,           0x41},
    {Qt::Key_B,           0x42},
    {Qt::Key_C,           0x43},
    {Qt::Key_D,           0x44},
    {Qt::Key_E,           0x45},
    {Qt::Key_F,           0x46},
    {Qt::Key_G,           0x47},
    {Qt::Key_H,           0x48},
    {Qt::Key_I,           0x49},
    {Qt::Key_J,           0x4a},
    {Qt::Key_K,           0x4b},
    {Qt::Key_L,           0x4c},
    {Qt::Key_M,           0x4d},
    {Qt::Key_N,           0x4e},
    {Qt::Key_O,           0x4f},
    {Qt::Key_P,           0x50},
    {Qt::Key_Q,           0x51},
    {Qt::Key_R,           0x52},
    {Qt::Key_S,           0x53},
    {Qt::Key_T,           0x54},
    {Qt::Key_U,           0x55},
    {Qt::Key_V,           0x56},
    {Qt::Key_W,           0x57},
    {Qt::Key_X,           0x58},
    {Qt::Key_Y,           0x59},
    {Qt::Key_Z,           0x5a},

    {Qt::Key_QuoteLeft,   VK_OEM_8},
    {Qt::Key_Minus,       VK_OEM_MINUS},
    {Qt::Key_Equal,       VK_OEM_PLUS},

    {Qt::Key_BracketLeft, VK_OEM_4},
    {Qt::Key_BracketRight,VK_OEM_6},

    {Qt::Key_Semicolon,   VK_OEM_1},
    {Qt::Key_Apostrophe,  VK_OEM_3},
    {Qt::Key_NumberSign,  VK_OEM_7},

    {Qt::Key_Backslash,   VK_OEM_5},
    {Qt::Key_Comma,	      VK_OEM_COMMA},
    {Qt::Key_Period,      VK_OEM_PERIOD},
    {Qt::Key_Slash,       VK_OEM_2},

    {Qt::Key_unknown,     0},
};

QHotkeyPrivate::NativeModMap QHotkeyPrivate::s_modMap[] = {
    {Qt::ShiftModifier,        MOD_SHIFT},
    {Qt::ControlModifier,      MOD_CONTROL},
    {Qt::AltModifier,          MOD_ALT},
    {Qt::MetaModifier,         MOD_WIN},

    {Qt::KeyboardModifierMask, 0}
};

static HHOOK s_keyboardHook = NULL;
static UINT s_mod = NULL;
static UINT s_key = NULL;

LRESULT CALLBACK KeyboardHookProc(INT nCode, WPARAM wParam, LPARAM lParam) {
    // By returning a non-zero value from the hook procedure, the
    // message does not get passed to the target window
    if (HC_ACTION == nCode) {
        KBDLLHOOKSTRUCT* event = (KBDLLHOOKSTRUCT*)lParam;
        if ((event->flags & LLKHF_UP) == 0 && event->vkCode == s_key) {
            SHORT sHighBit = SHORT(1 << (sizeof(SHORT)*8 - 1));
            bool bCtrl = ((s_mod & MOD_CONTROL) != 0) == ((GetAsyncKeyState(VK_CONTROL) & sHighBit) != 0);
            bool bShift = ((s_mod & MOD_SHIFT) != 0) == ((GetAsyncKeyState(VK_SHIFT) & sHighBit) != 0);
            bool bAlt = ((s_mod & MOD_ALT) != 0) == ((GetAsyncKeyState(VK_MENU) & sHighBit) != 0);
            bool bLWin = ((s_mod & MOD_WIN) != 0) == ((GetAsyncKeyState(VK_LWIN) & sHighBit) != 0);
            bool bRWin = ((s_mod & MOD_WIN) != 0) == ((GetAsyncKeyState(VK_RWIN) & sHighBit) != 0);

            qDebug() << "KeyboardHookProc, s_mod state:"
                << bCtrl << bShift << bAlt << bLWin << bRWin;

            if (bCtrl && bShift && bAlt && bLWin && bRWin) {
                PostMessage(NULL, WM_USER, s_key | s_mod, 0);
                qDebug() << "KeyboardHookProc, send WM_USER";
                return 1;
            }
        }
    }
    return CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
}

int QHotkeyPrivate::calcHotkeyId(quint32 key, quint32 mod) {
    return key | mod;
}

bool QHotkeyPrivate::EventFilter::nativeEventFilter(const QByteArray& eventType,
                                                    void* message,
                                                    long* result) {
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    MSG* msg = static_cast<MSG*>(message);
    if (msg->message == WM_HOTKEY || msg->message == WM_USER) {
        int id = static_cast<int>(msg->wParam);
        activateHotKey(id);
        return true;
    }
    return false;
}

bool QHotkeyPrivate::registerKey(quint32 key, quint32 mod, int keyId) {
    qDebug() << "QHotkeyPrivate::registerKey,"
        << "keyid:" << keyId << "mod:" << mod << "key:" << key;

    s_mod = mod;
    s_key = key;

    switch (key) {
    case VK_CAPITAL:
    case VK_SCROLL:
        if (!s_keyboardHook) {
            // Turn off capslock or scroll lock if they're on and we're not already
            // hooked. Nobody wants capslock turned on permanently do they?
            if (GetKeyState(VK_CAPITAL) == 1) {
                keybd_event(VK_CAPITAL, 0, 0, 0);
                keybd_event(VK_CAPITAL, 0, KEYEVENTF_KEYUP, 0);
            }
        }
    case VK_NUMLOCK:
        if (!s_keyboardHook) {
            s_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, NULL, NULL);
        }
        return true;
        break;

    default:
        if (s_keyboardHook) {
            UnhookWindowsHookEx(s_keyboardHook);
            s_keyboardHook = NULL;
            s_mod = NULL;
            s_key = NULL;
        }
        break;
    }

    return RegisterHotKey(NULL, keyId, mod, key) == TRUE;
}

void QHotkeyPrivate::unregisterKey(quint32 key, quint32 mod, int keyId) {
    Q_UNUSED(key);
    Q_UNUSED(mod);

    if (s_keyboardHook) {
        UnhookWindowsHookEx(s_keyboardHook);
        s_keyboardHook = NULL;
        s_mod = NULL;
        s_key = NULL;
    }

    UnregisterHotKey(NULL, keyId);
}

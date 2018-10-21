
#include "QHotkeyP.h"
#include <QX11Info>
#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

QHotkeyPrivate::NativeKeyMap QHotkeyPrivate::s_keyMap[] = {
    {Qt::Key_Escape,      XK_Escape},
    {Qt::Key_Tab,         XK_Tab},
    {Qt::Key_Backtab,     XK_ISO_Left_Tab},
    {Qt::Key_Backspace,   XK_BackSpace},
    {Qt::Key_Return,      XK_Return},
    {Qt::Key_Enter,       XK_KP_Enter},
    {Qt::Key_Insert,      XK_Insert},
    {Qt::Key_Delete,      XK_Delete},
    {Qt::Key_Pause,       XK_Pause},
    {Qt::Key_Print,       XK_Print},
    {Qt::Key_SysReq,      XK_Sys_Req},
    {Qt::Key_Clear,       XK_KP_Begin},
    {Qt::Key_Home,        XK_Home},
    {Qt::Key_End,         XK_End},
    {Qt::Key_Left,        XK_Left},
    {Qt::Key_Up,          XK_Up},
    {Qt::Key_Right,       XK_Right},
    {Qt::Key_Down,        XK_Down},
    {Qt::Key_PageUp,      XK_Prior},
    {Qt::Key_PageDown,    XK_Next},
    {Qt::Key_Shift,       XK_Shift_L},
    {Qt::Key_Control,     XK_Control_L},
    {Qt::Key_Meta,        XK_Meta_L},
    {Qt::Key_Alt,         XK_Alt_L},
    {Qt::Key_CapsLock,    XK_Caps_Lock},
    {Qt::Key_NumLock,     XK_Num_Lock},
    {Qt::Key_ScrollLock,  XK_Scroll_Lock},
    {Qt::Key_Space,       XK_space},
    {Qt::Key_Equal,       XK_equal},
    {Qt::Key_Asterisk,    XK_asterisk},

    {Qt::Key_Plus,        XK_plus},
    {Qt::Key_Comma,       XK_comma},
    {Qt::Key_Minus,       XK_minus},
    {Qt::Key_Period,      XK_period},
    {Qt::Key_Slash,       XK_slash},

    {Qt::Key_F1,          XK_F1},
    {Qt::Key_F2,          XK_F2},
    {Qt::Key_F3,          XK_F3},
    {Qt::Key_F4,          XK_F4},
    {Qt::Key_F5,          XK_F5},
    {Qt::Key_F6,          XK_F6},
    {Qt::Key_F7,          XK_F7},
    {Qt::Key_F8,          XK_F8},
    {Qt::Key_F9,          XK_F9},
    {Qt::Key_F10,         XK_F10},
    {Qt::Key_F11,         XK_F11},
    {Qt::Key_F12,         XK_F12},
    {Qt::Key_F13,         XK_F13},
    {Qt::Key_F14,         XK_F14},
    {Qt::Key_F15,         XK_F15},
    {Qt::Key_F16,         XK_F16},
    {Qt::Key_F17,         XK_F17},
    {Qt::Key_F18,         XK_F18},
    {Qt::Key_F19,         XK_F19},
    {Qt::Key_F20,         XK_F20},
    {Qt::Key_F21,         XK_F21},
    {Qt::Key_F22,         XK_F22},
    {Qt::Key_F23,         XK_F23},
    {Qt::Key_F24,         XK_F24},
    {Qt::Key_F25,         XK_F25},
    {Qt::Key_F26,         XK_F26},
    {Qt::Key_F27,         XK_F27},
    {Qt::Key_F28,         XK_F28},
    {Qt::Key_F29,         XK_F29},
    {Qt::Key_F30,         XK_F30},
    {Qt::Key_F31,         XK_F31},
    {Qt::Key_F32,         XK_F32},
    {Qt::Key_F33,         XK_F33},
    {Qt::Key_F34,         XK_F34},
    {Qt::Key_F35,         XK_F35},
    {Qt::Key_Super_L,     XK_Super_L},
    {Qt::Key_Super_R,     XK_Super_R},
    {Qt::Key_Menu,        XK_Menu},
    {Qt::Key_Hyper_L,     XK_Hyper_L},
    {Qt::Key_Hyper_R,     XK_Hyper_R},
    {Qt::Key_Help,        XK_Help},
    {Qt::Key_Direction_L, 0},
    {Qt::Key_Direction_R, 0},

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

    {Qt::Key_unknown,     0},
};

QHotkeyPrivate::NativeModMap QHotkeyPrivate::s_modMap[] = {
    {Qt::ShiftModifier,        ShiftMask},
    {Qt::ControlModifier,      ControlMask},
    {Qt::AltModifier,          Mod1Mask},
    {Qt::MetaModifier,         Mod4Mask},

    {Qt::KeyboardModifierMask, 0}
};

int QHotkeyPrivate::calcHotkeyId(quint32 key, quint32 mod) {
    return key | (mod << 16);
}

bool QHotkeyPrivate::EventFilter::nativeEventFilter(const QByteArray &eventType,
                                                    void *message,
                                                    long *result) {
    Q_UNUSED(eventType)
    Q_UNUSED(result)

    xcb_generic_event_t* e = static_cast<xcb_generic_event_t*>(message);
    if ((e->response_type & ~0x80) == XCB_KEY_PRESS) {
        xcb_key_press_event_t* ke = (xcb_key_press_event_t*)e;
        //xcb_get_keyboard_mapping_reply_t rep;
        //sxcb_keysym_t* k = xcb_get_keyboard_mapping_keysyms(&rep);
        quint32 keycode = ke->detail;
        quint32 mods = ke->state & (ShiftMask|ControlMask|Mod1Mask|Mod3Mask);
        return activateHotKey(calcHotkeyId(keycode, mods));
    }
    return false;
}

bool QHotkeyPrivate::registerKey(quint32 key, quint32 mod, int keyId) {
    Q_UNUSED(keyId)
    xcb_grab_key(QX11Info::connection(), 1, QX11Info::appRootWindow(),
                 mod, key, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    return true;
}

void QHotkeyPrivate::unregisterKey(quint32 key, quint32 mod, int keyId) {
    Q_UNUSED(keyId)
    xcb_ungrab_key(QX11Info::connection(), key,
                   QX11Info::appRootWindow(), mod);
}

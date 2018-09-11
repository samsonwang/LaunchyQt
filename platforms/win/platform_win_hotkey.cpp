/*
 * globalshortcutmanager_win.cpp - Windows implementation of global shortcuts
 * Copyright (C) 2003-2006  Justin Karneges, Maciej Niedzielski
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin, Simon Capewell

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#include "precompiled.h"
#include "platform_base_hotkey.h"
#include "platform_Base_hottrigger.h"


HHOOK keyboardHook;
HWND widgetWinId;
UINT mod, key;


HINSTANCE GetHInstance()
{    
    MEMORY_BASIC_INFORMATION mbi;
    TCHAR szModule[MAX_PATH];

    SetLastError(ERROR_SUCCESS);
    if (VirtualQuery(GetHInstance,&mbi,sizeof(mbi)))
    {
        if (GetModuleFileName((HINSTANCE)mbi.AllocationBase, szModule, sizeof(szModule)))
        {
            return (HINSTANCE)mbi.AllocationBase;
        }
    }
    return NULL;
}


LRESULT CALLBACK KeyboardHookProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
    // By returning a non-zero value from the hook procedure, the
    // message does not get passed to the target window
    switch (nCode)
    {
        case HC_ACTION:
        {
		    KBDLLHOOKSTRUCT* event = (KBDLLHOOKSTRUCT*)lParam;
			if (widgetWinId && (event->flags & LLKHF_UP) == 0 && event->vkCode == key)
			{
				if (
					(((mod & MOD_CONTROL) != 0) == (GetAsyncKeyState(VK_CONTROL) >> ((sizeof(SHORT) * 8) - 1))) &&
					(((mod & MOD_SHIFT) != 0) == (GetAsyncKeyState(VK_SHIFT) >> ((sizeof(SHORT) * 8) - 1))) &&
					(((mod & MOD_ALT) != 0) == (GetAsyncKeyState(VK_MENU) >> ((sizeof(SHORT) * 8) - 1))) &&
					(((mod & MOD_WIN) != 0) == (GetAsyncKeyState(VK_LWIN) >> ((sizeof(SHORT) * 8) - 1))) &&
					(((mod & MOD_WIN) != 0) == (GetAsyncKeyState(VK_RWIN) >> ((sizeof(SHORT) * 8) - 1)))
					)
				{
					PostMessage(widgetWinId, WM_USER, 0, 0);
					return 1;
				}
			}
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}


class GlobalShortcutManager::KeyTrigger::Impl : public QWidget
{
public:
	
	bool connected;
        /**
         * Constructor registers the hotkey.
         */
        Impl(GlobalShortcutManager::KeyTrigger* t, const QKeySequence& ks)
                : trigger_(t)
                , id_(0)
				, connected(false)
        {
			widgetWinId = winId();

			if (convertKeySequence(ks, &mod, &key))
			{
				switch (key)
				{
				case VK_CAPITAL:
				case VK_SCROLL:
					if (!keyboardHook)
					{
						// Turn off capslock or scroll lock if they're on and we're not already 
						// hooked. Nobody wants capslock turned on permanently do they?
						if (GetKeyState(VK_CAPITAL) == 1)
						{
							keybd_event(VK_CAPITAL, 0, 0, 0 );
							keybd_event(VK_CAPITAL, 0, KEYEVENTF_KEYUP, 0 );
						}
					}
				case VK_NUMLOCK:
					if (!keyboardHook)
						keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, GetHInstance(), 0);
					if (keyboardHook)
						connected = true;
					break;
				default:
					if (RegisterHotKey(winId(), nextId, mod, key))
					{
						id_ = nextId++;
						connected = true;
					}
					break;
				}
			}
        }

        /**
         * Destructor unregisters the hotkey.
         */
        ~Impl()
        {
			widgetWinId = NULL;
			if (keyboardHook)
			{
				UnhookWindowsHookEx(keyboardHook);
				keyboardHook = NULL;
				connected = false;
			}
			else if (id_) {
				UnregisterHotKey(winId(), id_);
				connected = false;
			}
        }

        /**
         * Triggers activated() signal when the hotkey is activated.
         */
        bool winEvent(MSG* m, long* result)
		{
			if ((m->message == WM_HOTKEY && m->wParam == id_) || m->message == WM_USER) {
				emit trigger_->activated();
				return true;
			}
			return QWidget::winEvent(m, result);
		}

private:
        KeyTrigger* trigger_;
        int id_;
        static int nextId;

private:
        struct Qt_VK_Keymap
        {
            int key;
            UINT vk;
        };
        static Qt_VK_Keymap qt_vk_table[];

        static bool convertKeySequence(const QKeySequence& ks, UINT* mod_, UINT* key_)
        {
                int code = ks;

				// JK: I had to put the code -='s here and comment out code &= 0xffff 
				// to correctly identify the action key

                UINT mod = 0;
				if (code & Qt::META) {
                        mod |= MOD_WIN;
						code -= Qt::META;
				}
				if (code & Qt::SHIFT) {
                        mod |= MOD_SHIFT;
						code -= Qt::SHIFT;
				}
				if (code & Qt::CTRL) {
                        mod |= MOD_CONTROL;
						code -= Qt::CTRL;
				}
				if (code & Qt::ALT) {
                        mod |= MOD_ALT;
						code -= Qt::ALT;
				}

                UINT key = 0;
//                code &= 0xffff;
				// Some keys map to ASCII keycodes
                if (code == 0x20 ||
					(code >= 0x30 && code <= 0x39) ||
					(code > 0x40 && code <= 0x5a) ||
					(code > 0x60 && code <= 0x7a))
                        key = code;
                else {
					// Others require lookup from a keymap
                        for (int n = 0; qt_vk_table[n].key != Qt::Key_unknown; ++n) {
                                if (qt_vk_table[n].key == code) {
                                        key = qt_vk_table[n].vk;
                                        break;
                                }
                        }
                        if (!key)
                                return false;
                }

                *mod_ = mod;
                if (key)
                        *key_ = key;

                return true;
        }
};

GlobalShortcutManager::KeyTrigger::Impl::Qt_VK_Keymap
GlobalShortcutManager::KeyTrigger::Impl::qt_vk_table[] = {
        { Qt::Key_Escape,      VK_ESCAPE },
        { Qt::Key_Tab,         VK_TAB },
        { Qt::Key_Backtab,     0 },
        { Qt::Key_Backspace,   VK_BACK },
        { Qt::Key_Return,      VK_RETURN },
        { Qt::Key_Enter,       VK_RETURN },
        { Qt::Key_Insert,      VK_INSERT },
        { Qt::Key_Delete,      VK_DELETE },
        { Qt::Key_Pause,       VK_PAUSE },
        { Qt::Key_Print,       VK_SNAPSHOT },
        { Qt::Key_SysReq,      0 },
        { Qt::Key_Clear,       VK_CLEAR },
        { Qt::Key_Home,        VK_HOME },
        { Qt::Key_End,         VK_END },
        { Qt::Key_Left,        VK_LEFT },
        { Qt::Key_Up,          VK_UP },
        { Qt::Key_Right,       VK_RIGHT },
        { Qt::Key_Down,        VK_DOWN },
        { Qt::Key_PageUp,      VK_PRIOR },
        { Qt::Key_PageDown,    VK_NEXT },
        { Qt::Key_Shift,       VK_SHIFT },
        { Qt::Key_Control,     VK_CONTROL },
        { Qt::Key_Meta,        VK_LWIN },
        { Qt::Key_Alt,         VK_MENU },
        { Qt::Key_CapsLock,    VK_CAPITAL },
        { Qt::Key_NumLock,     VK_NUMLOCK },
        { Qt::Key_ScrollLock,  VK_SCROLL },
        { Qt::Key_F1,          VK_F1 },
        { Qt::Key_F2,          VK_F2 },
        { Qt::Key_F3,          VK_F3 },
        { Qt::Key_F4,          VK_F4 },
        { Qt::Key_F5,          VK_F5 },
        { Qt::Key_F6,          VK_F6 },
        { Qt::Key_F7,          VK_F7 },
        { Qt::Key_F8,          VK_F8 },
        { Qt::Key_F9,          VK_F9 },
        { Qt::Key_F10,         VK_F10 },
        { Qt::Key_F11,         VK_F11 },
        { Qt::Key_F12,         VK_F12 },
        { Qt::Key_F13,         VK_F13 },
        { Qt::Key_F14,         VK_F14 },
        { Qt::Key_F15,         VK_F15 },
        { Qt::Key_F16,         VK_F16 },
        { Qt::Key_F17,         VK_F17 },
        { Qt::Key_F18,         VK_F18 },
        { Qt::Key_F19,         VK_F19 },
        { Qt::Key_F20,         VK_F20 },
        { Qt::Key_F21,         VK_F21 },
        { Qt::Key_F22,         VK_F22 },
        { Qt::Key_F23,         VK_F23 },
        { Qt::Key_F24,         VK_F24 },
        { Qt::Key_F25,         0 },
        { Qt::Key_F26,         0 },
        { Qt::Key_F27,         0 },
        { Qt::Key_F28,         0 },
        { Qt::Key_F29,         0 },
        { Qt::Key_F30,         0 },
        { Qt::Key_F31,         0 },
        { Qt::Key_F32,         0 },
        { Qt::Key_F33,         0 },
        { Qt::Key_F34,         0 },
        { Qt::Key_F35,         0 },
        { Qt::Key_Super_L,     0 },
        { Qt::Key_Super_R,     0 },
        { Qt::Key_Menu,        0 },
        { Qt::Key_Hyper_L,     0 },
        { Qt::Key_Hyper_R,     0 },
        { Qt::Key_Help,        0 },
        { Qt::Key_Direction_L, 0 },
        { Qt::Key_Direction_R, 0 },

		{ Qt::Key_QuoteLeft,   VK_OEM_8 },
		{ Qt::Key_Minus,       VK_OEM_MINUS },
		{ Qt::Key_Equal,       VK_OEM_PLUS },

		{ Qt::Key_BracketLeft, VK_OEM_4 },
		{ Qt::Key_BracketRight,VK_OEM_6 },

		{ Qt::Key_Semicolon,   VK_OEM_1 },
		{ Qt::Key_Apostrophe,  VK_OEM_3 },
		{ Qt::Key_NumberSign,  VK_OEM_7 },

		{ Qt::Key_Backslash,   VK_OEM_5 },
		{ Qt::Key_Comma,	   VK_OEM_COMMA },
		{ Qt::Key_Period,      VK_OEM_PERIOD },
		{ Qt::Key_Slash,       VK_OEM_2 },

        { Qt::Key_unknown,     0 },
};

int GlobalShortcutManager::KeyTrigger::Impl::nextId = 1;

GlobalShortcutManager::KeyTrigger::KeyTrigger(const QKeySequence& key)
{
        d.reset(new Impl(this, key));
}

GlobalShortcutManager::KeyTrigger::~KeyTrigger()
{
	/*
	delete d;
        d = 0;
		*/
}

bool GlobalShortcutManager::KeyTrigger::isConnected() 
{
	if (!d) return false;
	return d->connected;
}
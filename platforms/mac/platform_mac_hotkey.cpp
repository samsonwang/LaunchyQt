/*
 * globalshortcutmanager_mac.cpp - Mac OS X implementation of global shortcuts
 * Copyright (C) 2003-2007  Eric Smith, Michail Pishchagin
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
#include "platform_mac_hotkey.h"

MacKeyTriggerManager* MacKeyTriggerManager::instance_   = NULL;
EventHandlerUPP MacKeyTriggerManager::hot_key_function_ = NULL;
MacKeyTriggerManager::Ascii2KeyCodeTable MacKeyTriggerManager::key_codes_;

class GlobalShortcutManager::KeyTrigger::Impl : public MacKeyTrigger
{
public:
		static bool failed;
private:
	KeyTrigger* trigger_;
	EventHotKeyRef hotKey_;
	int id_;
	static int nextId;

public:
	/**
	 * Constructor registers the hotkey.
	 */
	Impl(GlobalShortcutManager::KeyTrigger* t, const QKeySequence& ks)
		: trigger_(t)
		, id_(0)
	{
		MacKeyTriggerManager::instance()->addTrigger(this);

		quint32 key, mod;
		if (MacKeyTriggerManager::convertKeySequence(ks, &key, &mod)) {
			EventHotKeyID hotKeyID;
			hotKeyID.signature = 'QtHK';
			hotKeyID.id = nextId;

			OSStatus ret = RegisterEventHotKey(key, mod, hotKeyID, GetApplicationEventTarget(), 0, &hotKey_);
			if (ret != 0) {
					failed = true;
				qWarning("RegisterEventHotKey(%d, %d): %d", key, mod, (int)ret);
				return;
			}
			failed = false;
			id_ = nextId++;
		}
	}

	/**
	 * Destructor unregisters the hotkey.
	 */
	~Impl()
	{
		MacKeyTriggerManager::instance()->removeTrigger(this);

		if (id_)
			UnregisterEventHotKey(hotKey_);
	}

	void activate()
	{
		emit trigger_->activated();
	}

	bool isAccepted(int id) const
	{
		return id_ == id;
	}
};

/*
 * The following table is from Apple sample-code.
 * Apple's headers don't appear to define any constants for the virtual key
 * codes of special keys, but these constants are somewhat documented in the chart at
 * <http://developer.apple.com/techpubs/mac/Text/Text-571.html#MARKER-9-18>
 *
 * The constants on the chartappear to be the same values as are used in Apple's iGetKeys
 * sample.
 * <http://developer.apple.com/samplecode/Sample_Code/Human_Interface_Toolbox/Mac_OS_High_Level_Toolbox/iGetKeys.htm>.
 *
 * See also <http://www.mactech.com/articles/mactech/Vol.04/04.12/Macinkeys/>.
 */
bool GlobalShortcutManager::KeyTrigger::Impl::failed;
MacKeyTriggerManager::Qt_Mac_Keymap
MacKeyTriggerManager::qt_keymap[] = {
	{ Qt::Key_Escape,      0x35 },
	{ Qt::Key_Tab,         0x30 },
	{ Qt::Key_Backtab,     0 },
	{ Qt::Key_Backspace,   0x33 },
	{ Qt::Key_Return,      0x24 },
	{ Qt::Key_Enter,       0x4c }, // Return & Enter are different on the Mac
	{ Qt::Key_Insert,      0 },
	{ Qt::Key_Delete,      0x75 },
	{ Qt::Key_Pause,       0 },
	{ Qt::Key_Print,       0 },
	{ Qt::Key_SysReq,      0 },
	{ Qt::Key_Clear,       0x47 },
	{ Qt::Key_Home,        0x73 },
	{ Qt::Key_End,         0x77 },
	{ Qt::Key_Left,        0x7b },
	{ Qt::Key_Up,          0x7e },
	{ Qt::Key_Right,       0x7c },
	{ Qt::Key_Down,        0x7d },
	{ Qt::Key_PageUp,      0x74 }, // Page Up
	{ Qt::Key_PageDown,    0x79 }, // Page Down
	{ Qt::Key_Shift,       0x38 },
	{ Qt::Key_Control,     0x3b },
	{ Qt::Key_Meta,        0x37 }, // Command
	{ Qt::Key_Alt,         0x3a }, // Option
	{ Qt::Key_CapsLock,    57 },
	{ Qt::Key_NumLock,     0 },
	{ Qt::Key_ScrollLock,  0 },
	{ Qt::Key_F1,          0x7a },
	{ Qt::Key_F2,          0x78 },
	{ Qt::Key_F3,          0x63 },
	{ Qt::Key_F4,          0x76 },
	{ Qt::Key_F5,          0x60 },
	{ Qt::Key_F6,          0x61 },
	{ Qt::Key_F7,          0x62 },
	{ Qt::Key_F8,          0x64 },
	{ Qt::Key_F9,          0x65 },
	{ Qt::Key_F10,         0x6d },
	{ Qt::Key_F11,         0x67 },
	{ Qt::Key_F12,         0x6f },
	{ Qt::Key_F13,         0x69 },
	{ Qt::Key_F14,         0x6b },
	{ Qt::Key_F15,         0x71 },
	{ Qt::Key_F16,         0 },
	{ Qt::Key_F17,         0 },
	{ Qt::Key_F18,         0 },
	{ Qt::Key_F19,         0 },
	{ Qt::Key_F20,         0 },
	{ Qt::Key_F21,         0 },
	{ Qt::Key_F22,         0 },
	{ Qt::Key_F23,         0 },
	{ Qt::Key_F24,         0 },
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
	{ Qt::Key_Help,        0x72 },
	{ Qt::Key_Direction_L, 0 },
	{ Qt::Key_Direction_R, 0 },

	{ Qt::Key_unknown,     0 }
};

int GlobalShortcutManager::KeyTrigger::Impl::nextId = 1;

GlobalShortcutManager::KeyTrigger::KeyTrigger(const QKeySequence& key)
{
        d.reset(new Impl(this, key));
}

GlobalShortcutManager::KeyTrigger::~KeyTrigger()
{
    d.reset();
}


bool GlobalShortcutManager::KeyTrigger::isConnected()
{
  	if (!d) return false;	
		return !GlobalShortcutManager::KeyTrigger::Impl::failed;
}

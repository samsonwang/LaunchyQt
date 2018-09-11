#pragma once

#include "platform_base_hotkey.h"
#include "platform_base_hottrigger.h"

// TODO:
//  - don't invoke hotkey if there is a modal dialog?
//  - do multi-mapping, like the x11 version

#include <QCoreApplication>

#include <Carbon/Carbon.h>

class MacKeyTrigger
{
public:
	virtual ~MacKeyTrigger() {}
	virtual void activate() = 0;
	virtual bool isAccepted(int id) const = 0;
};

class MacKeyTriggerManager : public QObject
{
public:
	static MacKeyTriggerManager* instance()
	{
		if(!instance_)
			instance_ = new MacKeyTriggerManager();
		return instance_;
	}

	void addTrigger(MacKeyTrigger* trigger)
	{
		triggers_ << trigger;
	}

	void removeTrigger(MacKeyTrigger* trigger)
	{
		triggers_.removeAll(trigger);
	}

private:
	MacKeyTriggerManager()
		: QObject(QCoreApplication::instance())
	{
		initAscii2KeyCodeTable(&key_codes_);
		hot_key_function_ = NewEventHandlerUPP(hotKeyHandler);
		EventTypeSpec type;
		type.eventClass = kEventClassKeyboard;
		type.eventKind = kEventHotKeyPressed;
		InstallApplicationEventHandler(hot_key_function_, 1, &type, this, NULL);
	}

	/**
	 * Callback function invoked when the user hits a hot-key.
	 */
	static pascal OSStatus hotKeyHandler(EventHandlerCallRef /*nextHandler*/, EventRef theEvent, void* userData)
	{
		EventHotKeyID hkID;
		GetEventParameter(theEvent, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof(EventHotKeyID), NULL, &hkID);
		static_cast<MacKeyTriggerManager*>(userData)->activated(hkID.id);
		return noErr;
	}

	void activated(int id)
	{
		foreach(MacKeyTrigger* trigger, triggers_) {
			if (trigger->isAccepted(id)) {
				trigger->activate();
				break;
			}
		}
	}

	static MacKeyTriggerManager* instance_;
	QList<MacKeyTrigger*> triggers_;

	typedef struct
	{
		short kchrID;
		Str255 KCHRname;
		short transtable[256];
	} Ascii2KeyCodeTable;

	enum {
		kTableCountOffset = 256 + 2,
		kFirstTableOffset = 256 + 4,
		kTableSize        = 128
	};

	static EventHandlerUPP hot_key_function_;
	static Ascii2KeyCodeTable key_codes_;

private:
	/**
	 * initAscii2KeyCodeTable initializes the ascii to key code
	 * look up table using the currently active KCHR resource. This
	 * routine calls GetResource so it cannot be called at interrupt time.
	 */
	static OSStatus initAscii2KeyCodeTable(Ascii2KeyCodeTable* ttable)
	{
		unsigned char* theCurrentKCHR, *ithKeyTable;
		short count, i, j, resID;
		Handle theKCHRRsrc;
		ResType rType;

		// set up our table to all minus ones
		for (i = 0; i < 256; i++)
			ttable->transtable[i] = -1;

		// find the current kchr resource ID
		ttable->kchrID = (short)GetScriptVariable(smCurrentScript, smScriptKeys);

		// get the current KCHR resource
		theKCHRRsrc = GetResource('KCHR', ttable->kchrID);
		if (theKCHRRsrc == NULL)
			return resNotFound;
		GetResInfo(theKCHRRsrc, &resID, &rType, ttable->KCHRname);

		// dereference the resource
		theCurrentKCHR = (unsigned char *)(*theKCHRRsrc);

		// get the count from the resource
		count = *(short*)(theCurrentKCHR + kTableCountOffset);

		// build inverse table by merging all key tables
		for (i = 0; i < count; i++) {
			ithKeyTable = theCurrentKCHR + kFirstTableOffset + (i * kTableSize);
			for (j = 0; j < kTableSize; j++) {
				if (ttable->transtable[ ithKeyTable[j] ] == -1)
					ttable->transtable[ ithKeyTable[j] ] = j;
			}
		}

		return noErr;
	}

	/**
	 * validateAscii2KeyCodeTable verifies that the ascii to key code
	 * lookup table is synchronized with the current KCHR resource. If
	 * it is not synchronized, then the table is re-built. This routine calls
	 * GetResource so it cannot be called at interrupt time.
	 *
	 * Should probably call this at some point, in case the user has switched keyboard
	 * layouts while we were running.
	 */
	static OSStatus validateAscii2KeyCodeTable(Ascii2KeyCodeTable* ttable, Boolean* wasChanged)
	{
		short theID;
		theID = (short) GetScriptVariable(smCurrentScript, smScriptKeys);
		if (theID != ttable->kchrID) {
			*wasChanged = true;
			return initAscii2KeyCodeTable(ttable);
		}
		else {
			*wasChanged = false;
			return noErr;
		}
	}

	/**
	 * asciiToKeyCode looks up the ascii character in the key
	 * code look up table and returns the virtual key code for that
	 * letter. If there is no virtual key code for that letter, then
	 * the value -1 will be returned.
	 */
	static short asciiToKeyCode(Ascii2KeyCodeTable* ttable, short asciiCode)
	{
		if (asciiCode >= 0 && asciiCode <= 255)
			return ttable->transtable[asciiCode];
		else return false;
	}

	/**
	 * Not used.
	 */
	static char keyCodeToAscii(short virtualKeyCode)
	{
		unsigned long state;
		long keyTrans;
		char charCode;
		Ptr kchr;
		state = 0;
		kchr = (Ptr)GetScriptVariable(smCurrentScript, smKCHRCache);
		keyTrans = KeyTranslate(kchr, virtualKeyCode, &state);
		charCode = keyTrans;
		if (!charCode)
			charCode = (keyTrans >> 16);
		return charCode;
	}

private:
	struct Qt_Mac_Keymap
	{
		int qt_key;
		int mac_key;
	};

	static Qt_Mac_Keymap qt_keymap[];

public:
	static bool convertKeySequence(const QKeySequence& ks, quint32* _key, quint32* _mod)
	{
		int code = ks[0];

		quint32 mod = 0;
		if (code & Qt::META)
			mod |= controlKey;
		if (code & Qt::SHIFT)
			mod |= shiftKey;
		if (code & Qt::CTRL)
			mod |= cmdKey;
		if (code & Qt::ALT)
			mod |= optionKey;

		code &= ~Qt::KeyboardModifierMask;
		quint32 key = 0;
		for (int n = 0; qt_keymap[n].qt_key != Qt::Key_unknown; ++n) {
			if (qt_keymap[n].qt_key == code) {
				key = qt_keymap[n].mac_key;
				break;
			}
		}
		if (key == 0) {
			key = asciiToKeyCode(&key_codes_, code & 0xffff);
		}

		if (_mod)
			*_mod = mod;
		if (_key)
			*_key = key;

		return true;
	}
};

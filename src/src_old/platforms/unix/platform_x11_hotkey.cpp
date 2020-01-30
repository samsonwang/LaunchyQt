/*
 * globalshortcutmanager_x11.cpp - X11 implementation of global shortcuts
 * Copyright (C) 2003-2007  Justin Karneges, Michail Pishchagin
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

#include "platform_x11_hotkey.h"

X11KeyTriggerManager* X11KeyTriggerManager::instance_;

class GlobalShortcutManager::KeyTrigger::Impl : public X11KeyTrigger
{
public:
        static bool failed;
private:
        KeyTrigger* trigger_;
        int qkey_;

        struct GrabbedKey {
                int code;
                uint mod;
        };
        QList<GrabbedKey> grabbedKeys_;


        static int XGrabErrorHandler(Display *, XErrorEvent *)
        {
                qWarning("failed to grab key");
                failed = true;
                return 0;
        }

        void bind(int keysym, unsigned int mod)
        {
                int code = XKeysymToKeycode(QX11Info::display(), keysym);

                // don't grab keys with empty code (because it means just the modifier key)
                if (keysym && !code)
                        return;

                failed = false;
                XErrorHandler savedErrorHandler = XSetErrorHandler(XGrabErrorHandler);

		WId w = QX11Info::appRootWindow();
		//		qDebug() << "X11 hotkey says root is:" << w;
		foreach(long mask_mod, X11KeyTriggerManager::ignModifiersList()) {
                        XGrabKey(QX11Info::display(), code, mod | mask_mod, w, False, GrabModeAsync, GrabModeAsync);
                        GrabbedKey grabbedKey;
                        grabbedKey.code = code;
                        grabbedKey.mod  = mod | mask_mod;
                        grabbedKeys_ << grabbedKey;
                }
                XSync(QX11Info::display(), False);
                XSetErrorHandler(savedErrorHandler);
        }

public:
        /**
         * Constructor registers the hotkey.
         */
        Impl(GlobalShortcutManager::KeyTrigger* t, const QKeySequence& ks)
                : trigger_(t)
                , qkey_(ks)
        {
                X11KeyTriggerManager::instance()->addTrigger(this);

                X11KeyTriggerManager::Qt_XK_Keygroup kg;
                unsigned int mod;
                if (X11KeyTriggerManager::convertKeySequence(ks, &mod, &kg))
                        for (int n = 0; n < kg.num; ++n)
                                bind(kg.sym[n], mod);
        }

        /**
         * Destructor unregisters the hotkey.
         */
        ~Impl()
        {
                X11KeyTriggerManager::instance()->removeTrigger(this);
		//	XUngrabKey(QX11Info::display(),AnyKey,AnyModifier,QX11Info::appRootWindow());
		foreach(GrabbedKey key, grabbedKeys_) 
		    XUngrabKey(QX11Info::display(), key.code, key.mod, QX11Info::appRootWindow());
		
		
        }

        void activate()
        {
                emit trigger_->activated();
        }

        bool isAccepted(int qkey) const
        {
                return qkey_ == qkey;
        }
};

bool GlobalShortcutManager::KeyTrigger::Impl::failed;
long X11KeyTriggerManager::alt_mask  = 0;
long X11KeyTriggerManager::meta_mask = 0;
long X11KeyTriggerManager::super_mask  = 0;
long X11KeyTriggerManager::hyper_mask  = 0;
long X11KeyTriggerManager::numlock_mask  = 0;
bool X11KeyTriggerManager::haveMods  = false;

X11KeyTriggerManager::Qt_XK_Keymap
X11KeyTriggerManager::qt_xk_table[] = {
        { Qt::Key_Escape,      {1, { XK_Escape }}},
        { Qt::Key_Tab,         {2, { XK_Tab, XK_KP_Tab }}},
        { Qt::Key_Backtab,     {1, { XK_ISO_Left_Tab }}},
        { Qt::Key_Backspace,   {1, { XK_BackSpace }}},
        { Qt::Key_Return,      {1, { XK_Return }}},
        { Qt::Key_Enter,       {1, { XK_KP_Enter }}},
        { Qt::Key_Insert,      {2, { XK_Insert, XK_KP_Insert }}},
        { Qt::Key_Delete,      {3, { XK_Delete, XK_KP_Delete, XK_Clear }}},
        { Qt::Key_Pause,       {1, { XK_Pause }}},
        { Qt::Key_Print,       {1, { XK_Print }}},
        { Qt::Key_SysReq,      {1, { XK_Sys_Req }}},
        { Qt::Key_Clear,       {1, { XK_KP_Begin }}},
        { Qt::Key_Home,        {2, { XK_Home, XK_KP_Home }}},
        { Qt::Key_End,         {2, { XK_End, XK_KP_End }}},
        { Qt::Key_Left,        {2, { XK_Left, XK_KP_Left }}},
        { Qt::Key_Up,          {2, { XK_Up, XK_KP_Up }}},
        { Qt::Key_Right,       {2, { XK_Right, XK_KP_Right }}},
        { Qt::Key_Down,        {2, { XK_Down, XK_KP_Down }}},
        { Qt::Key_PageUp,      {2, { XK_Prior, XK_KP_Prior }}},
        { Qt::Key_PageDown,    {2, { XK_Next, XK_KP_Next }}},
        { Qt::Key_Shift,       {3, { XK_Shift_L, XK_Shift_R, XK_Shift_Lock  }}},
        { Qt::Key_Control,     {2, { XK_Control_L, XK_Control_R }}},
        { Qt::Key_Meta,        {2, { XK_Meta_L, XK_Meta_R }}},
        { Qt::Key_Alt,         {2, { XK_Alt_L, XK_Alt_R }}},
        { Qt::Key_CapsLock,    {1, { XK_Caps_Lock }}},
        { Qt::Key_NumLock,     {1, { XK_Num_Lock }}},
        { Qt::Key_ScrollLock,  {1, { XK_Scroll_Lock }}},
        { Qt::Key_Space,       {2, { XK_space, XK_KP_Space }}},
        { Qt::Key_Equal,       {2, { XK_equal, XK_KP_Equal }}},
        { Qt::Key_Asterisk,    {2, { XK_asterisk, XK_KP_Multiply }}},
        { Qt::Key_Plus,        {2, { XK_plus, XK_KP_Add }}},
        { Qt::Key_Comma,       {2, { XK_comma, XK_KP_Separator }}},
        { Qt::Key_Minus,       {2, { XK_minus, XK_KP_Subtract }}},
        { Qt::Key_Period,      {2, { XK_period, XK_KP_Decimal }}},
        { Qt::Key_Slash,       {2, { XK_slash, XK_KP_Divide }}},
        { Qt::Key_F1,          {1, { XK_F1 }}},
        { Qt::Key_F2,          {1, { XK_F2 }}},
        { Qt::Key_F3,          {1, { XK_F3 }}},
        { Qt::Key_F4,          {1, { XK_F4 }}},
        { Qt::Key_F5,          {1, { XK_F5 }}},
        { Qt::Key_F6,          {1, { XK_F6 }}},
        { Qt::Key_F7,          {1, { XK_F7 }}},
        { Qt::Key_F8,          {1, { XK_F8 }}},
        { Qt::Key_F9,          {1, { XK_F9 }}},
        { Qt::Key_F10,         {1, { XK_F10 }}},
        { Qt::Key_F11,         {1, { XK_F11 }}},
        { Qt::Key_F12,         {1, { XK_F12 }}},
        { Qt::Key_F13,         {1, { XK_F13 }}},
        { Qt::Key_F14,         {1, { XK_F14 }}},
        { Qt::Key_F15,         {1, { XK_F15 }}},
        { Qt::Key_F16,         {1, { XK_F16 }}},
        { Qt::Key_F17,         {1, { XK_F17 }}},
        { Qt::Key_F18,         {1, { XK_F18 }}},
        { Qt::Key_F19,         {1, { XK_F19 }}},
        { Qt::Key_F20,         {1, { XK_F20 }}},
        { Qt::Key_F21,         {1, { XK_F21 }}},
        { Qt::Key_F22,         {1, { XK_F22 }}},
        { Qt::Key_F23,         {1, { XK_F23 }}},
        { Qt::Key_F24,         {1, { XK_F24 }}},
        { Qt::Key_F25,         {1, { XK_F25 }}},
        { Qt::Key_F26,         {1, { XK_F26 }}},
        { Qt::Key_F27,         {1, { XK_F27 }}},
        { Qt::Key_F28,         {1, { XK_F28 }}},
        { Qt::Key_F29,         {1, { XK_F29 }}},
        { Qt::Key_F30,         {1, { XK_F30 }}},
        { Qt::Key_F31,         {1, { XK_F31 }}},
        { Qt::Key_F32,         {1, { XK_F32 }}},
        { Qt::Key_F33,         {1, { XK_F33 }}},
        { Qt::Key_F34,         {1, { XK_F34 }}},
        { Qt::Key_F35,         {1, { XK_F35 }}},
        { Qt::Key_Super_L,     {1, { XK_Super_L }}},
        { Qt::Key_Super_R,     {1, { XK_Super_R }}},
        { Qt::Key_Menu,        {1, { XK_Menu }}},
        { Qt::Key_Hyper_L,     {1, { XK_Hyper_L }}},
        { Qt::Key_Hyper_R,     {1, { XK_Hyper_R }}},
        { Qt::Key_Help,        {1, { XK_Help }}},
        { Qt::Key_Direction_L, {0, { 0 }}},
        { Qt::Key_Direction_R, {0, { 0 }}},

        { Qt::Key_unknown,     {0, { 0 }}},
};

GlobalShortcutManager::KeyTrigger::KeyTrigger(const QKeySequence& key)
{
        d.reset(new Impl(this, key));
}

GlobalShortcutManager::KeyTrigger::~KeyTrigger()
{
	d.reset();
	/*
        delete d;
        d = 0;
        */
}

bool GlobalShortcutManager::KeyTrigger::isConnected()
{
  	if (!d) return false;	
	return !GlobalShortcutManager::KeyTrigger::Impl::failed;
}



//typedef GlobalShortcutManager::KeyTrigger::Impl mytrigger;

void X11KeyTriggerManager::xkeyPressed(XEvent* event) {
    //    qDebug() << "Receieved key press!";
    Display* dsp = QX11Info::display();
	
    unsigned int mod = event->xkey.state & (meta_mask | ShiftMask | ControlMask | alt_mask);
    
    unsigned int keysym = XKeycodeToKeysym(dsp, event->xkey.keycode, 0);
    
    bool found = false;
    uint n = 0;
    for (n = 0; qt_xk_table[n].key != Qt::Key_unknown; ++n) {
	if ((unsigned int) qt_xk_table[n].xk.sym[0] == keysym) {
	    found = true;
	    break;
	}
    }
    if (!found) return;
    
    unsigned int keyout;
    keyout = qt_xk_table[n].key;
    
    if (mod & meta_mask)
	keyout |= Qt::META;
    if (mod & ShiftMask)
	keyout |= Qt::SHIFT;
    if (mod & ControlMask)
	keyout |= Qt::CTRL;
    if (mod & alt_mask)
	keyout |= Qt::ALT;

    //    QKeySequence out(keyout);
    //qDebug() << mod << keysym << out;



    foreach(X11KeyTrigger* trigger, triggers_) {
	if (trigger->isAccepted(keyout)) {
	    trigger->activate();
	    return;
	}
    }        
}

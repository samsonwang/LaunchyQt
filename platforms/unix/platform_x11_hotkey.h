#pragma once

#include "platform_base_hotkey.h"
#include "platform_base_hottrigger.h"

#include <QWidget>
#include <QX11Info>
#include <QKeyEvent>
//#include <QCoreApplication>
#include <QDebug>
#include <QApplication>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "platform_x11_hotkey.h"

#include <boost/shared_ptr.hpp>

using namespace boost;

#ifdef KeyPress
// defined by X11 headers
const int XKeyPress   = KeyPress;
const int XKeyRelease = KeyRelease;
#undef KeyPress
#endif


class X11KeyTrigger
{
public:
        virtual ~X11KeyTrigger() {}
        virtual void activate() = 0;
        virtual bool isAccepted(int qkey) const = 0;
};


class X11KeyTriggerManager : public QObject
{
    Q_OBJECT
public:
        static X11KeyTriggerManager* instance()
        {
                if(!instance_)
                        instance_ = new X11KeyTriggerManager();
                return instance_;
        }

        void addTrigger(X11KeyTrigger* trigger)
        {
                triggers_ << trigger;
        }

        void removeTrigger(X11KeyTrigger* trigger)
        {
                triggers_.removeAll(trigger);
        }

        struct Qt_XK_Keygroup
        {
                char num;
                int sym[3];
        };

protected:
        // reimplemented
        bool eventFilter(QObject* o, QEvent* e)
        {
                if(e->type() == QEvent::KeyPress) {
                        QKeyEvent* k = static_cast<QKeyEvent*>(e);
                        int qkey = k->key();
                        if (k->modifiers() & Qt::ShiftModifier)
                                qkey |= Qt::SHIFT;
                        if (k->modifiers() & Qt::ControlModifier)
                                qkey |= Qt::CTRL;
                        if (k->modifiers() & Qt::AltModifier)
                                qkey |= Qt::ALT;
                        if (k->modifiers() & Qt::MetaModifier)
                                qkey |= Qt::META;

                        foreach(X11KeyTrigger* trigger, triggers_) {
                                if (trigger->isAccepted(qkey)) {
                                        trigger->activate();
                                        return true;
                                }
                        }
                }

                return QObject::eventFilter(o, e);
        }


public slots:
    void xkeyPressed(XEvent*);

    


private:
        X11KeyTriggerManager()
                : QObject(QCoreApplication::instance())
        {
	    // This does't always catch hotkeys if the app hasn't had mouse
	    // attention yet, so I built the xkeypressed system instead which
	    // monitors x11events as opposed to qt events
	    //qApp->installEventFilter(this);
	    connect(qApp, SIGNAL(xkeyPressed(XEvent*)), this, SLOT(xkeyPressed(XEvent*)));
        }

        static X11KeyTriggerManager* instance_;
        QList<X11KeyTrigger*> triggers_;

private:
        struct Qt_XK_Keymap
        {
                int key;
                Qt_XK_Keygroup xk;
        };

        static Qt_XK_Keymap qt_xk_table[];
        static long alt_mask;
        static long meta_mask;
        static long super_mask;
        static long hyper_mask;
        static long numlock_mask;
        static bool haveMods;

        // adapted from qapplication_x11.cpp
        static void ensureModifiers()
        {
                if (haveMods)
                        return;

                Display* appDpy = QX11Info::display();
                XModifierKeymap* map = XGetModifierMapping(appDpy);
                if (map) {
                        // XKeycodeToKeysym helper code adapeted from xmodmap
                        int min_keycode, max_keycode, keysyms_per_keycode = 1;
                        XDisplayKeycodes (appDpy, &min_keycode, &max_keycode);
                        XFree(XGetKeyboardMapping (appDpy, min_keycode, (max_keycode - min_keycode + 1), &keysyms_per_keycode));
                        
                        int i, maskIndex = 0, mapIndex = 0;
                        for (maskIndex = 0; maskIndex < 8; maskIndex++) {
                                for (i = 0; i < map->max_keypermod; i++) {
                                        if (map->modifiermap[mapIndex]) {
                                                KeySym sym;
                                                int symIndex = 0;
                                                do {
                                                        sym = XKeycodeToKeysym(appDpy, map->modifiermap[mapIndex], symIndex);
                                                        symIndex++;
                                                } while ( !sym && symIndex < keysyms_per_keycode);
                                                if (alt_mask == 0 && (sym == XK_Alt_L || sym == XK_Alt_R)) {
                                                        alt_mask = 1 << maskIndex;
                                                }
                                                if (meta_mask == 0 && (sym == XK_Meta_L || sym == XK_Meta_R)) {
                                                        meta_mask = 1 << maskIndex;
                                                }
                                                if (super_mask == 0 && (sym == XK_Super_L || sym == XK_Super_R)) {
                                                        super_mask = 1 << maskIndex;
                                                }
                                                if (hyper_mask == 0 && (sym == XK_Hyper_L || sym == XK_Hyper_R)) {
                                                        hyper_mask = 1 << maskIndex;
                                                }
                                                if (numlock_mask == 0 && (sym == XK_Num_Lock)) {
                                                        numlock_mask = 1 << maskIndex;
                                                }
                                        }
                                        mapIndex++;
                                }
                        }

                        XFreeModifiermap(map);

                        // logic from qt source see gui/kernel/qkeymapper_x11.cpp
                        if (meta_mask == 0 || meta_mask == alt_mask) {
                                // no meta keys... s,meta,super,
                                meta_mask = super_mask;
                                if (meta_mask == 0 || meta_mask == alt_mask) {
                                        // no super keys either? guess we'll use hyper then
                                        meta_mask = hyper_mask;
                                }
                        }
                }
                else {
                        // assume defaults
                        alt_mask = Mod1Mask;
                        meta_mask = Mod4Mask;
                }

                haveMods = true;
        }

public:
        static bool convertKeySequence(const QKeySequence& ks, unsigned int* _mod, Qt_XK_Keygroup* _kg)
        {
                int code = ks;
                ensureModifiers();

                unsigned int mod = 0;
                if (code & Qt::META)
                        mod |= meta_mask;
                if (code & Qt::SHIFT)
                        mod |= ShiftMask;
                if (code & Qt::CTRL)
                        mod |= ControlMask;
                if (code & Qt::ALT)
                        mod |= alt_mask;

                Qt_XK_Keygroup kg;
		kg.num = 0;
                kg.sym[0] = 0;
                code &= ~Qt::KeyboardModifierMask;

                bool found = false;
                for (int n = 0; qt_xk_table[n].key != Qt::Key_unknown; ++n) {
                        if (qt_xk_table[n].key == code) {
                                kg = qt_xk_table[n].xk;
                                found = true;
                                break;
                        }
                }

                if (!found) {
                        // try latin1
                        if (code >= 0x20 && code <= 0x7f) {
                                kg.num = 1;
                                kg.sym[0] = code;
                        }
                }

                if (!kg.num)
                        return false;

                if (_mod)
                        *_mod = mod;
                if (_kg)
                        *_kg = kg;

                return true;
        }

        static QList<long> ignModifiersList()
        {
                QList<long> ret;
                if (numlock_mask) {
                        ret << 0 << LockMask << numlock_mask << (LockMask | numlock_mask);
                }
                else {
                        ret << 0 << LockMask;
                }
                return ret;
        }
};


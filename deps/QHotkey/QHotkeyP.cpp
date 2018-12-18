
#include "QHotkeyP.h"
#include <QMultiHash>
#include <QCoreApplication>
#include <QDebug>

QScopedPointer<QHotkeyPrivate::EventFilter> QHotkeyPrivate::s_eventFilter;
QMultiHash<quint32, QHotkey*> QHotkeyPrivate::s_hotKeys;

QHotkeyPrivate::QHotkeyPrivate(QHotkey* q)
    : q_ptr(q) {
    init();
}

QHotkeyPrivate::~QHotkeyPrivate() {
    unsetKey();
}

void QHotkeyPrivate::init() {
    if (s_eventFilter.isNull()) {
        s_eventFilter.reset(new EventFilter);
        qApp->installNativeEventFilter(s_eventFilter.data());
    }
}

void QHotkeyPrivate::unsetKey() {
    quint32 keyId = calcHotkeyId(m_keySeq);
    Q_Q(QHotkey);
    if (s_hotKeys.remove(keyId, q) > 0) {
        if (s_hotKeys.count(keyId) == 0) {
            quint32 keycode = toNativeKeycode(getKey(m_keySeq));
            quint32 mods = toNativeModifiers(getModifiers(m_keySeq));
            unregisterKey(keycode, mods, keyId);
        }
    }
    m_bRegistered = false;
}

bool QHotkeyPrivate::activateHotKey(int keyId) {
    qDebug() << "QHotkeyPrivate::activateHotKey, keyId:" << keyId;
    if (s_hotKeys.contains(keyId)) {
        foreach(QHotkey* s, s_hotKeys.values(keyId)) {
            s->activated();
        }
        return true;
    }
    return false;
}

int QHotkeyPrivate::calcHotkeyId(const QKeySequence& keySeq) {
    if (keySeq.isEmpty()) {
        return 0;
    }
    quint32 key = toNativeKeycode(getKey(keySeq));
    quint32 mod = toNativeModifiers(getModifiers(keySeq));
    return calcHotkeyId(key, mod);
}

Qt::Key QHotkeyPrivate::getKey(const QKeySequence& keySeq) {
    if (keySeq.isEmpty()) {
        return Qt::Key(0);
    }
    return Qt::Key(keySeq[0] & ~Qt::KeyboardModifierMask);
}

Qt::KeyboardModifiers QHotkeyPrivate::getModifiers(const QKeySequence& keySeq) {
    if (keySeq.isEmpty()) {
        return Qt::KeyboardModifiers(0);
    }
    return Qt::KeyboardModifiers(keySeq[0] & Qt::KeyboardModifierMask);
}

const QKeySequence& QHotkeyPrivate::keySeq() const {
    return m_keySeq;
}

void QHotkeyPrivate::setKeySeq(const QKeySequence& keySeq) {
    qDebug() << "QHotkeyPrivate::setKeySeq, keySeq:" << keySeq;
    if (!m_keySeq.isEmpty()) {
        unsetKey();
    }

    quint32 keyId = calcHotkeyId(keySeq);
    if (s_hotKeys.count(keyId) == 0) {
        quint32 keycode = toNativeKeycode(getKey(keySeq));
        quint32 mods = toNativeModifiers(getModifiers(keySeq));
        qDebug() << "QHotkeyPrivate::setKeySeq, keyCode:" << keycode
            << "mods:" << mods << "keyId:" << keyId;
        if (!registerKey(keycode, mods, keyId)) {
            // register fail
            return;
        }
    }

    m_keySeq = keySeq;

    Q_Q(QHotkey);
    s_hotKeys.insert(keyId, q);

    m_bRegistered = true;
}

bool QHotkeyPrivate::registered() const {
    return m_bRegistered;
}

quint32 QHotkeyPrivate::toNativeKeycode(Qt::Key key) {
    for (size_t i = 0; s_keyMap[i].qtKey != Qt::Key_unknown; ++i) {
        if (s_keyMap[i].qtKey == key) {
            return s_keyMap[i].nativeKey;
        }
    }
    return 0;
}

quint32 QHotkeyPrivate::toNativeModifiers(Qt::KeyboardModifiers mod) {
    quint32 modNative = 0;
    for (size_t i = 0; s_modMap[i].qtMod != Qt::KeyboardModifierMask; ++i) {
        if (s_modMap[i].qtMod & mod) {
            modNative |= s_modMap[i].nativeMod;
        }
    }
    return modNative;
}

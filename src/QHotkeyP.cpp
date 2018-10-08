
#include "QHotKeyP.h"

QScopedPointer<QHotKeyPrivate::EventFilter> QHotKeyPrivate::s_eventFilter;
QMultiHash<quint32, QHotKey*> QHotKeyPrivate::s_hotKeys;

QHotKeyPrivate::QHotKeyPrivate(QHotKey* q)
    : q_ptr(q) {
    init();
}

QHotKeyPrivate::~QHotKeyPrivate() {
    unsetKey();
}

void QHotKeyPrivate::init() {
    if (s_eventFilter.isNull()) {
        s_eventFilter.reset(new EventFilter);
        qApp->installNativeEventFilter(s_eventFilter.data());
    }
}

void QHotKeyPrivate::unsetKey() {
    quint32 keyId = calcHotKeyId(m_keySeq);
    Q_Q(QHotKey);
    if (s_hotKeys.remove(keyId, q) > 0) {
        if (s_hotKeys.count(keyId) == 0) {
            quint32 keycode = toNativeKeycode(getKey(m_keySeq));
            quint32 mods = toNativeModifiers(getModifiers(m_keySeq));
            unregisterKey(keycode, mods, keyId);
        }
    }
}

bool QHotKeyPrivate::activateHotKey(int keyId) {
    if (s_hotKeys.contains(keyId)) {
        foreach(QHotKey* s, s_hotKeys.values(keyId)) {
            s->activated();
        }
        return true;
    }
    return false;
}

int QHotKeyPrivate::calcHotKeyId(const QKeySequence& keySeq) {
    if (keySeq.isEmpty()) {
        return 0;
    }
    return keySeq[0];
}

Qt::Key QHotKeyPrivate::getKey(const QKeySequence& keySeq) {
    if (keySeq.isEmpty()) {
        return Qt::Key(0);
    }
    return Qt::Key(keySeq[0] & ~Qt::KeyboardModifierMask);
}

Qt::KeyboardModifiers QHotKeyPrivate::getModifiers(const QKeySequence& keySeq) {
    if (keySeq.isEmpty()) {
        return Qt::KeyboardModifiers(0);
    }
    return Qt::KeyboardModifiers(keySeq[0] & Qt::KeyboardModifierMask);
}

QKeySequence QHotKeyPrivate::keySeq() const {
    return m_keySeq;
}

void QHotKeyPrivate::setKeySeq(const QKeySequence& keySeq) {
    if (!m_keySeq.isEmpty()) {
        unsetKey();
    }

    quint32 keyId = calcHotKeyId(keySeq);
    if (s_hotKeys.count(keyId) == 0) {
        quint32 keycode = toNativeKeycode(getKey(keySeq));
        quint32 mods = toNativeModifiers(getModifiers(keySeq));
        registerKey(keycode, mods, keyId);
    }

    m_keySeq = keySeq;

    Q_Q(QHotKey);
    s_hotKeys.insert(keyId, q);
}

quint32 QHotKeyPrivate::toNativeKeycode(Qt::Key key) {
    for (size_t i = 0; s_keyMap[i].qtKey != Qt::Key_unknown; ++i) {
        if (s_keyMap[i].qtKey == key) {
            return s_keyMap[i].nativeKey;
        }
    }
    return 0;
}

quint32 QHotKeyPrivate::toNativeModifiers(Qt::KeyboardModifiers mod) {
    quint32 modNative = 0;
    for (int i = 0; s_modMap[i].qtMod != Qt::KeyboardModifierMask; ++i) {
        if (s_modMap[i].qtMod & mod) {
            modNative |= s_modMap[i].nativeMod;
        }
    }
    return modNative;
}


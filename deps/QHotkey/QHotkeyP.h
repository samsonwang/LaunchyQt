#pragma once

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QKeySequence>

#include "QHotkey.h"

class QHotkeyPrivate : public QObject {
    Q_OBJECT
public:
    Q_DECLARE_PUBLIC(QHotkey)

    explicit QHotkeyPrivate(QHotkey* q);
    virtual ~QHotkeyPrivate();

public:
    const QKeySequence& keySeq() const;
    void setKeySeq(const QKeySequence& keySeq);
    bool registered() const;

private:
    void init();
    void unsetKey();


private:
    static bool activateHotKey(int keyId);
    static int calcHotkeyId(const QKeySequence& keySeq);
    static int calcHotkeyId(quint32 key, quint32 mod);
    static inline Qt::Key getKey(const QKeySequence& keySeq);
    static inline Qt::KeyboardModifiers getModifiers(const QKeySequence& keySeq);
    static quint32 toNativeKeycode(Qt::Key key);
    static quint32 toNativeModifiers(Qt::KeyboardModifiers mod);
    static bool registerKey(quint32 key, quint32 mod, int keyId);
    static void unregisterKey(quint32 key, quint32 mod, int keyId);

private:
    class EventFilter : public QAbstractNativeEventFilter {
    public:
        virtual bool nativeEventFilter(const QByteArray& eventType,
                                       void* message,
                                       qintptr* result);
    };

private:
    QHotkey* q_ptr;
    QKeySequence m_keySeq;
    bool m_bRegistered;

    static QScopedPointer<EventFilter> s_eventFilter;
    static QMultiHash<quint32, QHotkey*> s_hotKeys;

    static struct NativeKeyMap {
        Qt::Key qtKey;
        quint32 nativeKey;
    } s_keyMap[];

    static struct NativeModMap {
        Qt::KeyboardModifier qtMod;
        quint32 nativeMod;
    } s_modMap[];
};

#pragma once

#include <QObject>
#include "QHotKey.h"

class QHotKeyPrivate : public QObject {
    Q_OBJECT
public:
    Q_DECLARE_PUBLIC(QHotKey)

    explicit QHotKeyPrivate(QHotKey* q);
    virtual ~QHotKeyPrivate();

public:
    const QKeySequence& keySeq() const;
    void setKeySeq(const QKeySequence& keySeq);

private:
    void init();
    void unsetKey();
    

private:
    static bool activateHotKey(int keyId);
    static inline int calcHotKeyId(const QKeySequence& keySeq);
    //static inline int calcHotKeyId(quint32 mod, quint32 key);
    static inline Qt::Key getKey(const QKeySequence& keySeq);
    static inline Qt::KeyboardModifiers getModifiers(const QKeySequence& keySeq);
    static quint32 toNativeKeycode(Qt::Key key);
    static quint32 toNativeModifiers(Qt::KeyboardModifiers mod);
    static void registerKey(quint32 key, quint32 mod, int keyId);
    static void unregisterKey(quint32 key, quint32 mod, int keyId);

private:
    class EventFilter : public QAbstractNativeEventFilter {
    public:
        virtual bool nativeEventFilter(const QByteArray &eventType,
                                       void *message,
                                       long *result);
    };

private:
    QHotKey* q_ptr;
    QKeySequence m_keySeq;
    static QScopedPointer<EventFilter> s_eventFilter;
    static QMultiHash<quint32, QHotKey*> s_hotKeys;

    static struct NativeKeyMap {
        Qt::Key qtKey;
        quint32 nativeKey;
    } s_keyMap[];

    static struct NativeModMap {
        Qt::KeyboardModifier qtMod;
        quint32      nativeMod;
    } s_modMap[];
};

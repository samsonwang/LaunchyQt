#pragma once

#include <QObject>

class QHotkeyPrivate;

class QHotkey : public QObject {
    Q_OBJECT
public:
    explicit QHotkey(QObject* parent = nullptr);
    virtual ~QHotkey();

public:
    const QKeySequence& keySeq() const;
    void setKeySeq(const QKeySequence& keySeq);
    bool registered() const;

Q_SIGNALS:
    void activated();

private:
    QScopedPointer<QHotkeyPrivate> const d_ptr;
    Q_DECLARE_PRIVATE(QHotkey)
    Q_DISABLE_COPY(QHotkey)
};

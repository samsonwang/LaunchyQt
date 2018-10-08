#pragma once

#include <QObject>

class QHotKeyPrivate;

class QHotKey : public QObject {
    Q_OBJECT
public:
    explicit QHotKey(QObject* parent = nullptr);
    virtual ~QHotKey();

public:
    const QKeySequence& keySeq() const;
    void setKeySeq(const QKeySequence& keySeq);

Q_SIGNALS:
    void activated();

private:
    QScopedPointer<QHotKeyPrivate> const d_ptr;
    Q_DECLARE_PRIVATE(QHotKey)
    Q_DISABLE_COPY(QHotKey)
};

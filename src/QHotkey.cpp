
#include "QHotKey.h"
#include "QHotKeyP.h"

QHotKey::QHotKey(QObject* parent)
    : QObject(parent),
      d_ptr(new QHotKeyPrivate(this)) {

}

QHotKey::~QHotKey() {

}

const QKeySequence& QHotKey::keySeq() const {
    Q_D(const QHotKey);
    return d->keySeq();
}

void QHotKey::setKeySeq(const QKeySequence& keySeq) {
    Q_D(QHotKey);
    d->setKeySeq(keySeq);
}

bool QHotKey::registered() const
{
    Q_D(const QHotKey);
    return d->registered();
}

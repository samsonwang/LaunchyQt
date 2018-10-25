
#include "QHotkey.h"
#include "QHotkeyP.h"

QHotkey::QHotkey(QObject* parent)
    : QObject(parent),
      d_ptr(new QHotkeyPrivate(this)) {

}

QHotkey::~QHotkey() {

}

const QKeySequence& QHotkey::keySeq() const {
    Q_D(const QHotkey);
    return d->keySeq();
}

void QHotkey::setKeySeq(const QKeySequence& keySeq) {
    Q_D(QHotkey);
    d->setKeySeq(keySeq);
}

bool QHotkey::registered() const
{
    Q_D(const QHotkey);
    return d->registered();
}

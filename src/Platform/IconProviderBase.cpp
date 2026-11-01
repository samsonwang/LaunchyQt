
#include "IconProviderBase.h"

namespace launchy {

IconProviderBase::IconProviderBase()
    : m_preferredSize(32) {

}

IconProviderBase::~IconProviderBase() {

}

void IconProviderBase::setPreferredIconSize(int size) {
    m_preferredSize = size;
}

} // namespace launchy

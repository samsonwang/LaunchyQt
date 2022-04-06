
#pragma once

#include <QFileIconProvider>

namespace launchy {

class IconProviderBase : public QFileIconProvider {
public:
    IconProviderBase();
    virtual ~IconProviderBase();

public:
    void setPreferredIconSize(int size);

protected:
    int m_preferredSize;
};

} // namespace launchy

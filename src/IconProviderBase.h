
#pragma once

#include <QFileIconProvider>

namespace launchy {

class IconProviderBase : public QFileIconProvider {
public:
    IconProviderBase();
    virtual ~IconProviderBase();

    void setPreferredIconSize(int size);

protected:
    int m_preferredSize;
};

}
